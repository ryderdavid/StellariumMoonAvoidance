#include "MoonAvoidance.hpp"
#include "MoonAvoidanceDialog.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelPainter.hpp"
#include "StelProjector.hpp"
#include "StelUtils.hpp"
#include "StelModuleMgr.hpp"
#include "SolarSystem.hpp"
#include "Planet.hpp"
#include "StelSkyDrawer.hpp"
#include "VecMath.hpp"
#include <QSettings>
#include <QDebug>
#include <cmath>

MoonAvoidance::MoonAvoidance()
	: config(nullptr)
	, configDialog(nullptr)
	, enabled(false)
	, lastMoonAltitude(0.0)
	, lastMoonPhase(0.0)
{
	setObjectName("MoonAvoidance");
}

MoonAvoidance::~MoonAvoidance()
{
	if (config)
	{
		delete config;
	}
	if (configDialog)
	{
		delete configDialog;
	}
}

void MoonAvoidance::init()
{
	// Initialize config first with defaults
	config = new MoonAvoidanceConfig();
	
	// Load enabled state
	enabled = false;
	try {
		QSettings* conf = StelApp::getInstance().getSettings();
		if (conf)
		{
			enabled = conf->value("MoonAvoidance/enabled", false).toBool();
		}
	}
	catch (...)
	{
		// If settings access fails, default to disabled
		enabled = false;
	}
	
	flagShow = LinearFader(enabled);
	
	// Load configuration - this should be safe as it uses defaults if loading fails
	try {
		config->loadConfiguration();
	}
	catch (...)
	{
		// If config loading fails, use defaults
		config->setFilters(MoonAvoidanceConfig::getDefaultFilters());
	}
	
	qDebug() << "MoonAvoidance plugin initialized";
}

void MoonAvoidance::update(double deltaTime)
{
	flagShow.update(static_cast<int>(deltaTime * 1000));
	
	if (!flagShow.getInterstate())
		return;
	
	if (!config)
		return;
	
	// Update moon data
	try {
		StelCore* core = StelApp::getInstance().getCore();
		if (!core)
			return;
		
		SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
		if (!ssystem)
			return;
		
		PlanetP moonP = ssystem->searchByEnglishName("Moon");
		
		if (moonP)
		{
			Planet* moon = moonP.data();
			if (!moon)
				return;
			
			// Calculate moon altitude
			Vec3d altAzPos = moon->getAltAzPosAuto(core);
			double alt, az;
			StelUtils::rectToSphe(&az, &alt, altAzPos);
			lastMoonAltitude = alt * 180.0 / M_PI; // Convert to degrees
			
			// Calculate moon phase angle (0 = new moon, 180 = full moon)
			// getPhaseAngle requires observer position
			Vec3d obsPos = core->getObserverHeliocentricEclipticPos();
			double phaseAngle = moon->getPhaseAngle(obsPos) * 180.0 / M_PI; // Convert to degrees
			
			// Convert phase angle to phase factor (0.0 = new moon, 1.0 = full moon)
			// Phase angle: 0° = new moon, 180° = full moon
			lastMoonPhase = (1.0 + cos(phaseAngle * M_PI / 180.0)) / 2.0;
		}
	}
	catch (...)
	{
		// Silently handle errors during update
	}
}

void MoonAvoidance::draw(StelCore* core)
{
	if (!flagShow.getInterstate())
		return;
	
	if (!config)
		return;
	
	StelPainter painter(core->getProjection(StelCore::FrameJ2000));
	
	// Get moon position
	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	if (!ssystem)
		return;
	
	PlanetP moonP = ssystem->searchByEnglishName("Moon");
	
	if (!moonP)
		return;
	
	Planet* moon = moonP.data();
	
	// Check if moon is visible
	Vec3d altAzPos = moon->getAltAzPosAuto(core);
	if (altAzPos.norm() < 0.1)
		return;
	
	Vec3d moonPos = moon->getJ2000EquatorialPos(core);
	moonPos.normalize();
	
	// Draw circles for each filter
	QList<FilterConfig> filters = config->getFilters();
	
	for (const FilterConfig& filter : filters)
	{
		// Check if moon altitude is within valid range
		if (lastMoonAltitude < filter.minAlt || lastMoonAltitude > filter.maxAlt)
			continue;
		
		// Calculate circle radius
		double radius = calculateCircleRadius(filter, lastMoonAltitude, lastMoonPhase);
		
		// Only draw if radius is valid
		if (radius > 0.0)
		{
			// Draw circle
			drawCircle(painter, moonPos, radius, filter.color);
		}
	}
}

double MoonAvoidance::getCallOrder(StelModuleActionName actionName) const
{
	if (actionName == StelModule::ActionDraw)
		return StelApp::getInstance().getModuleMgr().getModule("SolarSystem")->getCallOrder(actionName) + 1;
	return 0;
}

double MoonAvoidance::calculateSeparation(const FilterConfig& filter, double moonAltitude) const
{
	// Separation = Separation + Relaxation * (moonAltitude - MaxAlt)
	return filter.separation + filter.relaxation * (moonAltitude - filter.maxAlt);
}

double MoonAvoidance::calculateWidth(const FilterConfig& filter, double moonAltitude) const
{
	// Width = Width * ((moonAltitude - MinAlt) / (MaxAlt - MinAlt))
	double denominator = filter.maxAlt - filter.minAlt;
	if (denominator == 0.0)
		return filter.width;
	
	double ratio = (moonAltitude - filter.minAlt) / denominator;
	return filter.width * ratio;
}

double MoonAvoidance::calculateCircleRadius(const FilterConfig& filter, double moonAltitude, double moonPhase) const
{
	// Calculate adjusted separation based on altitude
	double adjustedSeparation = calculateSeparation(filter, moonAltitude);
	
	// At full moon (phase = 1.0), use full separation
	// At new moon (phase = 0.0), use minimal separation
	// The phase factor scales the separation based on moon illumination
	// For full moon, phaseFactor = 1.0; for new moon, phaseFactor approaches 0
	double phaseFactor = moonPhase; // Direct use: 0.0 = new moon, 1.0 = full moon
	
	// The radius is the separation value in degrees
	// At full moon, use full separation; at new moon, use a fraction
	double radius = adjustedSeparation * phaseFactor;
	
	// Ensure minimum radius
	if (radius < 0.0)
		radius = 0.0;
	
	return radius * M_PI / 180.0; // Convert degrees to radians
}

void MoonAvoidance::drawCircle(StelPainter& painter, const Vec3d& moonPos, double radius, const QColor& color) const
{
	// Draw a circle around the moon position
	// The radius is in radians (angular separation on the sphere)
	
	// Convert QColor to Vec3f for StelPainter
	Vec3f colorVec(color.redF(), color.greenF(), color.blueF());
	painter.setColor(colorVec, color.alphaF());
	painter.setLineSmooth(true);
	
	// Normalize moon position
	Vec3d moonPosNorm = moonPos;
	moonPosNorm.normalize();
	
	// Calculate two perpendicular vectors to moon position
	// These will be used to construct the circle
	Vec3d north(0, 0, 1);
	Vec3d east(1, 0, 0);
	
	// Find a vector perpendicular to moon position
	// Use ^ operator for cross product
	Vec3d perp1 = moonPosNorm ^ north;
	if (perp1.norm() < 0.1)
	{
		perp1 = moonPosNorm ^ east;
	}
	perp1.normalize();
	
	// Find second perpendicular vector
	Vec3d perp2 = moonPosNorm ^ perp1;
	perp2.normalize();
	
	// Draw circle using small line segments
	const int segments = 64;
	const double angleStep = 2.0 * M_PI / segments;
	
	Vec3d prevPoint;
	bool first = true;
	
	for (int i = 0; i <= segments; ++i)
	{
		double angle = i * angleStep;
		
		// Calculate point on circle using spherical geometry
		// Circle is at angular distance 'radius' from moon position
		Vec3d point = moonPosNorm * cos(radius) + 
		              (perp1 * cos(angle) + perp2 * sin(angle)) * sin(radius);
		point.normalize();
		
		if (!first)
		{
			painter.drawGreatCircleArc(prevPoint, point);
		}
		
		prevPoint = point;
		first = false;
	}
}

void MoonAvoidance::loadConfiguration()
{
	if (config)
	{
		config->loadConfiguration();
	}
}

void MoonAvoidance::saveConfiguration()
{
	if (config)
	{
		config->saveConfiguration();
	}
	
	QSettings* conf = StelApp::getInstance().getSettings();
	conf->setValue("MoonAvoidance/enabled", enabled);
}

bool MoonAvoidance::configureGui(bool show)
{
	if (show)
	{
		showConfigurationDialog();
		return true; // We have a configuration GUI
	}
	return true; // We have a configuration GUI
}

void MoonAvoidance::showConfigurationDialog()
{
	// Safety checks
	if (!config)
	{
		qWarning() << "MoonAvoidance: Cannot show configuration dialog - config is null";
		return;
	}
	
	// Get current filters safely before creating dialog
	QList<FilterConfig> filters;
	try {
		filters = config->getFilters();
	}
	catch (...)
	{
		qWarning() << "MoonAvoidance: Error getting filters, using defaults";
		filters = MoonAvoidanceConfig::getDefaultFilters();
	}
	
	// Create dialog on the stack to ensure proper cleanup
	MoonAvoidanceDialog dialog(nullptr);
	
	try {
		// Set filters before showing
		dialog.setFilters(filters);
		
		// Show dialog modally
		dialog.setModal(true);
		
		// Execute dialog
		if (dialog.exec() == QDialog::Accepted)
		{
			QList<FilterConfig> newFilters = dialog.getFilters();
			config->setFilters(newFilters);
			config->saveConfiguration();
		}
	}
	catch (const std::exception& e)
	{
		qWarning() << "MoonAvoidance: Exception in showConfigurationDialog:" << e.what();
	}
	catch (...)
	{
		qWarning() << "MoonAvoidance: Unknown exception in showConfigurationDialog";
	}
}

void MoonAvoidance::setEnabled(bool b)
{
	if (b != enabled)
	{
		enabled = b;
		flagShow = LinearFader(enabled);
		saveConfiguration();
	}
}
