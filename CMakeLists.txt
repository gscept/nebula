# Nebula Trifid
# main cmake file that sets up fips

cmake_minimum_required(VERSION 3.21)
if(NOT FIPS_IMPORT)
    project(Nebula)
    get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
    include(${FIPS_ROOT_DIR}/cmake/fips.cmake)
    fips_setup()
endif()

SET(CODE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/code)

include_directories(code/addons)
include_directories(extlibs/scripts)
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/extlibs/scripts)

fips_ide_group(Core)
add_subdirectory(code)

fips_ide_group(Tests)
add_subdirectory(tests)

fips_ide_group(Toolkit)
add_subdirectory(toolkit)