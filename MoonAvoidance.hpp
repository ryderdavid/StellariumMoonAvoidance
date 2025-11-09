#ifndef MOONAVOIDANCE_HPP
#define MOONAVOIDANCE_HPP

#include "StelModule.hpp"
#include "StelFader.hpp"
#include "MoonAvoidanceConfig.hpp"
#include "VecMath.hpp"
#include <QOpenGLFunctions>

class StelPainter;
class MoonAvoidanceDialog;

class MoonAvoidance : public StelModule
{
	Q_OBJECT

public:
	MoonAvoidance();
	virtual ~MoonAvoidance();
	
	// StelModule methods
	virtual void init();
	virtual void update(double deltaTime);
	virtual void draw(StelCore* core);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	virtual bool configureGui(bool show=true) override;
	
	// Configuration
	void loadConfiguration();
	void saveConfiguration();
	
	// Dialog
	void showConfigurationDialog();
	MoonAvoidanceDialog* getDialog() { return configDialog; }
	
	// Enable/Disable
	bool isEnabled() const { return enabled; }
	void setEnabled(bool b);
	
	// Get current moon data for dialog calculations
	double getCurrentMoonAgeDays() const { return lastMoonAgeDays; } // Days since new moon
	double getCurrentMoonAgeFromFullDays() const { return lastMoonAgeFromFullDays; } // Days from full moon
	double getCurrentMoonAltitude() const { return lastMoonAltitude; }

private:
	// Moon avoidance calculations
	double calculateSeparation(const FilterConfig& filter, double moonAltitude) const;
	double calculateWidth(const FilterConfig& filter, double moonAltitude) const;
	double calculateCircleRadius(const FilterConfig& filter, double moonAltitude, double moonAgeDays) const;
	
	// Drawing
	void drawCircle(StelPainter& painter, const Vec3d& moonPos, double radius, const QColor& color, const QString& filterName, double radiusDegrees, int filterIndex) const;
	
	// Configuration
	MoonAvoidanceConfig* config;
	MoonAvoidanceDialog* configDialog;
	
	// State
	bool enabled;
	LinearFader flagShow;
	
	// Moon data
	double lastMoonAltitude;
	double lastMoonAgeDays; // Days since new moon (0 = new moon, ~14.77 = full moon)
	double lastMoonAgeFromFullDays; // Days from full moon (0 = full moon) for formula
};

#endif // MOONAVOIDANCE_HPP

