cmake_minimum_required(VERSION 3.16)

# Test configuration
enable_testing()

# Find required packages
find_package(Qt6 REQUIRED COMPONENTS Core Test)
find_package(Stellarium REQUIRED)

# Test source files
set(TEST_SOURCES
    testMoonAvoidance.cpp
    testMoonAvoidanceConfig.cpp
)

# Create test executable
add_executable(MoonAvoidanceTests
    ${TEST_SOURCES}
)

target_link_libraries(MoonAvoidanceTests
    Qt6::Core
    Qt6::Test
    Stellarium::StelCore
)

target_include_directories(MoonAvoidanceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# Add tests
add_test(NAME MoonAvoidanceTests COMMAND MoonAvoidanceTests)

