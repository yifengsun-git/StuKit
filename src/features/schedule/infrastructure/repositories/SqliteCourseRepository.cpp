#include "repositories/SqliteCourseRepository.h"

#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <utility>

namespace stukit::schedule {

SqliteCourseRepository::SqliteCourseRepository(QSqlDatabase database)
    : m_database(std::move(database))
{
}

bool SqliteCourseRepository::save(const Course &course)
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO courses(id, term_id, name, teacher, location, "
        "assessment_standard, teaching_plan, color) "
        "VALUES(:id, :termId, :name, :teacher, :location, "
        ":assessmentStandard, :teachingPlan, :color) "
        "ON CONFLICT(id) DO UPDATE SET "
        "term_id = excluded.term_id, "
        "name = excluded.name, "
        "teacher = excluded.teacher, "
        "location = excluded.location, "
        "assessment_standard = excluded.assessment_standard, "
        "teaching_plan = excluded.teaching_plan, "
        "color = excluded.color"));
    query.bindValue(
        QStringLiteral(":id"), course.id().toString(QUuid::WithoutBraces));
    query.bindValue(
        QStringLiteral(":termId"),
        course.termId().toString(QUuid::WithoutBraces));
    query.bindValue(QStringLiteral(":name"), course.name());
    query.bindValue(
        QStringLiteral(":teacher"),
        course.teacher().isNull() ? QStringLiteral("") : course.teacher());
    query.bindValue(
        QStringLiteral(":location"),
        course.location().isNull() ? QStringLiteral("") : course.location());
    query.bindValue(
        QStringLiteral(":assessmentStandard"),
        course.assessmentStandard().isNull()
            ? QStringLiteral("") : course.assessmentStandard());
    query.bindValue(
        QStringLiteral(":teachingPlan"),
        course.teachingPlan().isNull()
            ? QStringLiteral("") : course.teachingPlan());
    query.bindValue(QStringLiteral(":color"), course.color());

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return false;
    }
    return true;
}

bool SqliteCourseRepository::remove(const QUuid &id)
{
    m_lastError.clear();
    if (!m_database.isOpen() || id.isNull()) {
        m_lastError = QStringLiteral("Database is not open or id is invalid");
        return false;
    }
    if (!m_database.transaction()) {
        m_lastError = m_database.lastError().text();
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral("DELETE FROM courses WHERE id = :id"));
    query.bindValue(QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec() || query.numRowsAffected() != 1) {
        m_lastError = query.lastError().text();
        m_database.rollback();
        return false;
    }
    if (!m_database.commit()) {
        m_lastError = m_database.lastError().text();
        m_database.rollback();
        return false;
    }
    return true;
}

std::optional<Course> SqliteCourseRepository::findById(
    const QUuid &id) const
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, term_id, name, teacher, location, assessment_standard, "
        "teaching_plan, color "
        "FROM courses WHERE id = :id"));
    query.bindValue(
        QStringLiteral(":id"), id.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    if (!query.next()) {
        return std::nullopt;
    }
    return courseFromCurrentRow(query);
}

std::optional<std::vector<Course>> SqliteCourseRepository::findByTermId(
    const QUuid &termId) const
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, term_id, name, teacher, location, assessment_standard, "
        "teaching_plan, color "
        "FROM courses WHERE term_id = :termId "
        "ORDER BY name COLLATE NOCASE, id"));
    query.bindValue(
        QStringLiteral(":termId"), termId.toString(QUuid::WithoutBraces));
    if (!query.exec()) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    return coursesFromQuery(query);
}

std::optional<std::vector<Course>> SqliteCourseRepository::findAll() const
{
    m_lastError.clear();
    if (!m_database.isOpen()) {
        m_lastError = QStringLiteral("Database is not open");
        return std::nullopt;
    }

    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral(
            "SELECT id, term_id, name, teacher, location, assessment_standard, "
            "teaching_plan, color "
            "FROM courses "
            "ORDER BY term_id, name COLLATE NOCASE, id"))) {
        m_lastError = query.lastError().text();
        return std::nullopt;
    }
    return coursesFromQuery(query);
}

const QString &SqliteCourseRepository::lastError() const noexcept
{
    return m_lastError;
}

std::optional<Course> SqliteCourseRepository::courseFromCurrentRow(
    const QSqlQuery &query) const
{
    const auto course = Course::create(CourseData{
        .id = QUuid(query.value(QStringLiteral("id")).toString()),
        .termId = QUuid(query.value(QStringLiteral("term_id")).toString()),
        .name = query.value(QStringLiteral("name")).toString(),
        .teacher = query.value(QStringLiteral("teacher")).toString(),
        .location = query.value(QStringLiteral("location")).toString(),
        .assessmentStandard = query.value(
            QStringLiteral("assessment_standard")).toString(),
        .teachingPlan = query.value(QStringLiteral("teaching_plan")).toString(),
        .color = query.value(QStringLiteral("color")).toString(),
    });
    if (!course.has_value()) {
        m_lastError = QStringLiteral("Database contains an invalid course");
    }
    return course;
}

std::optional<std::vector<Course>> SqliteCourseRepository::coursesFromQuery(
    QSqlQuery &query) const
{
    std::vector<Course> courses;
    while (query.next()) {
        auto course = courseFromCurrentRow(query);
        if (!course.has_value()) {
            return std::nullopt;
        }
        courses.push_back(std::move(*course));
    }
    return courses;
}

} // namespace stukit::schedule
