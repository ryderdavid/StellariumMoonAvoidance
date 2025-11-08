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
	
	// Enable/Disable
	bool isEnabled() const { return enabled; }
	void setEnabled(bool b);

private:
	// Moon avoidance calculations
	double calculateSeparation(const FilterConfig& filter, double moonAltitude) const;
	double calculateWidth(const FilterConfig& filter, double moonAltitude) const;
	double calculateCircleRadius(const FilterConfig& filter, double moonAltitude, double moonPhase) const;
	
	// Drawing
	void drawCircle(StelPainter& painter, const Vec3d& moonPos, double radius, const QColor& color) const;
	
	// Configuration
	MoonAvoidanceConfig* config;
	MoonAvoidanceDialog* configDialog;
	
	// State
	bool enabled;
	LinearFader flagShow;
	
	// Moon data
	double lastMoonAltitude;
	double lastMoonPhase;
};

#endif // MOONAVOIDANCE_HPP

