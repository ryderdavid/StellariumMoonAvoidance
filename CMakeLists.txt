cmake_minimum_required(VERSION 3.16)
project(MoonAvoidance VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to specify Stellarium source root
set(STELROOT "" CACHE PATH "Path to Stellarium source root directory")
set(STELLARIUM_BUILD_DIR "" CACHE PATH "Path to Stellarium build directory")

# Load required Qt version from config file
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/qt_version.conf" CONFIG_CONTENTS REGEX "^QT_VERSION_REQUIRED=")
foreach(line ${CONFIG_CONTENTS})
    if("${line}" MATCHES "QT_VERSION_REQUIRED=(.*)")
        set(REQUIRED_QT_VERSION "${CMAKE_MATCH_1}")
    endif()
endforeach()

if(NOT REQUIRED_QT_VERSION)
    message(FATAL_ERROR "Could not determine required Qt version from qt_version.conf")
endif()

message(STATUS "Required Qt version: ${REQUIRED_QT_VERSION}")

# Find required packages
# User MUST provide CMAKE_PREFIX_PATH to the specific Qt installation
# e.g. cmake -DCMAKE_PREFIX_PATH=~/Qt/6.5.3/macos ..

if(NOT CMAKE_PREFIX_PATH)
    message(FATAL_ERROR "CMAKE_PREFIX_PATH is not defined. Please set it to your Qt installation path (e.g. -DCMAKE_PREFIX_PATH=/path/to/Qt/${REQUIRED_QT_VERSION}/platform)")
endif()

find_package(Qt6 ${REQUIRED_QT_VERSION} EXACT REQUIRED COMPONENTS Core Widgets)

# Try to find Stellarium
if(STELROOT)
	set(STELLARIUM_SOURCE_DIR ${STELROOT})
else()
	# Try common locations
	set(STELLARIUM_SOURCE_DIR "" CACHE PATH "Stellarium source directory")
endif()

# If we have a Stellarium source directory, use it
if(STELROOT)
	set(STELLARIUM_SOURCE_DIR ${STELROOT}/src)
	set(STELLARIUM_INCLUDE_DIRS
		${STELROOT}
		${STELROOT}/src
		${STELROOT}/src/core
		${STELROOT}/src/core/modules
		${STELROOT}/src/gui
	)
	
	# If build directory is specified, use it for libraries
	if(STELLARIUM_BUILD_DIR)
		set(STELLARIUM_LIB_DIR ${STELLARIUM_BUILD_DIR}/src)
	else()
		# Try to find build directory relative to source
		if(EXISTS "${STELROOT}/build/src")
			set(STELLARIUM_LIB_DIR ${STELROOT}/build/src)
		else()
			set(STELLARIUM_LIB_DIR ${STELROOT}/src)
		endif()
	endif()
	
	message(STATUS "Using Stellarium source directory: ${STELROOT}")
	message(STATUS "Stellarium include directories: ${STELLARIUM_INCLUDE_DIRS}")
	message(STATUS "Stellarium library directory: ${STELLARIUM_LIB_DIR}")
	
	# Link against Stellarium libraries
	# Check if separate libraries exist, otherwise use main library
	if(EXISTS "${STELLARIUM_LIB_DIR}/libStelCore.a")
		set(STELLARIUM_LIBS
			${STELLARIUM_LIB_DIR}/libStelCore.a
			${STELLARIUM_LIB_DIR}/libStelModule.a
			${STELLARIUM_LIB_DIR}/libStelPainter.a
			${STELLARIUM_LIB_DIR}/libStelApp.a
		)
	else()
		# Use main library if separate libraries don't exist
		set(STELLARIUM_LIBS ${STELLARIUM_LIB_DIR}/libstelMain.a)
	endif()
elseif(STELLARIUM_SOURCE_DIR)
	# Manual specification
	set(STELLARIUM_INCLUDE_DIRS
		${STELLARIUM_SOURCE_DIR}/..
		${STELLARIUM_SOURCE_DIR}
		${STELLARIUM_SOURCE_DIR}/core
		${STELLARIUM_SOURCE_DIR}/core/modules
		${STELLARIUM_SOURCE_DIR}/gui
	)
	
	if(STELLARIUM_BUILD_DIR)
		set(STELLARIUM_LIB_DIR ${STELLARIUM_BUILD_DIR}/src)
	else()
		set(STELLARIUM_LIB_DIR ${STELLARIUM_SOURCE_DIR})
	endif()
	
	# Check if separate libraries exist, otherwise use main library
	if(EXISTS "${STELLARIUM_LIB_DIR}/libStelCore.a")
		set(STELLARIUM_LIBS
			${STELLARIUM_LIB_DIR}/libStelCore.a
			${STELLARIUM_LIB_DIR}/libStelModule.a
			${STELLARIUM_LIB_DIR}/libStelPainter.a
			${STELLARIUM_LIB_DIR}/libStelApp.a
		)
	else()
		# Use main library if separate libraries don't exist
		set(STELLARIUM_LIBS ${STELLARIUM_LIB_DIR}/libstelMain.a)
	endif()
else()
	# Try to find installed Stellarium
	find_package(Stellarium QUIET)
	if(Stellarium_FOUND)
		message(STATUS "Using installed Stellarium")
		set(STELLARIUM_INCLUDE_DIRS ${Stellarium_INCLUDE_DIRS})
		set(STELLARIUM_LIBS ${Stellarium_LIBRARIES})
	else()
		message(FATAL_ERROR "Stellarium not found. Please specify STELROOT to point to Stellarium source directory (e.g., cmake .. -DSTELROOT=/path/to/stellarium), or set STELLARIUM_SOURCE_DIR and STELLARIUM_BUILD_DIR.")
	endif()
endif()

# Plugin source files
set(PLUGIN_SOURCES
    MoonAvoidance.cpp
    MoonAvoidanceConfig.cpp
    MoonAvoidanceDialog.cpp
    MoonAvoidancePluginInterface.cpp
)

set(PLUGIN_HEADERS
    MoonAvoidance.hpp
    MoonAvoidanceConfig.hpp
    MoonAvoidanceDialog.hpp
    MoonAvoidancePluginInterface.hpp
)

# Create the plugin library
add_library(MoonAvoidance SHARED
    ${PLUGIN_SOURCES}
    ${PLUGIN_HEADERS}
)

# Enable AUTOMOC for Qt plugin interface
set_target_properties(MoonAvoidance PROPERTIES
    AUTOMOC ON
)

target_include_directories(MoonAvoidance PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${STELLARIUM_INCLUDE_DIRS}
)

# For dynamic plugins, we link against Qt6::Core and Qt6::Widgets for compilation
# The -undefined dynamic_lookup flag ensures runtime uses Stellarium's bundled Qt
target_link_libraries(MoonAvoidance PRIVATE
    Qt6::Core
    Qt6::Widgets
)

# For dynamic plugins, we don't link against Stellarium libraries
# The plugin will be loaded by Stellarium at runtime
# We only need the headers for compilation
# Use undefined symbols that will be resolved at runtime
if(APPLE)
	target_link_options(MoonAvoidance PRIVATE
		-undefined dynamic_lookup
	)
endif()

# Install plugin
if(DEFINED STELLARIUM_PLUGIN_INSTALL_DIR)
	install(TARGETS MoonAvoidance
		LIBRARY DESTINATION ${STELLARIUM_PLUGIN_INSTALL_DIR}
	)
	
	# Install plugin.ini
	install(FILES plugin.ini
		DESTINATION ${STELLARIUM_PLUGIN_INSTALL_DIR}
	)
else()
	# Default installation location for dynamic plugins
	if(APPLE)
		set(PLUGIN_INSTALL_DIR "$ENV{HOME}/Library/Application Support/Stellarium/modules/MoonAvoidance")
	elseif(UNIX)
		set(PLUGIN_INSTALL_DIR "$ENV{HOME}/.stellarium/modules/MoonAvoidance")
	elseif(WIN32)
		set(PLUGIN_INSTALL_DIR "$ENV{APPDATA}/Stellarium/modules/MoonAvoidance")
	endif()
	
	install(TARGETS MoonAvoidance
		LIBRARY DESTINATION ${PLUGIN_INSTALL_DIR}
	)
	
	install(FILES plugin.ini
		DESTINATION ${PLUGIN_INSTALL_DIR}
	)
endif()

