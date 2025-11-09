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
#include <QApplication>
#include <QMetaObject>
#include <QThread>
#include <QDebug>
#include <QtGlobal> // For qMax, qMin, qBound
#include <cmath>

MoonAvoidance::MoonAvoidance()
	: config(nullptr)
	, configDialog(new MoonAvoidanceDialog())
	, enabled(false)
	, lastMoonAltitude(0.0)
	, lastMoonAgeDays(0.0)
	, lastMoonAgeFromFullDays(0.0)
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
	
	// Load enabled state - default to enabled
	enabled = true;
	try {
		QSettings* conf = StelApp::getInstance().getSettings();
		if (conf)
		{
			enabled = conf->value("MoonAvoidance/enabled", true).toBool();
		}
	}
	catch (...)
	{
		// If settings access fails, default to enabled
		enabled = true;
	}
	
	flagShow = LinearFader(1000, enabled); // duration=1000ms, initialState=enabled
	
	// Load configuration - this should be safe as it uses defaults if loading fails
	try {
		config->loadConfiguration();
		
		// Validate that we have filters with valid values
		QList<FilterConfig> loadedFilters = config->getFilters();
		bool needsReset = false;
		
		if (loadedFilters.isEmpty())
		{
			needsReset = true;
			qWarning() << "MoonAvoidance: No filters loaded, resetting to defaults";
		}
		else
		{
			// Check if all filters have zero values (invalid config)
			bool allZero = true;
			for (const FilterConfig& filter : loadedFilters)
			{
				if (filter.separation != 0.0 || filter.width != 0.0 || filter.relaxation != 0.0)
				{
					allZero = false;
					break;
				}
			}
			
			if (allZero)
			{
				needsReset = true;
				qWarning() << "MoonAvoidance: All filter values are zero, resetting to defaults";
			}
		}
		
		if (needsReset)
		{
			config->setFilters(MoonAvoidanceConfig::getDefaultFilters());
			config->saveConfiguration();
			qDebug() << "MoonAvoidance: Reset to defaults and saved";
		}
	}
	catch (...)
	{
		// If config loading fails, use defaults and save them
		config->setFilters(MoonAvoidanceConfig::getDefaultFilters());
		config->saveConfiguration();
		qWarning() << "MoonAvoidance: Config load failed, using and saving defaults";
	}
	
	// Connect to dialog's visibleChanged signal to save when closed with OK
	// Do this after config is initialized
	if (configDialog)
	{
		connect(configDialog, &StelDialog::visibleChanged, this, [this](bool visible) {
			if (!visible && config && configDialog && configDialog->wasAccepted())
			{
				// Dialog was closed with OK - save configuration
				QList<FilterConfig> newFilters = configDialog->getFilters();
				if (!newFilters.isEmpty())
				{
					config->setFilters(newFilters);
					config->saveConfiguration();
					qDebug() << "MoonAvoidance: Configuration saved";
				}
			}
		});
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
			
			// Calculate moon age in days since last new moon (standard astronomical definition)
			// Moon age = days since last new moon (0 = new moon, ~14.77 = full moon, ~29.53 = next new moon)
			// For the Lorentzian formula, we need days from full moon, so we'll convert
			
			const double SYNODIC_PERIOD_DAYS = 29.530588853; // Moon synodic period in days
			const double HALF_SYNODIC_PERIOD = SYNODIC_PERIOD_DAYS / 2.0; // ~14.765 days
			
			// Get current Julian Day from Stellarium
			double currentJD = core->getJD();
			
			// Reference JD for a known new moon (from Stellarium's AstroCalcDialog.cpp)
			// This is an approximate JD for a new moon near J2000
			const double REFERENCE_NEW_MOON_JD = 2451550.09765;
			
			// Calculate how many synodic periods have passed since the reference new moon
			double periodsSinceRef = (currentJD - REFERENCE_NEW_MOON_JD) / SYNODIC_PERIOD_DAYS;
			
			// Find the JD of the most recent new moon before or at current time
			double lastNewMoonJD = REFERENCE_NEW_MOON_JD + floor(periodsSinceRef) * SYNODIC_PERIOD_DAYS;
			
			// Calculate moon age: days since last new moon (standard definition)
			double moonAgeSinceNewMoon = currentJD - lastNewMoonJD;
			
			// Ensure age is in range [0, SYNODIC_PERIOD_DAYS)
			while (moonAgeSinceNewMoon < 0.0)
				moonAgeSinceNewMoon += SYNODIC_PERIOD_DAYS;
			while (moonAgeSinceNewMoon >= SYNODIC_PERIOD_DAYS)
				moonAgeSinceNewMoon -= SYNODIC_PERIOD_DAYS;
			
			// Store moon age since new moon for display
			lastMoonAgeDays = moonAgeSinceNewMoon;
			
			// For the Lorentzian formula, we need days from full moon (0 = full moon)
			// Full moon occurs at ~14.765 days (half synodic period) after new moon
			// The formula uses AGE where 0 = full moon, and separation is highest at full moon
			// Convert moon age (days since new moon) to days from full moon:
			// - If age < 14.765: we're before full moon, so days from full = 14.765 - age
			// - If age >= 14.765: we're after full moon, so days from full = age - 14.765
			// But we want the absolute distance from full moon, so we use the minimum
			// Actually, the formula should give highest separation at full moon (AGE=0),
			// so we need to calculate days from the nearest full moon
			double daysFromFullMoon;
			if (moonAgeSinceNewMoon < HALF_SYNODIC_PERIOD)
			{
				// Before full moon: days from full = half period - age
				daysFromFullMoon = HALF_SYNODIC_PERIOD - moonAgeSinceNewMoon;
			}
			else
			{
				// After full moon: days from full = age - half period
				daysFromFullMoon = moonAgeSinceNewMoon - HALF_SYNODIC_PERIOD;
			}
			
			// Store days from full moon for the formula
			// At full moon (age = 14.765), daysFromFullMoon = 0 (highest separation)
			// At new moon (age = 0 or 29.53), daysFromFullMoon = 14.765 (lowest separation)
			lastMoonAgeFromFullDays = daysFromFullMoon;
		}
	}
	catch (...)
	{
		// Silently handle errors during update
	}
}

void MoonAvoidance::draw(StelCore* core)
{
	qDebug() << "MoonAvoidance: draw() entry - enabled:" << enabled << ", flagShow interstate:" << flagShow.getInterstate();
	
	if (!flagShow.getInterstate())
	{
		qDebug() << "MoonAvoidance: draw() skipped - flagShow not interstate";
		return;
	}
	
	if (!config)
	{
		qDebug() << "MoonAvoidance: draw() skipped - config is null";
		return;
	}
	
	// Get moon position
	SolarSystem* ssystem = GETSTELMODULE(SolarSystem);
	if (!ssystem)
		return;
	
	PlanetP moonP = ssystem->searchByEnglishName("Moon");
	
	if (!moonP)
		return;
	
	Planet* moon = moonP.data();
	
	// Calculate moon altitude for altitude-based filtering
	Vec3d altAzPos = moon->getAltAzPosAuto(core);
	double alt, az;
	StelUtils::rectToSphe(&az, &alt, altAzPos);
	double currentMoonAltitude = alt * 180.0 / M_PI; // Convert to degrees
	
	// Always update lastMoonAltitude from current calculation
	lastMoonAltitude = currentMoonAltitude;
	
	qDebug() << "MoonAvoidance: Moon altitude calculated:" << lastMoonAltitude << "degrees";
	
	// Get moon position in the same frame as the projection
	// Use the current frame to match the view
	Vec3d moonPos = moon->getJ2000EquatorialPos(core);
	moonPos.normalize();
	
	// Initialize painter with the same frame as the moon position
	StelPainter painter(core->getProjection(StelCore::FrameJ2000));
	
	// Enable blending for transparency support (like GridLinesMgr does)
	painter.setBlending(true);
	
	// Draw circles for each filter
	QList<FilterConfig> filters = config->getFilters();
	
	qDebug() << "MoonAvoidance: draw() called - moon altitude:" << lastMoonAltitude << "degrees, filters:" << filters.size();
	
	for (const FilterConfig& filter : filters)
	{
		// Altitude check: If moon is within [MinAlt, MaxAlt], relaxation applies
		// If moon is outside this range, use traditional avoidance (no relaxation)
		// Circles always draw regardless of altitude
		
		qDebug() << "MoonAvoidance: Processing filter" << filter.name 
		         << "- altitude:" << lastMoonAltitude << "degrees (relaxation range: [" << filter.minAlt << "," << filter.maxAlt << "])";
		
		// Calculate circle radius (use days from full moon for the formula)
		// The calculateCircleRadius function will handle relaxation based on altitude
		double radius = calculateCircleRadius(filter, lastMoonAltitude, lastMoonAgeFromFullDays);
		double radiusDegrees = radius * 180.0 / M_PI;
		
		qDebug() << "MoonAvoidance: Drawing circle for filter" << filter.name 
		         << "- radius:" << radiusDegrees << "degrees, moon age:" << lastMoonAgeDays << "days";
		
		// Only draw if radius is valid and reasonable
		if (radius > 0.0 && radius < M_PI) // Radius should be less than 180 degrees
		{
			// Draw circle
			drawCircle(painter, moonPos, radius, filter.color);
		}
		else
		{
			qDebug() << "MoonAvoidance: Invalid radius" << radius << "for filter" << filter.name;
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
	// If moon altitude is within [MinAlt, MaxAlt], apply relaxation adjustment
	// If moon altitude is outside this range, use base separation (no relaxation)
	if (moonAltitude >= filter.minAlt && moonAltitude <= filter.maxAlt)
	{
		// Within relaxation range: Separation = Separation + Relaxation * (moonAltitude - MaxAlt)
		return filter.separation + filter.relaxation * (moonAltitude - filter.maxAlt);
	}
	else
	{
		// Outside relaxation range: Use base separation (traditional avoidance)
		return filter.separation;
	}
}

double MoonAvoidance::calculateWidth(const FilterConfig& filter, double moonAltitude) const
{
	// If moon altitude is within [MinAlt, MaxAlt], apply width adjustment
	// If moon altitude is outside this range, use base width (no adjustment)
	if (moonAltitude >= filter.minAlt && moonAltitude <= filter.maxAlt)
	{
		// Within relaxation range: Width = Width * ((moonAltitude - MinAlt) / (MaxAlt - MinAlt))
		double denominator = filter.maxAlt - filter.minAlt;
		if (denominator == 0.0)
			return filter.width;
		
		double ratio = (moonAltitude - filter.minAlt) / denominator;
		return filter.width * ratio;
	}
	else
	{
		// Outside relaxation range: Use base width (traditional avoidance)
		return filter.width;
	}
}

double MoonAvoidance::calculateCircleRadius(const FilterConfig& filter, double moonAltitude, double moonAgeDays) const
{
	// Calculate adjusted separation based on altitude (in degrees)
	double adjustedSeparation = calculateSeparation(filter, moonAltitude);
	
	// Calculate adjusted width based on altitude (in days)
	double adjustedWidth = calculateWidth(filter, moonAltitude);
	
	// Apply the Moon Avoidance Lorentzian formula from the spreadsheet:
	// SEPARATION = DISTANCE / ( 1 + POWER( ( ( 0.5 - ( AGE / 29.5) ) / ( WIDTH / 29.5 ) ) , 2 ) )
	// Simplified: SEPARATION = DISTANCE / ( 1 + ( ( 0.5 - ( AGE / 29.5) ) / ( WIDTH / 29.5 ) )^2 )
	// Further simplified: SEPARATION = DISTANCE / ( 1 + ( ( 0.5 * 29.5 - AGE ) / WIDTH )^2 )
	// where:
	//   DISTANCE = adjustedSeparation (in degrees)
	//   WIDTH = adjustedWidth (in days)
	//   AGE = moonAgeDays (days from full moon, 0 = full moon)
	//   29.5 = moon synodic period in days
	
	const double SYNODIC_PERIOD = 29.5;
	
	// Avoid division by zero
	if (adjustedWidth <= 0.0)
		adjustedWidth = 1.0;
	
	// Calculate the normalized term: ( 0.5 - ( AGE / 29.5) ) / ( WIDTH / 29.5 )
	// This simplifies to: ( 0.5 * 29.5 - AGE ) / WIDTH
	// Where AGE is days from full moon (0 = full moon, highest separation)
	// The formula should give highest separation at full moon (AGE = 0)
	// At full moon (AGE = 0): numerator = 0.5 * 29.5 = 14.75, term = 14.75 / WIDTH
	// At new moon (AGE = 14.765): numerator = 0.5 * 29.5 - 14.765 ≈ 0, term ≈ 0
	// 
	// The formula SEPARATION = DISTANCE / ( 1 + term^2 ) gives:
	// - At full moon (AGE = 0): SEPARATION = DISTANCE / ( 1 + (14.75/WIDTH)^2 ) = smaller value
	// - At new moon (AGE = 14.765): SEPARATION = DISTANCE / ( 1 + 0^2 ) = DISTANCE (largest)
	//
	// This is backwards! The spreadsheet shows highest separation at full moon.
	// The formula must be inverted or AGE must be calculated differently.
	// Actually, looking at the spreadsheet, the formula might be:
	// SEPARATION = DISTANCE * ( 1 + term^2 ) / ( 1 + term^2 ) or similar
	// 
	// Let me check: if we use the absolute value of (0.5 - AGE/29.5), we get:
	// At full moon (AGE = 0): |0.5 - 0| = 0.5, term = 0.5 / (WIDTH/29.5) = 0.5 * 29.5 / WIDTH
	// At new moon (AGE = 14.765): |0.5 - 14.765/29.5| = |0.5 - 0.5| = 0, term = 0
	//
	// Actually, I think the issue is that the formula should be:
	// SEPARATION = DISTANCE * ( 1 + term^2 ) / ( 1 + term^2 ) where term = (0.5 - AGE/29.5) / (WIDTH/29.5)
	// But that simplifies to DISTANCE, which doesn't make sense.
	//
	// Let me reconsider: the spreadsheet formula might be:
	// SEPARATION = DISTANCE / ( 1 + ( (0.5 - AGE/29.5) / (WIDTH/29.5) )^2 )
	// At full moon (AGE = 0): SEPARATION = DISTANCE / ( 1 + (0.5 / (WIDTH/29.5))^2 ) = DISTANCE / (1 + large) = small
	// At new moon (AGE = 14.765): SEPARATION = DISTANCE / ( 1 + (0 / (WIDTH/29.5))^2 ) = DISTANCE / 1 = DISTANCE
	//
	// This is still backwards. The formula must be inverted. Let me try:
	// SEPARATION = DISTANCE * ( 1 + term^2 ) where term = (0.5 - AGE/29.5) / (WIDTH/29.5)
	// At full moon (AGE = 0): SEPARATION = DISTANCE * ( 1 + (0.5 / (WIDTH/29.5))^2 ) = DISTANCE * (1 + large) = large
	// At new moon (AGE = 14.765): SEPARATION = DISTANCE * ( 1 + 0^2 ) = DISTANCE
	//
	// This is also backwards - it gives larger separation at full moon, but the factor is > 1, so it's larger than DISTANCE.
	//
	// Actually, I think the correct interpretation is that the formula should give:
	// - Highest separation at full moon (AGE = 0)
	// - Lower separation as we move away from full moon
	//
	// So the formula should be something like:
	// SEPARATION = DISTANCE / ( 1 + (AGE / WIDTH)^2 )
	// At full moon (AGE = 0): SEPARATION = DISTANCE / 1 = DISTANCE (highest)
	// At new moon (AGE = 14.765): SEPARATION = DISTANCE / ( 1 + (14.765/WIDTH)^2 ) = smaller
	//
	// But the spreadsheet formula uses (0.5 - AGE/29.5) / (WIDTH/29.5), which is different.
	//
	// Let me try a different approach: maybe AGE should be the absolute distance from full moon,
	// and the formula should be: SEPARATION = DISTANCE / ( 1 + (AGE / WIDTH)^2 )
	// This would give highest separation at full moon (AGE = 0) and lower as we move away.
	
	// For now, let's use the formula as written in the spreadsheet, but we need to verify it gives
	// the correct behavior. The spreadsheet shows highest separation at full moon, so the formula
	// must be correct. Let me use it as-is and see if it works.
	// The spreadsheet formula gives highest separation at full moon, but the formula as written
	// gives highest at new moon. The correct formula should be:
	// SEPARATION = DISTANCE / ( 1 + (AGE / WIDTH)^2 )
	// where AGE is days from full moon (0 = full moon, 14.765 = new moon)
	// This gives:
	// - At full moon (AGE = 0): SEPARATION = DISTANCE / 1 = DISTANCE (highest)
	// - At new moon (AGE = 14.765): SEPARATION = DISTANCE / ( 1 + (14.765/WIDTH)^2 ) = smaller
	//
	// Use days from full moon directly in the formula
	double normalizedTerm = moonAgeDays / adjustedWidth;
	
	// Calculate the Lorentzian factor: 1 + (normalizedTerm)^2
	double lorentzianFactor = 1.0 + (normalizedTerm * normalizedTerm);
	
	// Calculate the radius in degrees
	// At full moon (AGE = 0): SEPARATION = DISTANCE / 1 = DISTANCE (highest)
	// At new moon (AGE = 14.765): SEPARATION = DISTANCE / ( 1 + (14.765/WIDTH)^2 ) = smaller
	double radiusDegrees = adjustedSeparation / lorentzianFactor;
	
	// Ensure minimum radius (at least 1 degree)
	if (radiusDegrees < 1.0)
		radiusDegrees = 1.0;
	
	// Convert degrees to radians for drawing
	return radiusDegrees * M_PI / 180.0;
}

void MoonAvoidance::drawCircle(StelPainter& painter, const Vec3d& moonPos, double radius, const QColor& color) const
{
	// Draw a circle around the moon position
	// The radius is in radians (angular separation on the sphere)
	// This is a "small circle" - a circle on the sphere at constant angular distance from a point
	
	// Convert QColor to Vec3f for StelPainter
	Vec3f colorVec(color.redF(), color.greenF(), color.blueF());
	painter.setColor(colorVec, 1.0f); // Use full opacity for visibility
	painter.setLineSmooth(true);
	painter.setLineWidth(4.0f); // Make lines very visible
	
	// Normalize moon position (center of circle)
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
		// If moon is near north pole, use east vector instead
		perp1 = moonPosNorm ^ east;
	}
	perp1.normalize();
	
	// Find second perpendicular vector (orthogonal to both moon position and perp1)
	Vec3d perp2 = moonPosNorm ^ perp1;
	perp2.normalize();
	
	// Draw circle using small circle arcs
	// For a small circle on a sphere, we use drawSmallCircleArc
	// Increase segments based on circle size - larger circles need more segments
	double radiusDegrees = radius * 180.0 / M_PI;
	int segments = qMax(256, static_cast<int>(radiusDegrees * 8)); // At least 256, more for larger circles
	segments = qMin(segments, 1024); // Cap at 1024 for performance
	const double angleStep = 2.0 * M_PI / segments;
	
	// Draw the circle by connecting consecutive points with small circle arcs
	// The rotation center is the moon position (center of the circle)
	Vec3d prevPoint;
	bool first = true;
	
	for (int i = 0; i <= segments; ++i)
	{
		double angle = i * angleStep;
		
		// Calculate point on small circle using spherical geometry
		// For a small circle at angular distance 'radius' from center:
		// point = center * cos(radius) + (perp1 * cos(angle) + perp2 * sin(angle)) * sin(radius)
		Vec3d point = moonPosNorm * cos(radius) + 
		              (perp1 * cos(angle) + perp2 * sin(angle)) * sin(radius);
		point.normalize();
		
		// Verify the point is at the correct angular distance from center
		double dotProduct = moonPosNorm * point;
		double actualAngle = acos(qBound(-1.0, dotProduct, 1.0));
		double angleError = fabs(actualAngle - radius);
		
		// If error is significant, recalculate (shouldn't happen, but safety check)
		if (angleError > 0.01) // More than 0.01 radians error
		{
			// Recalculate to ensure correct distance
			point = moonPosNorm * cos(radius) + 
			        (perp1 * cos(angle) + perp2 * sin(angle)) * sin(radius);
			point.normalize();
		}
		
		if (!first)
		{
			// Connect consecutive points with great circle arcs to form a smooth polygon
			// This creates a circle by connecting points on the small circle with great circle segments
			try {
				painter.drawGreatCircleArc(prevPoint, point, nullptr);
			}
			catch (...)
			{
				qWarning() << "MoonAvoidance: Error drawing great circle arc";
			}
		}
		
		prevPoint = point;
		first = false;
	}
	
	qDebug() << "MoonAvoidance: drawCircle() completed - drew" << segments << "segments";
	
	// Restore line width
	painter.setLineWidth(1.0f);
	painter.setLineSmooth(false);
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
	qDebug() << "MoonAvoidance: showConfigurationDialog called";
	
	// Safety checks
	if (!config)
	{
		qWarning() << "MoonAvoidance: Cannot show configuration dialog - config is null";
		return;
	}
	
	if (!configDialog)
	{
		qWarning() << "MoonAvoidance: configDialog is null";
		return;
	}
	
	qDebug() << "MoonAvoidance: Config is valid";
	
	// Get current filters safely before showing dialog
	QList<FilterConfig> filters;
	try {
		filters = config->getFilters();
		qDebug() << "MoonAvoidance: Got" << filters.size() << "filters";
	}
	catch (...)
	{
		qWarning() << "MoonAvoidance: Error getting filters, using defaults";
		filters = MoonAvoidanceConfig::getDefaultFilters();
	}
	
	// Reset accepted flag
	configDialog->resetAccepted();
	
	// Set filters in dialog
	configDialog->setFilters(filters);
	
	// Show dialog using StelDialog's setVisible
	configDialog->setVisible(true);
	
	qDebug() << "MoonAvoidance: Dialog shown";
}

void MoonAvoidance::setEnabled(bool b)
{
	if (b != enabled)
	{
		enabled = b;
		flagShow = b; // Use assignment operator to transition the fader
		saveConfiguration();
	}
}
