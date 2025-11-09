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
#include <QPainter>
#include <QtGlobal> // For qMax, qMin, qBound
#include <algorithm> // For std::sort
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
	
	// Track visible and offscreen circles for label positioning
	struct VisibleFilterInfo {
		FilterConfig filter;
		double sepAngle;
		double topmostLeftX;  // Leftmost point along the top of the screen
		double topmostRightX; // Rightmost point along the top of the screen
		Vec3d topmostLeftPoint3D;
		Vec3d topmostRightPoint3D;
	};
	QList<VisibleFilterInfo> visibleFilters;
	QList<QPair<QString, double>> offscreenFilters; // Filter name and separation angle
	
	// Get projector for screen coordinate calculations
	StelProjectorP projector = painter.getProjector();
	if (!projector)
		return;
	
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
			// Draw circle with arrows
			// Find filter index for staggering arrows by comparing names
			int filterIndex = -1;
			for (int idx = 0; idx < filters.size(); ++idx)
			{
				if (filters[idx].name == filter.name)
				{
					filterIndex = idx;
					break;
				}
			}
			if (filterIndex < 0)
				filterIndex = 0; // Fallback to 0 if not found
			drawCircle(painter, moonPos, radius, filter.color, filter.name, radiusDegrees, filterIndex);
			
		// Check if circle is visible and find topmost left and right points
		bool isVisible = false;
		double topmostY = -1e9; // Largest Y (topmost in OpenGL coords where Y increases upward)
		double topmostLeftX = 1e9; // Leftmost X among topmost points
		double topmostRightX = -1e9; // Rightmost X among topmost points
		Vec3d topmostLeftPoint3D;
		Vec3d topmostRightPoint3D;
		
		// Get viewport bounds first
		double vpX = projector->getViewportPosX();
		double vpY = projector->getViewportPosY();
		double vpW = projector->getViewportWidth();
		double vpH = projector->getViewportHeight();
		
		// Sample points around circle to find topmost left and right points in screen space
		const int sampleCount = 128; // Increased for better detection
			for (int i = 0; i < sampleCount; ++i)
			{
				double angle = (2.0 * M_PI * i) / sampleCount;
				
				// Calculate point on circle
				Vec3d moonPosNorm = moonPos;
				moonPosNorm.normalize();
				
				Vec3d north(0, 0, 1);
				Vec3d east(1, 0, 0);
				Vec3d perp1 = moonPosNorm ^ north;
				if (perp1.norm() < 0.1)
					perp1 = moonPosNorm ^ east;
				perp1.normalize();
				Vec3d perp2 = moonPosNorm ^ perp1;
				perp2.normalize();
				
				Vec3d point = moonPosNorm * cos(radius) + 
				              (perp1 * cos(angle) + perp2 * sin(angle)) * sin(radius);
				point.normalize();
				
				// Project to screen coordinates
				Vec3d screenPos;
				if (projector->project(point, screenPos))
				{
					// Check if point is in viewport (with some tolerance)
					if (screenPos[0] >= vpX - 10 && screenPos[0] <= vpX + vpW + 10 &&
					    screenPos[1] >= vpY - 10 && screenPos[1] <= vpY + vpH + 10)
					{
						isVisible = true;
						// Track points that are actually in viewport
						if (screenPos[0] >= vpX && screenPos[0] <= vpX + vpW &&
						    screenPos[1] >= vpY && screenPos[1] <= vpY + vpH)
						{
							// In OpenGL coords, Y increases upward, so topmost = largest Y
							// We want points near the top of the screen (large Y values)
							// Focus on the top portion of the screen - use a larger threshold
							// to catch more points near the top
							double topThreshold = vpY + vpH - 100; // Within 100 pixels of top (increased from 50)
							
							if (screenPos[1] >= topThreshold)
							{
								// This point is near the top of the screen
								// Track the topmost points (largest Y) and their leftmost/rightmost X
								if (screenPos[1] > topmostY)
								{
									// New topmost - reset left and right
									topmostY = screenPos[1];
									topmostLeftX = screenPos[0];
									topmostRightX = screenPos[0];
									topmostLeftPoint3D = point;
									topmostRightPoint3D = point;
								}
								else if (screenPos[1] == topmostY)
								{
									// Same Y level - track leftmost and rightmost
									if (screenPos[0] < topmostLeftX)
									{
										topmostLeftX = screenPos[0];
										topmostLeftPoint3D = point;
									}
									if (screenPos[0] > topmostRightX)
									{
										topmostRightX = screenPos[0];
										topmostRightPoint3D = point;
									}
								}
							}
						}
					}
				}
			}
			
			if (isVisible)
			{
				// If we found topmost points, use them; otherwise find any visible points
				if (topmostY < -1e8) // No topmost points found (topmostY was never updated)
				{
					// Circle is visible but no points near the top
					// Re-sample to find leftmost and rightmost visible points anywhere in viewport
					double leftmostVisibleX = 1e9;
					double rightmostVisibleX = -1e9;
					
					for (int i = 0; i < sampleCount; ++i)
					{
						double angle = (2.0 * M_PI * i) / sampleCount;
						
						Vec3d moonPosNorm = moonPos;
						moonPosNorm.normalize();
						
						Vec3d north(0, 0, 1);
						Vec3d east(1, 0, 0);
						Vec3d perp1 = moonPosNorm ^ north;
						if (perp1.norm() < 0.1)
							perp1 = moonPosNorm ^ east;
						perp1.normalize();
						Vec3d perp2 = moonPosNorm ^ perp1;
						perp2.normalize();
						
						Vec3d point = moonPosNorm * cos(radius) + 
						              (perp1 * cos(angle) + perp2 * sin(angle)) * sin(radius);
						point.normalize();
						
						Vec3d screenPos;
						if (projector->project(point, screenPos))
						{
							if (screenPos[0] >= vpX && screenPos[0] <= vpX + vpW &&
							    screenPos[1] >= vpY && screenPos[1] <= vpY + vpH)
							{
								if (screenPos[0] < leftmostVisibleX)
									leftmostVisibleX = screenPos[0];
								if (screenPos[0] > rightmostVisibleX)
									rightmostVisibleX = screenPos[0];
							}
						}
					}
					
					// Use the visible points, or viewport edges as final fallback
					if (leftmostVisibleX < 1e8)
					{
						topmostLeftX = leftmostVisibleX;
						topmostRightX = rightmostVisibleX;
					}
					else
					{
						// Still no points found - use viewport edges
						topmostLeftX = vpX;
						topmostRightX = vpX + vpW;
					}
				}
				
				// Store visible filter info for drawing after all circles
				VisibleFilterInfo info;
				info.filter = filter;
				info.sepAngle = radiusDegrees;
				info.topmostLeftX = topmostLeftX;
				info.topmostRightX = topmostRightX;
				info.topmostLeftPoint3D = topmostLeftPoint3D;
				info.topmostRightPoint3D = topmostRightPoint3D;
				visibleFilters.append(info);
			}
			else
			{
				// Circle is offscreen - add to list for stacking at left edge
				offscreenFilters.append(QPair<QString, double>(filter.name, radiusDegrees));
			}
		}
		else
		{
			qDebug() << "MoonAvoidance: Invalid radius" << radius << "for filter" << filter.name;
		}
	}
	
	// Draw visible filter labels at top, ensuring they don't overlap circles
	// Use StelPainter's drawText(float x, float y, ...) for screen-space text
	// Track drawn labels for collision detection with offscreen labels
	struct DrawnLabel {
		double x, y, width, height;
	};
	QList<DrawnLabel> drawnVisibleLabels;
	
	for (const VisibleFilterInfo& info : visibleFilters)
	{
		QString labelText = QString("%1 safe at %2°").arg(info.filter.name).arg(info.sepAngle, 0, 'f', 1);
		
		// Estimate text width (approximate: ~8 pixels per character)
		// This is a rough estimate - StelPainter doesn't expose text metrics easily
		// Add safety margin to account for font variations and ensure no overlap
		double estimatedTextWidth = labelText.length() * 8.0 + 20.0; // Add 20px safety margin
		double estimatedTextHeight = 20.0; // Approximate text height
		
		double vpX = projector->getViewportPosX();
		double vpW = projector->getViewportWidth();
		double vpY = projector->getViewportPosY();
		double vpH = projector->getViewportHeight();
		
		// Always prefer left side placement unless it's impossible
		double padding = 60.0; // Padding between text and circle
		double minPaddingFromEdge = 40.0; // Padding from screen edge (matches offscreen labels)
		double labelX;
		bool canPlaceLabel = false;
		
		// Try left side first: place label to the left of circle's topmost left point
		// Rightmost edge of text should be left of circle's topmost left point
		labelX = info.topmostLeftX - padding - estimatedTextWidth;
		
		// Check if left side placement is possible
		if (labelX >= vpX + minPaddingFromEdge)
		{
			// Left side is possible
			canPlaceLabel = true;
		}
		else
		{
			// Left side is impossible - try right side instead
			// Leftmost edge of text should be right of circle's topmost right point
			labelX = info.topmostRightX + padding;
			
			// Check if right side placement is possible
			if (labelX + estimatedTextWidth <= vpX + vpW - minPaddingFromEdge)
			{
				// Right side is possible
				canPlaceLabel = true;
			}
			// If neither side is possible, canPlaceLabel remains false
		}
		
		// Only draw label if it can be placed properly
		if (canPlaceLabel)
		{
			// StelPainter::drawText uses OpenGL coordinates where Y=0 is at bottom
			// So we need to invert Y: top of viewport is at vpY + vpH
			// For top of screen: start at bottom (vpY) and go up by height, then subtract offset
			double paddingFromTopForVisible = 50.0; // Padding from top (matches offscreen labels)
			double labelY = vpY + vpH - paddingFromTopForVisible; // Padding from top (in OpenGL coords, Y increases upward)
			
			// Set text color to filter color
			Vec3f textColor(info.filter.color.redF(), info.filter.color.greenF(), info.filter.color.blueF());
			painter.setColor(textColor, 1.0f);
			
			try {
				// Use StelPainter's screen-space drawText method
				painter.drawText(static_cast<float>(labelX), static_cast<float>(labelY), labelText, 0.0f);
				
				// Track this label for collision detection
				DrawnLabel drawn;
				drawn.x = labelX;
				drawn.y = labelY - estimatedTextHeight; // Y is top in OpenGL coords, so bottom is Y - height
				drawn.width = estimatedTextWidth;
				drawn.height = estimatedTextHeight;
				drawnVisibleLabels.append(drawn);
			}
			catch (...)
			{
				qWarning() << "MoonAvoidance: Error drawing visible label text";
			}
		}
	}
	
	// Draw offscreen labels stacked at left edge
	// Hide labels that would collide with visible labels or circles
	if (!offscreenFilters.isEmpty())
	{
		double vpX = projector->getViewportPosX();
		double vpY = projector->getViewportPosY();
		double vpH = projector->getViewportHeight();
		double paddingFromTop = 50.0; // Increased padding from top edge
		double paddingFromLeft = 40.0; // Increased padding from left edge
		double lineSpacing = 40.0; // Increased spacing between stacked labels
		double startY = vpY + vpH - paddingFromTop; // Start from top (in OpenGL coords)
		
		// Check if any circles are visible near the left edge where offscreen labels are drawn
		// We'll check if any visible circle has points near the left edge (within 100 pixels)
		double leftEdgeCheckX = vpX + paddingFromLeft + 100.0; // Check area where offscreen labels are drawn
		bool hasCircleNearLeftEdge = false;
		for (const VisibleFilterInfo& info : visibleFilters)
		{
			// Check if circle's leftmost point is near the left edge
			if (info.topmostLeftX < leftEdgeCheckX)
			{
				hasCircleNearLeftEdge = true;
				break;
			}
		}
		
		for (int i = 0; i < offscreenFilters.size(); ++i)
		{
			const QPair<QString, double>& filterInfo = offscreenFilters[i];
			QString filterName = filterInfo.first;
			double sepAngle = filterInfo.second;
			
			// Find the filter config for color
			FilterConfig filterConfig;
			for (const FilterConfig& f : filters)
			{
				if (f.name == filterName)
				{
					filterConfig = f;
					break;
				}
			}
			
			QString labelText = QString("%1 safe at %2°").arg(filterName).arg(sepAngle, 0, 'f', 1);
			
			// Calculate label position at left edge, stacked vertically
			double labelX = vpX + paddingFromLeft; // Padding from left edge
			double labelY = startY - (i * lineSpacing); // Stack upward from top (in OpenGL coords, Y increases upward)
			double estimatedTextWidth = labelText.length() * 8.0 + 20.0; // Add 20px safety margin
			double estimatedTextHeight = 20.0; // Approximate text height
			
			// Check for collisions with visible labels
			bool wouldCollide = false;
			for (const DrawnLabel& drawn : drawnVisibleLabels)
			{
				// Check if offscreen label would overlap with visible label
				// Using bounding box collision detection
				double offscreenLeft = labelX;
				double offscreenRight = labelX + estimatedTextWidth;
				double offscreenTop = labelY;
				double offscreenBottom = labelY - estimatedTextHeight;
				
				double visibleLeft = drawn.x;
				double visibleRight = drawn.x + drawn.width;
				double visibleTop = drawn.y + drawn.height;
				double visibleBottom = drawn.y;
				
				// Check for overlap (with some padding)
				double collisionPadding = 5.0;
				if (!(offscreenRight + collisionPadding < visibleLeft || 
				      offscreenLeft - collisionPadding > visibleRight ||
				      offscreenBottom - collisionPadding > visibleTop ||
				      offscreenTop + collisionPadding < visibleBottom))
				{
					wouldCollide = true;
					break;
				}
			}
			
			// Also check if a circle is visible near the left edge
			if (!wouldCollide && hasCircleNearLeftEdge)
			{
				// Check if this offscreen label's Y position is near where visible circles are
				// (within the top portion of the screen where labels are drawn)
				double topThreshold = vpY + vpH - 150.0; // Check top 150 pixels
				if (labelY >= topThreshold)
				{
					wouldCollide = true;
				}
			}
			
			// Only draw if no collision
			if (!wouldCollide)
			{
				// Set text color to filter color
				Vec3f textColor(filterConfig.color.redF(), filterConfig.color.greenF(), filterConfig.color.blueF());
				painter.setColor(textColor, 1.0f);
				
				try {
					// Use StelPainter's screen-space drawText method
					painter.drawText(static_cast<float>(labelX), static_cast<float>(labelY), labelText, 0.0f);
				}
				catch (...)
				{
					qWarning() << "MoonAvoidance: Error drawing offscreen label text";
				}
			}
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

void MoonAvoidance::drawCircle(StelPainter& painter, const Vec3d& moonPos, double radius, const QColor& color, const QString& filterName, double radiusDegrees, int filterIndex) const
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
	// radiusDegrees is already a parameter, use it directly
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
	
	// Draw 6 radial arrows pointing outward from the circle, away from the moon
	// Arrows are evenly spaced (60 degrees apart), but staggered for each filter circle
	const int arrowCount = 6;
	const double arrowSpacing = 2.0 * M_PI / arrowCount;
	const double arrowLength = 0.03; // ~1.7 degrees outward from circle
	const double arrowHeadLength = 0.01; // ~0.6 degrees for arrowhead
	const double arrowHeadAngle = M_PI / 6.0; // 30 degrees for arrowhead
	
	// Stagger arrows for each filter circle by rotating them
	// Each filter gets a different starting angle offset
	// Offset by 10 degrees per filter index to create a staggered effect
	const double staggerOffset = (filterIndex * 10.0) * M_PI / 180.0; // Convert to radians
	
	// Set color for arrows (same as circle)
	painter.setColor(colorVec, 1.0f);
	painter.setLineWidth(2.0f);
	
	for (int i = 0; i < arrowCount; ++i)
	{
		double angle = i * arrowSpacing + staggerOffset;
		
		// Calculate point on the circle
		Vec3d circlePoint = moonPosNorm * cos(radius) + 
		                   (perp1 * cos(angle) + perp2 * sin(angle)) * sin(radius);
		circlePoint.normalize();
		
		// Calculate point further out (away from moon) for arrow tip
		double arrowRadius = radius + arrowLength;
		if (arrowRadius > M_PI * 0.9)
			arrowRadius = M_PI * 0.9;
		
		Vec3d arrowTip = moonPosNorm * cos(arrowRadius) + 
		                (perp1 * cos(angle) + perp2 * sin(angle)) * sin(arrowRadius);
		arrowTip.normalize();
		
		// Draw arrow shaft (line from circle to tip)
		try {
			painter.drawGreatCircleArc(circlePoint, arrowTip, nullptr);
		}
		catch (...)
		{
			qWarning() << "MoonAvoidance: Error drawing arrow shaft";
		}
		
		// Draw arrowhead (small triangle at the tip)
		// Calculate two points for the arrowhead, perpendicular to the arrow direction
		Vec3d arrowDir = arrowTip - circlePoint;
		arrowDir.normalize();
		
		// Find a perpendicular vector to arrow direction
		Vec3d perpArrow = moonPosNorm ^ arrowDir;
		if (perpArrow.norm() < 0.1)
		{
			// If arrow is parallel to moon direction, use a different perpendicular
			perpArrow = perp1 ^ arrowDir;
		}
		perpArrow.normalize();
		
		// Calculate arrowhead base point (slightly back from tip)
		double headBaseRadius = arrowRadius - arrowHeadLength;
		if (headBaseRadius < radius)
			headBaseRadius = radius;
		
		Vec3d headBase = moonPosNorm * cos(headBaseRadius) + 
		                (perp1 * cos(angle) + perp2 * sin(angle)) * sin(headBaseRadius);
		headBase.normalize();
		
		// Calculate arrowhead side points
		Vec3d headSide1 = headBase + perpArrow * (arrowHeadLength * 0.5);
		headSide1.normalize();
		
		Vec3d headSide2 = headBase - perpArrow * (arrowHeadLength * 0.5);
		headSide2.normalize();
		
		// Draw arrowhead triangle
		try {
			painter.drawGreatCircleArc(arrowTip, headSide1, nullptr);
			painter.drawGreatCircleArc(arrowTip, headSide2, nullptr);
			painter.drawGreatCircleArc(headSide1, headSide2, nullptr);
		}
		catch (...)
		{
			qWarning() << "MoonAvoidance: Error drawing arrowhead";
		}
	}
	
	// Labels temporarily hidden - user wants to try a different approach
	/*
	// Find the best position for the label: middle point of visible segment above horizon
	// Sample points around the circle and find the middle of the visible arc above horizon
	StelCore* core = StelApp::getInstance().getCore();
	if (!core)
		return;
	
	StelProjectorP projector = painter.getProjector();
	if (!projector)
		return;
	
	QList<double> visibleAngles; // Store angles of visible points above horizon
	
	// Sample points around the circle to find visible points above horizon
	const int sampleCount = 128; // More samples for better accuracy
	for (int i = 0; i < sampleCount; ++i)
	{
		double angle = (2.0 * M_PI * i) / sampleCount;
		
		// Calculate point on circle
		Vec3d point = moonPosNorm * cos(radius) + 
		              (perp1 * cos(angle) + perp2 * sin(angle)) * sin(radius);
		point.normalize();
		
		// Convert to Alt/Az to check altitude
		Vec3d altAzPos = core->j2000ToAltAz(point, StelCore::RefractionOff);
		double alt, az;
		StelUtils::rectToSphe(&az, &alt, altAzPos);
		double altitude = alt * 180.0 / M_PI; // Convert to degrees
		
		// Check if point is visible in viewport
		Vec3d screenPos;
		bool isVisible = projector->project(point, screenPos);
		
		// Check if point is within viewport bounds
		if (isVisible)
		{
			double vpX = projector->getViewportPosX();
			double vpY = projector->getViewportPosY();
			double vpW = projector->getViewportWidth();
			double vpH = projector->getViewportHeight();
			
			isVisible = (screenPos[0] >= vpX && screenPos[0] <= vpX + vpW &&
			            screenPos[1] >= vpY && screenPos[1] <= vpY + vpH);
		}
		
		// If point is visible and above horizon, store its angle
		if (isVisible && altitude > 0.0)
		{
			visibleAngles.append(angle);
		}
	}
	
	// Find the middle angle of the visible segment
	// This is the geometric middle of the largest continuous arc segment that's visible and above horizon
	double bestAngle = 0.0;
	if (!visibleAngles.isEmpty())
	{
		// Sort angles to handle wraparound at 0/2π
		std::sort(visibleAngles.begin(), visibleAngles.end());
		
		if (visibleAngles.size() == 1)
		{
			bestAngle = visibleAngles[0];
		}
		else
		{
			// Find the largest gap between consecutive visible angles
			// This gap represents where the circle is NOT visible
			// The visible segment is everything else (the complement)
			double maxGap = 0.0;
			int maxGapIndex = -1;
			
			// Check gaps between consecutive angles
			for (int i = 0; i < visibleAngles.size() - 1; ++i)
			{
				double gap = visibleAngles[i + 1] - visibleAngles[i];
				if (gap > maxGap)
				{
					maxGap = gap;
					maxGapIndex = i;
				}
			}
			
			// Check wraparound gap (between last and first)
			double wrapGap = (2.0 * M_PI - visibleAngles.last()) + visibleAngles.first();
			
			if (wrapGap > maxGap)
			{
				// Wraparound case: the visible segment wraps around 0/2π
				// The visible segment is from last angle to first angle (wrapping)
				// Calculate geometric middle: (last + (first + 2π)) / 2, then normalize
				double startAngle = visibleAngles.last();
				double endAngle = visibleAngles.first() + 2.0 * M_PI;
				bestAngle = (startAngle + endAngle) / 2.0;
				if (bestAngle >= 2.0 * M_PI)
					bestAngle -= 2.0 * M_PI;
			}
			else if (maxGapIndex >= 0)
			{
				// Normal case: the visible segment is continuous
				// If the largest gap is between angle[i] and angle[i+1], then:
				// - The gap (not visible) is from angle[i] to angle[i+1]
				// - The visible segment is from angle[i+1] to angle[i] (wrapping)
				// - The middle is halfway between angle[i+1] and angle[i] (wrapping)
				int startIdx = (maxGapIndex + 1) % visibleAngles.size();
				int endIdx = maxGapIndex;
				
				double startAngle = visibleAngles[startIdx];
				double endAngle = visibleAngles[endIdx];
				
				// Since the segment wraps (startIdx comes after endIdx in sorted order),
				// we need to add 2π to endAngle to calculate the middle correctly
				// The middle is: (startAngle + (endAngle + 2π)) / 2
				endAngle += 2.0 * M_PI;
				
				bestAngle = (startAngle + endAngle) / 2.0;
				if (bestAngle >= 2.0 * M_PI)
					bestAngle -= 2.0 * M_PI;
			}
			else
			{
				// Fallback: if all angles are evenly spaced, use the middle
				// Calculate average of all visible angles
				double sum = 0.0;
				for (double angle : visibleAngles)
				{
					sum += angle;
				}
				bestAngle = sum / visibleAngles.size();
			}
		}
	}
	
	// Calculate label position at the best angle, offset outward
	Vec3d labelPoint = moonPosNorm * cos(radius) + 
	                   (perp1 * cos(bestAngle) + perp2 * sin(bestAngle)) * sin(radius);
	labelPoint.normalize();
	
	// Offset label further outward from the circle for better visibility
	// Calculate a point slightly further from the moon
	double labelOffsetRadius = radius + 0.1; // Add ~5.7 degrees offset
	if (labelOffsetRadius > M_PI * 0.9) // Don't go too far
		labelOffsetRadius = M_PI * 0.9;
	
	Vec3d labelPos = moonPosNorm * cos(labelOffsetRadius) + 
	                 (perp1 * cos(bestAngle) + perp2 * sin(bestAngle)) * sin(labelOffsetRadius);
	labelPos.normalize();
	
	// Format label text: "SAFE TO SHOOT {FILTER} ({radiusDegrees}°)"
	QString labelText = QString("SAFE TO SHOOT %1 (%2°)").arg(filterName).arg(radiusDegrees, 0, 'f', 1);
	
	// Set text color to match circle color, but ensure it's visible
	Vec3f textColor(color.redF(), color.greenF(), color.blueF());
	painter.setColor(textColor, 1.0f);
	
	// Draw text at the label position
	try {
		painter.drawText(labelPos, labelText, 0.0f, 0.0f, 0.0f, true);
	}
	catch (...)
	{
		qWarning() << "MoonAvoidance: Error drawing label text";
	}
	*/
	
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
