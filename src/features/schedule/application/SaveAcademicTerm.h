#pragma once

#include "IAcademicTermRepository.h"
#include "ICourseMeetingRepository.h"
#include "ICourseRepository.h"

#include <QDate>
#include <QString>
#include <QUuid>

#include <variant>

namespace stukit::schedule {

struct SaveAcademicTermRequest
{
    QUuid id;
    QString name;
    QDate startDate;
    int totalWeeks{0};
    bool archived{false};
};

enum class SaveAcademicTermError
{
    InvalidInput,
    NotFound,
    HasOutOfRangeMeetings,
    StorageFailure
};

using SaveAcademicTermResult = std::variant<
    AcademicTerm, SaveAcademicTermError>;

class SaveAcademicTerm final
{
public:
    SaveAcademicTerm(
        IAcademicTermRepository &termRepository,
        ICourseRepository &courseRepository,
        ICourseMeetingRepository &meetingRepository);

    [[nodiscard]] SaveAcademicTermResult execute(
        const SaveAcademicTermRequest &request) const;

private:
    IAcademicTermRepository &m_termRepository;
    ICourseRepository &m_courseRepository;
    ICourseMeetingRepository &m_meetingRepository;
};

} // namespace stukit::schedule
