#pragma once

#include "GetAcademicTerms.h"
#include "GetWeeklySchedule.h"
#include "WeeklyScheduleListModel.h"

#include <QAbstractItemModel>
#include <QDate>
#include <QObject>
#include <QString>
#include <QVariantList>

#include <optional>
#include <vector>

namespace stukit::schedule::presentation {

class ScheduleViewModel final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList termOptions READ termOptions NOTIFY termsChanged)
    Q_PROPERTY(bool hasTerms READ hasTerms NOTIFY termsChanged)
    Q_PROPERTY(QString selectedTermId READ selectedTermId NOTIFY selectedTermChanged)
    Q_PROPERTY(QString selectedTermName READ selectedTermName NOTIFY selectedTermChanged)
    Q_PROPERTY(int selectedWeek READ selectedWeek NOTIFY selectedWeekChanged)
    Q_PROPERTY(int totalWeeks READ totalWeeks NOTIFY selectedTermChanged)
    Q_PROPERTY(QString weekLabel READ weekLabel NOTIFY selectedWeekChanged)
    Q_PROPERTY(QString weekDateRange READ weekDateRange NOTIFY selectedWeekChanged)
    Q_PROPERTY(QVariantList weekDays READ weekDays NOTIFY selectedWeekChanged)
    Q_PROPERTY(bool canShowPreviousWeek READ canShowPreviousWeek NOTIFY selectedWeekChanged)
    Q_PROPERTY(bool canShowNextWeek READ canShowNextWeek NOTIFY selectedWeekChanged)
    Q_PROPERTY(bool currentWeekAvailable READ currentWeekAvailable NOTIFY selectedTermChanged)
    Q_PROPERTY(bool showingCurrentWeek READ showingCurrentWeek NOTIFY selectedWeekChanged)
    Q_PROPERTY(bool empty READ isEmpty NOTIFY scheduleChanged)
    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QAbstractItemModel *meetings READ meetings CONSTANT)

public:
    ScheduleViewModel(
        GetAcademicTerms &getAcademicTerms,
        GetWeeklySchedule &getWeeklySchedule,
        QDate today,
        QUuid preferredTermId = {},
        QObject *parent = nullptr);

    [[nodiscard]] QVariantList termOptions() const;
    [[nodiscard]] bool hasTerms() const noexcept;
    [[nodiscard]] QString selectedTermId() const;
    [[nodiscard]] QString selectedTermName() const;
    [[nodiscard]] int selectedWeek() const noexcept;
    [[nodiscard]] int totalWeeks() const noexcept;
    [[nodiscard]] QString weekLabel() const;
    [[nodiscard]] QString weekDateRange() const;
    [[nodiscard]] QVariantList weekDays() const;
    [[nodiscard]] bool canShowPreviousWeek() const noexcept;
    [[nodiscard]] bool canShowNextWeek() const noexcept;
    [[nodiscard]] bool currentWeekAvailable() const noexcept;
    [[nodiscard]] bool showingCurrentWeek() const noexcept;
    [[nodiscard]] bool isEmpty() const noexcept;
    [[nodiscard]] bool isLoading() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] QAbstractItemModel *meetings() noexcept;

    Q_INVOKABLE void load();
    Q_INVOKABLE bool selectTerm(const QString &termId);
    Q_INVOKABLE void showPreviousWeek();
    Q_INVOKABLE void showNextWeek();
    Q_INVOKABLE bool showCurrentWeek();

signals:
    void termsChanged();
    void selectedTermChanged();
    void selectedWeekChanged();
    void scheduleChanged();
    void loadingChanged();
    void errorChanged();

private:
    [[nodiscard]] int currentWeekFor(
        const AcademicTerm &term) const noexcept;
    [[nodiscard]] std::optional<AcademicTerm> chooseTerm(
        const QUuid &previousTermId) const;
    void applySelectedTerm(AcademicTerm term, int preferredWeek = 0);
    void loadSchedule();
    void clearSelection();
    void setLoading(bool loading);
    void setError(QString message);
    void clearError();

    GetAcademicTerms &m_getAcademicTerms;
    GetWeeklySchedule &m_getWeeklySchedule;
    QDate m_today;
    QUuid m_preferredTermId;
    std::vector<AcademicTerm> m_terms;
    std::optional<AcademicTerm> m_selectedTerm;
    int m_selectedWeek{0};
    WeeklyScheduleListModel m_meetings;
    bool m_loading{false};
    QString m_errorMessage;
};

} // namespace stukit::schedule::presentation
