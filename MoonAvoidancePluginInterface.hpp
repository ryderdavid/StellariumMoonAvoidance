#ifndef MOONAVOIDANCEPLUGININTERFACE_HPP
#define MOONAVOIDANCEPLUGININTERFACE_HPP

#include "StelPluginInterface.hpp"

class MoonAvoidancePluginInterface : public QObject, public StelPluginInterface
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID StelPluginInterface_iid)
	Q_INTERFACES(StelPluginInterface)

public:
	MoonAvoidancePluginInterface();
	virtual ~MoonAvoidancePluginInterface();
	
	virtual StelModule* getStelModule() const;
	virtual StelPluginInfo getPluginInfo() const;
	virtual QObjectList getExtensionList() const { return QObjectList(); }
};

#endif // MOONAVOIDANCEPLUGININTERFACE_HPP

