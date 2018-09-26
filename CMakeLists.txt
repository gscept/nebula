# Nebula Tests

# 3.7.2 required for using FindVulkan
cmake_minimum_required(VERSION 3.7.2)

get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include(${FIPS_ROOT_DIR}/cmake/fips.cmake)

fips_setup(PROJECT nebula-tests)
fips_include_directories(.)
fips_ide_group(tests)
fips_add_subdirectory(testbase)
fips_add_subdirectory(testfoundation)
fips_add_subdirectory(testrender)
fips_add_subdirectory(mathtest)
fips_add_subdirectory(testwin32)
fips_add_subdirectory(testgame)
fips_add_subdirectory(testjobs)
fips_add_subdirectory(testvisibility)
fips_add_subdirectory(testmisc)
fips_add_subdirectory(benchmarks)
fips_finish()
