#include "MoonAvoidance.hpp"
#include "MoonAvoidancePluginInterface.hpp"
#include "StelPluginInterface.hpp"
#include "StelModuleMgr.hpp"
#include <QString>

MoonAvoidancePluginInterface::MoonAvoidancePluginInterface()
{
}

MoonAvoidancePluginInterface::~MoonAvoidancePluginInterface()
{
}

StelModule* MoonAvoidancePluginInterface::getStelModule() const
{
	return new MoonAvoidance();
}

StelPluginInfo MoonAvoidancePluginInterface::getPluginInfo() const
{
	StelPluginInfo info;
	info.id = "MoonAvoidance";
	info.displayedName = "Moon Avoidance";
	info.authors = "Stellarium Community";
	info.contact = "";
	info.description = "Visualizes moon avoidance zones for different filters using Lorentzian calculations";
	info.version = "1.0.0";
	info.license = "GPL";
	return info;
}

