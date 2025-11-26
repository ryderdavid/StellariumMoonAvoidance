# Moon Avoidance Plugin for Stellarium

A dynamic Stellarium plugin that visualizes moon avoidance zones for different filters using Moon Avoidance Lorentzian calculations.

## Description

This plugin draws concentric circles around the moon representing "no-shoot-zones" for different filters (LRGB, O, S, H). The circles are calculated using Moon Avoidance Lorentzian formulas that take into account:
- Moon's current altitude above the horizon
- Moon's phase (illumination)
- Filter-specific parameters (Separation, Width, Relaxation, MinAlt, MaxAlt)

## Features

- Real-time visualization of moon avoidance zones
- Configurable filter parameters via configuration dialog
- Support for multiple filters with custom colors
- Automatic updates as time progresses in Stellarium
- Default filters: LRGB (white), O (cyan), S (yellow), H (red)

## Building

### Prerequisites

- CMake 3.16 or higher
- Qt6 (Core, Widgets)
- Stellarium SDK/API
- C++17 compatible compiler

### Build Instructions

This plugin requires Qt 6.5.3 (or the version specified in `qt_version.conf`).

#### Helper Scripts (Recommended)

We provide helper scripts to simplify the build process.

**Linux / macOS:**
```bash
./build.sh --qt-path /path/to/Qt/6.5.3/macos --stel-root /path/to/stellarium/source
```
*Options:*
- `--qt-path`: Path to Qt installation
- `--stel-root`: Path to Stellarium source (optional if installed)
- `--clean`: Clean build directory before building
- No arguments: Runs in interactive mode

**Windows:**
```powershell
.\build.ps1 -QtPath "C:\Qt\6.5.3\msvc2019_64" -StelRoot "C:\path\to\stellarium\source"
```
*Parameters:*
- `-QtPath`: Path to Qt installation
- `-StelRoot`: Path to Stellarium source
- `-Clean`: Clean build directory before building
- No arguments: Runs in interactive mode

#### Manual Build

You MUST provide the path to your Qt installation using `CMAKE_PREFIX_PATH`.

1.  **Determine your Qt path**: This should point to the platform-specific directory inside your Qt installation.
    *   macOS: `/path/to/Qt/6.5.3/macos`
    *   Windows: `C:/Qt/6.5.3/msvc2019_64` (use forward slashes or escape backslashes)
    *   Linux: `/path/to/Qt/6.5.3/gcc_64`

2.  **Configure and Build**:

#### Option 1: Build against Stellarium source tree

If you have the Stellarium source code:

```bash
mkdir build
cd build
cmake .. \
    -DSTELROOT=/path/to/stellarium/source \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.3/macos
make
make install
```

#### Option 2: Build with manual Stellarium paths

If you have Stellarium installed or built elsewhere:

```bash
mkdir build
cd build
cmake .. \
    -DSTELLARIUM_SOURCE_DIR=/path/to/stellarium/src \
    -DSTELLARIUM_BUILD_DIR=/path/to/stellarium/build \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/6.5.3/macos
make
make install
```

#### Option 3: Build as part of Stellarium source tree

Copy this plugin directory into Stellarium's plugins directory:

```bash
cp -r MoonAvoidance /path/to/stellarium/plugins/
cd /path/to/stellarium
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

The plugin will be installed to Stellarium's plugin directory.

## Configuration

The plugin can be configured through Stellarium's plugin configuration dialog. You can:
- Add, remove, or modify filters
- Adjust parameters for each filter:
  - **Separation**: Base separation distance in degrees
  - **Width**: Width of the avoidance zone
  - **Relaxation**: Relaxation scale factor
  - **MinAlt**: Minimum altitude for calculations (degrees)
  - **MaxAlt**: Maximum altitude for calculations (degrees)
- Set custom colors for each filter

## Default Filter Values

| Filter | Separation | Width | Relaxation | MinAlt | MaxAlt | Color |
|--------|-----------|-------|------------|--------|--------|-------|
| LRGB   | 140       | 14    | 2          | -15    | 5      | White |
| O      | 120       | 10    | 1          | -15    | 5      | Cyan  |
| S      | 45        | 9     | 1          | -15    | 5      | Yellow|
| H      | 35        | 7     | 1          | -15    | 5      | Red   |

## Formulas

The plugin uses the following formulas:

- **Separation**: `Separation = Separation + Relaxation * (moonAltitude - MaxAlt)`
- **Width**: `Width = Width * ((moonAltitude - MinAlt) / (MaxAlt - MinAlt))`
- **Circle Radius**: Based on adjusted separation and moon phase

## License

GPL (GNU General Public License)

## Author

Stellarium Community

