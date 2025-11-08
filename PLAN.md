# Moon Avoidance Plugin Implementation Plan

## Overview
Implement a Stellarium dynamic plugin that visualizes moon avoidance zones by drawing concentric circles around the moon. Each circle represents the boundary for a specific filter (LRGB, O, S, H) using Moon Avoidance Lorentzian calculations.

## Implementation Status

### ✅ Completed

1. **Project Structure**
   - ✅ Created CMakeLists.txt with proper build configuration
   - ✅ Created plugin.ini with metadata
   - ✅ Set up proper directory structure
   - ✅ Created .gitignore for build artifacts

2. **Plugin Core (MoonAvoidance.*)**
   - ✅ Main plugin class inheriting from StelModule
   - ✅ Real-time moon position and altitude tracking
   - ✅ Moon avoidance calculations using Lorentzian formulas
   - ✅ Circle drawing using proper spherical geometry
   - ✅ Real-time updates as Stellarium time progresses
   - ✅ Error handling and safety checks

3. **Configuration System (MoonAvoidanceConfig.*)**
   - ✅ Filter configuration management
   - ✅ Default filters: LRGB (white), O (cyan), S (yellow), H (red)
   - ✅ Save/load from Stellarium config file
   - ✅ Support for adding, removing, and modifying filters

4. **Configuration Dialog (MoonAvoidanceDialog.*)**
   - ✅ Qt-based dialog for configuring filters
   - ✅ Table view with editable parameters
   - ✅ Color picker for each filter
   - ✅ Add/Remove filter functionality
   - ✅ Input validation
   - ✅ Safety checks and error handling

5. **Plugin Interface (MoonAvoidancePluginInterface.*)**
   - ✅ Dynamic plugin interface implementation
   - ✅ Qt plugin system with Q_PLUGIN_METADATA
   - ✅ Proper MOC generation
   - ✅ configureGui() method implemented

6. **Build System**
   - ✅ CMakeLists.txt configured for dynamic plugin
   - ✅ Proper linking with Qt6 and Stellarium headers
   - ✅ Runtime symbol resolution for dynamic plugins
   - ✅ Test suite configuration

7. **Testing**
   - ✅ Unit tests for moon avoidance calculations
   - ✅ Tests for configuration management
   - ✅ Test helper functions

8. **Documentation**
   - ✅ README.md with build instructions and usage
   - ✅ plugin.ini with plugin metadata
   - ✅ Code comments and documentation

9. **Git Repository**
   - ✅ Repository initialized
   - ✅ Initial commit created
   - ✅ Pushed to GitHub: https://github.com/ryderdavid/StellariumMoonAvoidance

### 🔄 In Progress

1. **Configuration Dialog Stability**
   - 🔄 Fixing crash when opening configuration dialog
   - ✅ Changed dialog creation to stack-based (safer memory management)
   - ✅ Added comprehensive error handling
   - ✅ Added safety checks for null pointers
   - ✅ Disconnected signals during table population to avoid recursive updates
   - ✅ Added debug logging to trace crash location
   - ⏳ Investigating crash - need to check Stellarium log for error details
   - ⏳ May need to simplify dialog or use different approach

### ⏳ Pending

1. **Circle Drawing Improvements**
   - ⏳ Verify circle drawing accuracy on sphere
   - ⏳ Test with different moon phases and altitudes
   - ⏳ Optimize drawing performance if needed

2. **Configuration Dialog Polish**
   - ⏳ Verify dialog works without crashes
   - ⏳ Test all dialog functionality (add/remove/edit filters)
   - ⏳ Verify color picker works correctly

3. **Integration Testing**
   - ⏳ Test plugin loading in Stellarium
   - ⏳ Test real-time updates
   - ⏳ Test configuration persistence
   - ⏳ Test with different Stellarium versions

4. **Documentation Updates**
   - ⏳ Add troubleshooting section
   - ⏳ Add screenshots if possible
   - ⏳ Update with any final implementation details

## Technical Details

### Formulas Implemented
- **Separation**: `Separation = Separation + Relaxation * (moonAltitude - MaxAlt)`
- **Width**: `Width = Width * ((moonAltitude - MinAlt) / (MaxAlt - MinAlt))`
- **Circle Radius**: Based on adjusted separation and moon phase (full moon = full separation)

### Default Filter Values
| Filter | Separation | Width | Relaxation | MinAlt | MaxAlt | Color |
|--------|-----------|-------|------------|--------|--------|-------|
| LRGB   | 140       | 14    | 2          | -15    | 5      | White |
| O      | 120      | 10    | 1          | -15    | 5      | Cyan  |
| S      | 45        | 9     | 1          | -15    | 5      | Yellow|
| H      | 35        | 7     | 1          | -15    | 5      | Red   |

### Known Issues
- ⚠️ Configuration dialog crashes when opened (actively debugging)
  - Added debug logging to trace crash location
  - Disconnected signals during table population
  - Using stack-based dialog creation for safer memory management
  - Need to check Stellarium log file for crash details
- Some compiler warnings about missing `override` keywords (non-critical)

### Next Steps
1. Test the fixed configuration dialog
2. Verify all functionality works correctly
3. Final testing and polish
4. Update documentation with any final details

