#include "ScheduleViewModel.h"

#include <QVariantMap>
#include <QStringList>

#include <algorithm>
#include <utility>

namespace stukit::schedule::presentation {

ScheduleViewModel::ScheduleViewModel(
    GetAcademicTerms &getAcademicTerms,
    GetWeeklySchedule &getWeeklySchedule,
    QDate today,
    QUuid preferredTermId,
    QObject *parent)
    : QObject(parent)
    , m_getAcademicTerms(getAcademicTerms)
    , m_getWeeklySchedule(getWeeklySchedule)
    , m_today(today)
    , m_preferredTermId(std::move(preferredTermId))
{
}

QVariantList ScheduleViewModel::termOptions() const
{
    QVariantList options;
    options.reserve(static_cast<qsizetype>(m_terms.size()));
    for (const AcademicTerm &term : m_terms) {
        options.push_back(QVariantMap{
            {QStringLiteral("id"),
             term.id().toString(QUuid::WithoutBraces)},
            {QStringLiteral("name"), term.name()},
            {QStringLiteral("label"),
             term.isArchived()
                 ? QStringLiteral("%1（已归档）").arg(term.name())
                 : term.name()},
            {QStringLiteral("startDate"), term.startDate()},
            {QStringLiteral("totalWeeks"), term.totalWeeks()},
            {QStringLiteral("archived"), term.isArchived()},
        });
    }
    return options;
}

bool ScheduleViewModel::hasTerms() const noexcept
{
    return !m_terms.empty();
}

QString ScheduleViewModel::selectedTermId() const
{
    return m_selectedTerm.has_value()
        ? m_selectedTerm->id().toString(QUuid::WithoutBraces)
        : QString{};
}

QString ScheduleViewModel::selectedTermName() const
{
    return m_selectedTerm.has_value() ? m_selectedTerm->name() : QString{};
}

int ScheduleViewModel::selectedWeek() const noexcept
{
    return m_selectedWeek;
}

int ScheduleViewModel::totalWeeks() const noexcept
{
    return m_selectedTerm.has_value() ? m_selectedTerm->totalWeeks() : 0;
}

QString ScheduleViewModel::weekLabel() const
{
    return m_selectedWeek > 0
        ? QStringLiteral("第 %1 周").arg(m_selectedWeek)
        : QStringLiteral("未选择教学周");
}

QString ScheduleViewModel::weekDateRange() const
{
    if (!m_selectedTerm.has_value() || m_selectedWeek <= 0) {
        return {};
    }
    const QDate firstDay = m_selectedTerm->startDate().addDays(
        static_cast<qint64>(m_selectedWeek - 1) * 7);
    return QStringLiteral("%1 ～ %2")
        .arg(firstDay.toString(QStringLiteral("yyyy-MM-dd")),
             firstDay.addDays(6).toString(QStringLiteral("yyyy-MM-dd")));
}

QVariantList ScheduleViewModel::weekDays() const
{
    if (!m_selectedTerm.has_value() || m_selectedWeek <= 0) {
        return {};
    }

    static const QStringList weekdayNames{
        QStringLiteral("周一"),
        QStringLiteral("周二"),
        QStringLiteral("周三"),
        QStringLiteral("周四"),
        QStringLiteral("周五"),
        QStringLiteral("周六"),
        QStringLiteral("周日"),
    };
    const QDate firstDay = m_selectedTerm->startDate().addDays(
        static_cast<qint64>(m_selectedWeek - 1) * 7);

    QVariantList days;
    days.reserve(7);
    for (int index = 0; index < 7; ++index) {
        const QDate date = firstDay.addDays(index);
        days.push_back(QVariantMap{
            {QStringLiteral("weekday"), index + 1},
            {QStringLiteral("weekdayText"), weekdayNames.at(index)},
            {QStringLiteral("dateText"),
             date.toString(QStringLiteral("MM-dd"))},
            {QStringLiteral("fullDateText"),
             date.toString(QStringLiteral("yyyy-MM-dd"))},
            {QStringLiteral("isToday"), date == m_today},
        });
    }
    return days;
}

bool ScheduleViewModel::canShowPreviousWeek() const noexcept
{
    return m_selectedWeek > 1;
}

bool ScheduleViewModel::canShowNextWeek() const noexcept
{
    return m_selectedTerm.has_value()
        && m_selectedWeek > 0
        && m_selectedWeek < m_selectedTerm->totalWeeks();
}

bool ScheduleViewModel::currentWeekAvailable() const noexcept
{
    return m_selectedTerm.has_value()
        && currentWeekFor(*m_selectedTerm) > 0;
}

bool ScheduleViewModel::showingCurrentWeek() const noexcept
{
    return currentWeekAvailable()
        && m_selectedWeek == currentWeekFor(*m_selectedTerm);
}

bool ScheduleViewModel::isEmpty() const noexcept
{
    return m_meetings.rowCount() == 0;
}

bool ScheduleViewModel::isLoading() const noexcept
{
    return m_loading;
}

bool ScheduleViewModel::hasError() const noexcept
{
    return !m_errorMessage.isEmpty();
}

const QString &ScheduleViewModel::errorMessage() const noexcept
{
    return m_errorMessage;
}

QAbstractItemModel *ScheduleViewModel::meetings() noexcept
{
    return &m_meetings;
}

void ScheduleViewModel::load()
{
    setLoading(true);
    const QUuid previousTermId = m_selectedTerm.has_value()
        ? m_selectedTerm->id()
        : QUuid{};
    const int previousWeek = m_selectedWeek;
    const auto result = m_getAcademicTerms.execute();
    if (std::holds_alternative<GetAcademicTermsError>(result)) {
        m_terms.clear();
        clearSelection();
        emit termsChanged();
        setError(QStringLiteral("无法读取学期数据"));
        setLoading(false);
        return;
    }

    m_terms = std::get<std::vector<AcademicTerm>>(result);
    emit termsChanged();
    const auto term = chooseTerm(previousTermId);
    if (!term.has_value()) {
        clearSelection();
        clearError();
        setLoading(false);
        return;
    }

    const int week = term->id() == previousTermId ? previousWeek : 0;
    applySelectedTerm(*term, week);
    loadSchedule();
    setLoading(false);
}

bool ScheduleViewModel::selectTerm(const QString &termId)
{
    const QUuid id(termId.trimmed());
    const auto term = std::ranges::find_if(
        m_terms,
        [&id](const AcademicTerm &candidate) {
            return !id.isNull() && candidate.id() == id;
        });
    if (term == m_terms.cend()) {
        setError(QStringLiteral("所选学期不存在"));
        return false;
    }
    if (m_selectedTerm.has_value() && m_selectedTerm->id() == id) {
        clearError();
        return true;
    }

    setLoading(true);
    applySelectedTerm(*term);
    loadSchedule();
    setLoading(false);
    return !hasError();
}

void ScheduleViewModel::showPreviousWeek()
{
    if (!canShowPreviousWeek()) {
        return;
    }
    --m_selectedWeek;
    emit selectedWeekChanged();
    setLoading(true);
    loadSchedule();
    setLoading(false);
}

void ScheduleViewModel::showNextWeek()
{
    if (!canShowNextWeek()) {
        return;
    }
    ++m_selectedWeek;
    emit selectedWeekChanged();
    setLoading(true);
    loadSchedule();
    setLoading(false);
}

bool ScheduleViewModel::showCurrentWeek()
{
    if (!m_selectedTerm.has_value()) {
        setError(QStringLiteral("请先选择学期"));
        return false;
    }
    const int week = currentWeekFor(*m_selectedTerm);
    if (week <= 0) {
        setError(QStringLiteral("当前日期不在所选学期内"));
        return false;
    }
    if (m_selectedWeek == week) {
        clearError();
        return true;
    }

    m_selectedWeek = week;
    emit selectedWeekChanged();
    setLoading(true);
    loadSchedule();
    setLoading(false);
    return !hasError();
}

int ScheduleViewModel::currentWeekFor(
    const AcademicTerm &term) const noexcept
{
    if (!m_today.isValid()) {
        return 0;
    }
    const qint64 daysFromStart = term.startDate().daysTo(m_today);
    if (daysFromStart < 0
        || daysFromStart >= static_cast<qint64>(term.totalWeeks()) * 7) {
        return 0;
    }
    return static_cast<int>(daysFromStart / 7) + 1;
}

std::optional<AcademicTerm> ScheduleViewModel::chooseTerm(
    const QUuid &previousTermId) const
{
    const auto findById = [this](const QUuid &id)
        -> std::optional<AcademicTerm> {
        if (id.isNull()) {
            return std::nullopt;
        }
        const auto found = std::ranges::find_if(
            m_terms,
            [&id](const AcademicTerm &term) {
                return term.id() == id;
            });
        return found == m_terms.cend()
            ? std::nullopt
            : std::optional<AcademicTerm>(*found);
    };

    if (auto previous = findById(previousTermId); previous.has_value()) {
        return previous;
    }
    if (auto preferred = findById(m_preferredTermId); preferred.has_value()) {
        return preferred;
    }
    const auto current = std::ranges::find_if(
        m_terms,
        [this](const AcademicTerm &term) {
            return !term.isArchived() && currentWeekFor(term) > 0;
        });
    if (current != m_terms.cend()) {
        return *current;
    }
    const auto active = std::ranges::find_if(
        m_terms,
        [](const AcademicTerm &term) {
            return !term.isArchived();
        });
    if (active != m_terms.cend()) {
        return *active;
    }
    return m_terms.empty()
        ? std::nullopt
        : std::optional<AcademicTerm>(m_terms.front());
}

void ScheduleViewModel::applySelectedTerm(
    AcademicTerm term, int preferredWeek)
{
    m_selectedTerm = std::move(term);

    int week = preferredWeek;
    if (week < 1 || week > m_selectedTerm->totalWeeks()) {
        week = currentWeekFor(*m_selectedTerm);
        if (week <= 0) {
            week = 1;
        }
    }
    m_selectedWeek = week;
    emit selectedTermChanged();
    emit selectedWeekChanged();
}

void ScheduleViewModel::loadSchedule()
{
    if (!m_selectedTerm.has_value() || m_selectedWeek <= 0) {
        m_meetings.clear();
        emit scheduleChanged();
        return;
    }

    const auto result = m_getWeeklySchedule.execute(
        m_selectedTerm->id(), m_selectedWeek);
    if (std::holds_alternative<GetWeeklyScheduleError>(result)) {
        const auto error = std::get<GetWeeklyScheduleError>(result);
        switch (error) {
        case GetWeeklyScheduleError::InvalidInput:
            setError(QStringLiteral("课表请求无效"));
            break;
        case GetWeeklyScheduleError::TermNotFound:
            setError(QStringLiteral("所选学期不存在"));
            break;
        case GetWeeklyScheduleError::WeekOutOfRange:
            setError(QStringLiteral("教学周超出学期范围"));
            break;
        case GetWeeklyScheduleError::StorageFailure:
            setError(QStringLiteral("无法读取课表数据"));
            break;
        }
        m_meetings.clear();
        emit scheduleChanged();
        return;
    }

    m_meetings.replaceItems(
        std::get<WeeklySchedule>(result).items);
    clearError();
    emit scheduleChanged();
}

void ScheduleViewModel::clearSelection()
{
    const bool hadTerm = m_selectedTerm.has_value();
    const bool hadWeek = m_selectedWeek != 0;
    m_selectedTerm.reset();
    m_selectedWeek = 0;
    m_meetings.clear();
    if (hadTerm) {
        emit selectedTermChanged();
    }
    if (hadTerm || hadWeek) {
        emit selectedWeekChanged();
    }
    emit scheduleChanged();
}

void ScheduleViewModel::setLoading(bool loading)
{
    if (m_loading == loading) {
        return;
    }
    m_loading = loading;
    emit loadingChanged();
}

void ScheduleViewModel::setError(QString message)
{
    if (m_errorMessage == message) {
        return;
    }
    m_errorMessage = std::move(message);
    emit errorChanged();
}

void ScheduleViewModel::clearError()
{
    if (m_errorMessage.isEmpty()) {
        return;
    }
    m_errorMessage.clear();
    emit errorChanged();
}

} // namespace stukit::schedule::presentation
