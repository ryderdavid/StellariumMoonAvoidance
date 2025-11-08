#include <QtTest/QtTest>
#include "../MoonAvoidanceConfig.hpp"

class TestMoonAvoidanceConfig : public QObject
{
	Q_OBJECT

private slots:
	void testDefaultFilters();
	void testAddRemoveFilter();
	void testUpdateFilter();
};

void TestMoonAvoidanceConfig::testDefaultFilters()
{
	QList<FilterConfig> defaults = MoonAvoidanceConfig::getDefaultFilters();
	
	QCOMPARE(defaults.size(), 4);
	QCOMPARE(defaults[0].name, QString("LRGB"));
	QCOMPARE(defaults[1].name, QString("O"));
	QCOMPARE(defaults[2].name, QString("S"));
	QCOMPARE(defaults[3].name, QString("H"));
}

void TestMoonAvoidanceConfig::testAddRemoveFilter()
{
	MoonAvoidanceConfig config;
	
	QList<FilterConfig> filters = config.getFilters();
	int initialSize = filters.size();
	
	FilterConfig newFilter("TestFilter", 50.0, 5.0, 1.0, -15.0, 5.0, Qt::blue);
	config.addFilter(newFilter);
	
	filters = config.getFilters();
	QCOMPARE(filters.size(), initialSize + 1);
	QCOMPARE(filters.last().name, QString("TestFilter"));
	
	config.removeFilter(filters.size() - 1);
	filters = config.getFilters();
	QCOMPARE(filters.size(), initialSize);
}

void TestMoonAvoidanceConfig::testUpdateFilter()
{
	MoonAvoidanceConfig config;
	
	QList<FilterConfig> filters = config.getFilters();
	if (filters.size() > 0)
	{
		FilterConfig updated = filters[0];
		updated.separation = 200.0;
		
		config.updateFilter(0, updated);
		
		filters = config.getFilters();
		QCOMPARE(filters[0].separation, 200.0);
	}
}

QTEST_MAIN(TestMoonAvoidanceConfig)
#include "testMoonAvoidanceConfig.moc"

