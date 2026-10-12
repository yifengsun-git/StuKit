#include "GetAcademicTerms.h"
#include "support/InMemoryAcademicTermRepository.h"

#include <QTest>

#include <variant>

using namespace stukit::schedule;
using namespace stukit::schedule::testing;

namespace {

AcademicTerm makeTerm(
    const QString &name, const QDate &startDate, bool archived)
{
    return AcademicTerm::create(AcademicTermData{
        .id = QUuid::createUuid(),
        .name = name,
        .startDate = startDate,
        .totalWeeks = 18,
        .archived = archived,
    }).value();
}

} // namespace

class GetAcademicTermsTest final : public QObject
{
    Q_OBJECT

private slots:
    void returnsEmptyList();
    void sortsActiveAndArchivedTerms();
    void reportsStorageFailure();
};

void GetAcademicTermsTest::returnsEmptyList()
{
    InMemoryAcademicTermRepository terms;
    GetAcademicTerms useCase(terms);

    const auto result = useCase.execute();

    QVERIFY(std::holds_alternative<std::vector<AcademicTerm>>(result));
    QVERIFY(std::get<std::vector<AcademicTerm>>(result).empty());
}

void GetAcademicTermsTest::sortsActiveAndArchivedTerms()
{
    InMemoryAcademicTermRepository terms;
    QVERIFY(terms.save(makeTerm(
        QStringLiteral("旧学期"), QDate(2025, 9, 1), true)));
    QVERIFY(terms.save(makeTerm(
        QStringLiteral("春季学期"), QDate(2026, 2, 23), false)));
    QVERIFY(terms.save(makeTerm(
        QStringLiteral("秋季学期"), QDate(2026, 9, 7), false)));
    GetAcademicTerms useCase(terms);

    const auto result = useCase.execute();

    QVERIFY(std::holds_alternative<std::vector<AcademicTerm>>(result));
    const auto &items = std::get<std::vector<AcademicTerm>>(result);
    QCOMPARE(items.size(), std::size_t{3});
    QCOMPARE(items.at(0).name(), QStringLiteral("秋季学期"));
    QCOMPARE(items.at(1).name(), QStringLiteral("春季学期"));
    QCOMPARE(items.at(2).name(), QStringLiteral("旧学期"));
}

void GetAcademicTermsTest::reportsStorageFailure()
{
    InMemoryAcademicTermRepository terms;
    terms.setQueryShouldSucceed(false);
    GetAcademicTerms useCase(terms);

    const auto result = useCase.execute();

    QCOMPARE(
        std::get<GetAcademicTermsError>(result),
        GetAcademicTermsError::StorageFailure);
}

QTEST_APPLESS_MAIN(GetAcademicTermsTest)

#include "GetAcademicTermsTest.moc"
