#pragma once

#include "GetScheduleSetup.h"
#include "SaveAcademicTerm.h"
#include "SaveCourse.h"
#include "SaveCourseMeeting.h"
#include "ScheduleManagementModels.h"

#include <QAbstractItemModel>
#include <QObject>
#include <QString>
#include <QVariantList>

#include <vector>

namespace stukit::schedule::presentation {

class ScheduleManagementViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel *terms READ terms CONSTANT)
    Q_PROPERTY(QAbstractItemModel *courses READ courses CONSTANT)
    Q_PROPERTY(QAbstractItemModel *meetings READ meetings CONSTANT)
    Q_PROPERTY(QString selectedTermId READ selectedTermId NOTIFY selectionChanged)
    Q_PROPERTY(QString selectedCourseId READ selectedCourseId NOTIFY selectionChanged)
    Q_PROPERTY(bool selectedTermArchived READ selectedTermArchived NOTIFY selectionChanged)
    Q_PROPERTY(int selectedTermTotalWeeks READ selectedTermTotalWeeks NOTIFY selectionChanged)
    Q_PROPERTY(bool hasTerms READ hasTerms NOTIFY setupChanged)
    Q_PROPERTY(bool hasCourses READ hasCourses NOTIFY setupChanged)
    Q_PROPERTY(bool hasMeetings READ hasMeetings NOTIFY setupChanged)
    Q_PROPERTY(QVariantList conflicts READ conflicts NOTIFY conflictsChanged)
    Q_PROPERTY(bool hasConflicts READ hasConflicts NOTIFY conflictsChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)

public:
    ScheduleManagementViewModel(
        GetScheduleSetup &getScheduleSetup,
        SaveAcademicTerm &saveAcademicTerm,
        SaveCourse &saveCourse,
        SaveCourseMeeting &saveCourseMeeting,
        QObject *parent = nullptr);

    [[nodiscard]] QAbstractItemModel *terms() noexcept;
    [[nodiscard]] QAbstractItemModel *courses() noexcept;
    [[nodiscard]] QAbstractItemModel *meetings() noexcept;
    [[nodiscard]] QString selectedTermId() const;
    [[nodiscard]] QString selectedCourseId() const;
    [[nodiscard]] bool selectedTermArchived() const noexcept;
    [[nodiscard]] int selectedTermTotalWeeks() const noexcept;
    [[nodiscard]] bool hasTerms() const noexcept;
    [[nodiscard]] bool hasCourses() const noexcept;
    [[nodiscard]] bool hasMeetings() const noexcept;
    [[nodiscard]] QVariantList conflicts() const;
    [[nodiscard]] bool hasConflicts() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;

    Q_INVOKABLE void load();
    Q_INVOKABLE bool selectTerm(const QString &termId);
    Q_INVOKABLE bool selectCourse(const QString &courseId);
    Q_INVOKABLE bool saveTerm(
        const QString &termId,
        const QString &name,
        const QString &startDateText,
        int totalWeeks,
        bool archived);
    Q_INVOKABLE bool setTermArchived(
        const QString &termId, bool archived);
    Q_INVOKABLE bool saveCourseDetails(
        const QString &courseId,
        const QString &termId,
        const QString &name,
        const QString &teacher,
        const QString &location,
        const QString &color);
    Q_INVOKABLE bool saveMeetingDetails(
        const QString &meetingId,
        const QString &courseId,
        int weekday,
        int startPeriod,
        int endPeriod,
        int startWeek,
        int endWeek,
        int weekPattern);
    Q_INVOKABLE void clearMessages();

signals:
    void setupChanged();
    void selectionChanged();
    void conflictsChanged();
    void errorChanged();
    void dataChanged();

private:
    [[nodiscard]] const AcademicTerm *findTerm(const QUuid &id) const;
    [[nodiscard]] const Course *findCourse(const QUuid &id) const;
    void rebuildFilteredModels();
    void rebuildMeetingModel();
    void setConflicts(std::vector<ScheduleConflict> conflicts);
    void clearConflicts();
    void setError(QString message);
    void clearError();

    GetScheduleSetup &m_getScheduleSetup;
    SaveAcademicTerm &m_saveAcademicTerm;
    SaveCourse &m_saveCourse;
    SaveCourseMeeting &m_saveCourseMeeting;
    AcademicTermListModel m_termsModel;
    CourseListModel m_coursesModel;
    CourseMeetingListModel m_meetingsModel;
    std::vector<AcademicTerm> m_terms;
    std::vector<Course> m_courses;
    std::vector<CourseMeeting> m_meetings;
    std::vector<ScheduleConflict> m_conflicts;
    QUuid m_selectedTermId;
    QUuid m_selectedCourseId;
    QString m_errorMessage;
};

} // namespace stukit::schedule::presentation
