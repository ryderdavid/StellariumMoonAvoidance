#include "MoonAvoidanceConfig.hpp"
#include <QStandardPaths>
#include <QDir>

MoonAvoidanceConfig::MoonAvoidanceConfig()
	: settings(nullptr)
{
	loadDefaults();
}

MoonAvoidanceConfig::~MoonAvoidanceConfig()
{
	if (settings)
	{
		delete settings;
	}
}

void MoonAvoidanceConfig::loadDefaults()
{
	filters = getDefaultFilters();
}

QList<FilterConfig> MoonAvoidanceConfig::getDefaultFilters()
{
	QList<FilterConfig> defaults;
	
	// LRGB - white
	defaults.append(FilterConfig("LRGB", 140.0, 14.0, 2.0, -15.0, 5.0, Qt::white));
	
	// O - cyan
	defaults.append(FilterConfig("O", 120.0, 10.0, 1.0, -15.0, 5.0, QColor(0, 255, 255)));
	
	// S - yellow
	defaults.append(FilterConfig("S", 45.0, 9.0, 1.0, -15.0, 5.0, Qt::yellow));
	
	// H - red
	defaults.append(FilterConfig("H", 35.0, 7.0, 1.0, -15.0, 5.0, Qt::red));
	
	return defaults;
}

void MoonAvoidanceConfig::loadConfiguration()
{
	// Use Stellarium's file manager if available, otherwise fall back to QStandardPaths
	QString pluginConfigPath;
	try {
		// Try to use Stellarium's file manager
		QString configDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
		pluginConfigPath = configDir + "/stellarium/plugins/MoonAvoidance.ini";
	}
	catch (...)
	{
		// Fallback to default location
		pluginConfigPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/stellarium/plugins/MoonAvoidance.ini";
	}
	
	if (settings)
	{
		delete settings;
		settings = nullptr;
	}
	
	settings = new QSettings(pluginConfigPath, QSettings::IniFormat);
	
	filters.clear();
	
	// Load filter groups
	QStringList groups = settings->childGroups();
	
	bool hasInvalidValues = false;
	
	for (const QString& group : groups)
	{
		settings->beginGroup(group);
		
		QString name = group;
		double separation = settings->value("Separation", 0.0).toDouble();
		double width = settings->value("Width", 0.0).toDouble();
		double relaxation = settings->value("Relaxation", 0.0).toDouble();
		double minAlt = settings->value("MinAlt", -15.0).toDouble();
		double maxAlt = settings->value("MaxAlt", 5.0).toDouble();
		
		// Validate values - check if they're all zeros or invalid
		if (separation == 0.0 && width == 0.0 && relaxation == 0.0)
		{
			hasInvalidValues = true;
			qWarning() << "MoonAvoidanceConfig: Filter" << name << "has all zero values, will reset to defaults";
		}
		
		// Load color
		QColor color(Qt::white);
		if (settings->contains("Color"))
		{
			color = settings->value("Color").value<QColor>();
		}
		else
		{
			// Use default colors based on filter name
			if (name == "LRGB") color = Qt::white;
			else if (name == "H") color = Qt::red;
			else if (name == "O") color = QColor(0, 255, 255); // cyan
			else if (name == "S") color = Qt::yellow;
		}
		
		filters.append(FilterConfig(name, separation, width, relaxation, minAlt, maxAlt, color));
		
		settings->endGroup();
	}
	
	// If no filters loaded or all values are invalid, use defaults
	if (filters.isEmpty() || hasInvalidValues)
	{
		qWarning() << "MoonAvoidanceConfig: No valid filters found or invalid values detected, loading defaults";
		loadDefaults();
		// Save defaults to config file
		saveConfiguration();
	}
}

void MoonAvoidanceConfig::saveConfiguration()
{
	if (!settings)
	{
		QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
		QString pluginConfigPath = configPath + "/stellarium/plugins/MoonAvoidance.ini";
		settings = new QSettings(pluginConfigPath, QSettings::IniFormat);
	}
	
	settings->clear();
	
	for (const FilterConfig& filter : filters)
	{
		settings->beginGroup(filter.name);
		settings->setValue("Separation", filter.separation);
		settings->setValue("Width", filter.width);
		settings->setValue("Relaxation", filter.relaxation);
		settings->setValue("MinAlt", filter.minAlt);
		settings->setValue("MaxAlt", filter.maxAlt);
		settings->setValue("Color", filter.color);
		settings->endGroup();
	}
	
	settings->sync();
}

void MoonAvoidanceConfig::addFilter(const FilterConfig& filter)
{
	filters.append(filter);
}

void MoonAvoidanceConfig::removeFilter(int index)
{
	if (index >= 0 && index < filters.size())
	{
		filters.removeAt(index);
	}
}

void MoonAvoidanceConfig::updateFilter(int index, const FilterConfig& filter)
{
	if (index >= 0 && index < filters.size())
	{
		filters[index] = filter;
	}
}

