#pragma once

#include "ChangeTaskStatus.h"
#include "DeleteTask.h"
#include "GetTaskOverview.h"
#include "ICourseLookup.h"
#include "ITaskCategoryStore.h"
#include "SaveTask.h"
#include "ScanTaskReminders.h"

#include <QObject>
#include <QTimeZone>
#include <QVariantList>

namespace stukit::tasks::presentation {

class TasksViewModel final : public QObject, public ITaskReminderNotifier
{
    Q_OBJECT
    Q_PROPERTY(QVariantList tasks READ tasks NOTIFY dataChanged)
    Q_PROPERTY(QVariantList courseOptions READ courseOptions NOTIFY dataChanged)
    Q_PROPERTY(QStringList categoryOptions READ categoryOptions NOTIFY dataChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY dataChanged)
    Q_PROPERTY(int completedCount READ completedCount NOTIFY dataChanged)
    Q_PROPERTY(int overdueCount READ overdueCount NOTIFY dataChanged)
    Q_PROPERTY(QString completionRateText READ completionRateText NOTIFY dataChanged)
    Q_PROPERTY(bool includeArchived READ includeArchived NOTIFY filterChanged)
    Q_PROPERTY(int categoryFilter READ categoryFilter NOTIFY filterChanged)
    Q_PROPERTY(QString keyword READ keyword NOTIFY filterChanged)
    Q_PROPERTY(bool hasError READ hasError NOTIFY errorChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorChanged)
    Q_PROPERTY(QVariantList reminders READ reminders NOTIFY remindersChanged)

public:
    TasksViewModel(
        SaveTask &saveTask,
        ChangeTaskStatus &changeStatus,
        DeleteTask &deleteTask,
        GetTaskOverview &getOverview,
        ICourseLookup &courseLookup,
        ITaskCategoryStore &categoryStore,
        QTimeZone displayTimeZone,
        QObject *parent = nullptr);

    [[nodiscard]] QVariantList tasks() const;
    [[nodiscard]] QVariantList courseOptions() const;
    [[nodiscard]] QStringList categoryOptions() const;
    [[nodiscard]] int pendingCount() const noexcept;
    [[nodiscard]] int completedCount() const noexcept;
    [[nodiscard]] int overdueCount() const noexcept;
    [[nodiscard]] QString completionRateText() const;
    [[nodiscard]] bool includeArchived() const noexcept;
    [[nodiscard]] int categoryFilter() const noexcept;
    [[nodiscard]] const QString &keyword() const noexcept;
    [[nodiscard]] bool hasError() const noexcept;
    [[nodiscard]] const QString &errorMessage() const noexcept;
    [[nodiscard]] QVariantList reminders() const;

    void setReminderScanner(ScanTaskReminders *scanner);
    [[nodiscard]] bool deliver(const TaskReminder &reminder) override;

    Q_INVOKABLE void load();
    Q_INVOKABLE void setIncludeArchived(bool include);
    Q_INVOKABLE void setCategoryFilter(int category);
    Q_INVOKABLE void setKeyword(const QString &keyword);
    Q_INVOKABLE bool addCategory(const QString &name);
    Q_INVOKABLE bool save(
        const QString &id,
        const QString &title,
        const QString &description,
        const QString &dueLocalText,
        const QString &reminderLocalText,
        int priority,
        const QString &categoryName,
        const QString &courseId);
    Q_INVOKABLE bool complete(const QString &id);
    Q_INVOKABLE bool reopen(const QString &id);
    Q_INVOKABLE bool archive(const QString &id);
    Q_INVOKABLE bool restore(const QString &id);
    Q_INVOKABLE bool remove(const QString &id);
    Q_INVOKABLE void dismissReminder(int index);
    Q_INVOKABLE void scanReminders();

signals:
    void dataChanged();
    void filterChanged();
    void errorChanged();
    void remindersChanged();

private:
    [[nodiscard]] std::optional<QDateTime> parseLocalTime(
        const QString &text,
        bool &valid) const;
    [[nodiscard]] bool change(const QString &id, TaskStatusAction action);
    void setError(QString message);
    void clearError();

    SaveTask &m_saveTask;
    ChangeTaskStatus &m_changeStatus;
    DeleteTask &m_deleteTask;
    GetTaskOverview &m_getOverview;
    ICourseLookup &m_courseLookup;
    ITaskCategoryStore &m_categoryStore;
    QTimeZone m_displayTimeZone;
    TaskOverview m_overview;
    std::vector<CourseReference> m_courses;
    QStringList m_categories;
    bool m_includeArchived{false};
    int m_categoryFilter{-1};
    QString m_keyword;
    QString m_errorMessage;
    ScanTaskReminders *m_reminderScanner{nullptr};
    QVariantList m_reminders;
};

} // namespace stukit::tasks::presentation
