#include <QtTest/QtTest>
#include <cmath>
#include "../MoonAvoidanceConfig.hpp"

// Test helper functions that mirror the calculation logic
namespace MoonAvoidanceTestHelpers
{
	double calculateSeparation(const FilterConfig& filter, double moonAltitude)
	{
		return filter.separation + filter.relaxation * (moonAltitude - filter.maxAlt);
	}
	
	double calculateWidth(const FilterConfig& filter, double moonAltitude)
	{
		double denominator = filter.maxAlt - filter.minAlt;
		if (denominator == 0.0)
			return filter.width;
		
		double ratio = (moonAltitude - filter.minAlt) / denominator;
		return filter.width * ratio;
	}
	
	double calculateCircleRadius(const FilterConfig& filter, double moonAltitude, double moonPhase)
	{
		double adjustedSeparation = calculateSeparation(filter, moonAltitude);
		double phaseFactor = moonPhase;
		double radius = adjustedSeparation * phaseFactor;
		
		if (radius < 0.0)
			radius = 0.0;
		
		return radius * M_PI / 180.0; // Convert degrees to radians
	}
}

class TestMoonAvoidance : public QObject
{
	Q_OBJECT

private slots:
	void testSeparationCalculation();
	void testWidthCalculation();
	void testCircleRadiusCalculation();
	void testDefaultFilters();
};

void TestMoonAvoidance::testSeparationCalculation()
{
	FilterConfig filter("Test", 100.0, 10.0, 2.0, -15.0, 5.0, Qt::white);
	
	// Test at max altitude
	double separation = MoonAvoidanceTestHelpers::calculateSeparation(filter, 5.0);
	QCOMPARE(separation, 100.0); // Base separation
	
	// Test below max altitude
	separation = MoonAvoidanceTestHelpers::calculateSeparation(filter, 0.0);
	QCOMPARE(separation, 90.0); // 100 + 2 * (0 - 5) = 90
}

void TestMoonAvoidance::testWidthCalculation()
{
	FilterConfig filter("Test", 100.0, 10.0, 2.0, -15.0, 5.0, Qt::white);
	
	// Test at min altitude
	double width = MoonAvoidanceTestHelpers::calculateWidth(filter, -15.0);
	QCOMPARE(width, 0.0); // Width * ((-15 - (-15)) / (5 - (-15))) = 0
	
	// Test at max altitude
	width = MoonAvoidanceTestHelpers::calculateWidth(filter, 5.0);
	QCOMPARE(width, 10.0); // Width * ((5 - (-15)) / (5 - (-15))) = 10
}

void TestMoonAvoidance::testCircleRadiusCalculation()
{
	FilterConfig filter("Test", 100.0, 10.0, 2.0, -15.0, 5.0, Qt::white);
	
	// Test at full moon (phase = 1.0)
	double radius = MoonAvoidanceTestHelpers::calculateCircleRadius(filter, 5.0, 1.0);
	QVERIFY(radius > 0.0);
	
	// Test at new moon (phase = 0.0)
	radius = MoonAvoidanceTestHelpers::calculateCircleRadius(filter, 5.0, 0.0);
	QVERIFY(radius >= 0.0);
}

void TestMoonAvoidance::testDefaultFilters()
{
	QList<FilterConfig> defaults = MoonAvoidanceConfig::getDefaultFilters();
	
	QCOMPARE(defaults.size(), 4);
	
	// Check LRGB filter
	QCOMPARE(defaults[0].name, QString("LRGB"));
	QCOMPARE(defaults[0].separation, 140.0);
	QCOMPARE(defaults[0].color, QColor(Qt::white));
	
	// Check H filter
	QCOMPARE(defaults[3].name, QString("H"));
	QCOMPARE(defaults[3].separation, 35.0);
	QCOMPARE(defaults[3].color, QColor(Qt::red));
}

QTEST_MAIN(TestMoonAvoidance)
#include "testMoonAvoidance.moc"

