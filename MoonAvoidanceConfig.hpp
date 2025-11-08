#ifndef MOONAVOIDANCECONFIG_HPP
#define MOONAVOIDANCECONFIG_HPP

#include <QString>
#include <QColor>
#include <QSettings>
#include <QList>

struct FilterConfig
{
	QString name;
	double separation;
	double width;
	double relaxation;
	double minAlt;
	double maxAlt;
	QColor color;
	
	FilterConfig()
		: separation(0.0)
		, width(0.0)
		, relaxation(0.0)
		, minAlt(0.0)
		, maxAlt(0.0)
		, color(Qt::white)
	{}
	
	FilterConfig(const QString& n, double sep, double w, double rel, double min, double max, const QColor& c)
		: name(n)
		, separation(sep)
		, width(w)
		, relaxation(rel)
		, minAlt(min)
		, maxAlt(max)
		, color(c)
	{}
};

class MoonAvoidanceConfig
{
public:
	MoonAvoidanceConfig();
	~MoonAvoidanceConfig();
	
	void loadConfiguration();
	void saveConfiguration();
	
	QList<FilterConfig> getFilters() const { return filters; }
	void setFilters(const QList<FilterConfig>& f) { filters = f; }
	
	void addFilter(const FilterConfig& filter);
	void removeFilter(int index);
	void updateFilter(int index, const FilterConfig& filter);
	
	static QList<FilterConfig> getDefaultFilters();

private:
	QList<FilterConfig> filters;
	QSettings* settings;
	
	void loadDefaults();
};

#endif // MOONAVOIDANCECONFIG_HPP

