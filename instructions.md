This is going to be another stellarium plugin, but its own repository, and a Dynamic Plugin. I would like to implement a dynamic plugin called Moon Avoidance, which uses the Moon Avoidance Lorentzian. You can read about how it works here: https://tcpalmer.github.io/nina-scheduler/target-management/exposure-templates.html#moon-avoidance 
and here: http://bobdenny.com/ar/RefDocs/HelpFiles/ACPScheduler81Help/Constraints.htm 


Below are sample default values. These should be kept in some standardized for Stellarium config file -- look at how other dynamic plugins store default values. 

[LRGB]
Separation=140
Width=14
Relaxation=2
MinAlt=-15
MaxAlt=5

[O]
Separation=120
Width=10
Relaxation=1
MinAlt=-15
MaxAlt=5

[S]
Separation=45
Width=9
Relaxation=1
MinAlt=-15
MaxAlt=5

[H]
Separation=35
Width=7
Relaxation=1
MinAlt=-15
MaxAlt=5

The ultimate purpose of this plugin is to draw concentric circles around the moon at a given date and time, that reflect the boundary of a "no-shoot-zone" for that particular filter. If you study the moon avoidance curve, for the above values, say for H, which has 35/7, this would mean that at exact full moon, the boundary circle would be drawn at 35 degrees radius around the moon. 

Formulas: 
Separation = Separation + RelaxScale * (altitude - MaxAltitude)
Width = Width * ((altitude - MinimumAltitude) / (MaximumAltitude - MinimumAltitude))

Use what knowledge we have gained in implementing the static Oculars plugin to work on this dynamic plugin. Some additional sample resources are:
https://github.com/Stellarium/stellarium/tree/master/plugins/SimpleDrawLine - SimpleDrawLine plugin.
https://github.com/Stellarium/stellarium-dynamic-plugin  - Sample Dynamic Plugin template. 

This plugin should be configured via a configuration dialog opened from the plugin listing, and allow you to set each of those params above, adding, removing or changing any of the default filters for other values. Default colors for each of these circles should be LRGB = white; H = red; O = cyan; S = yellow. 

The stellarium coding conventions should be followed for this, https://stellarium.org/doc/25.0/codingStyle.html , and tests should be built during development to ensure coverage of the plugin's functionality. Feel free to look at the testing suites for other plugins in the stellarium repository (https://github.com/Stellarium/stellarium/tree/master )
