# Nebula Trifid
# main cmake file that sets up fips

# 3.7.2 required for using FindVulkan
# 3.13 require for target_link_options, required for scripting atm
if(NOT FIPS_IMPORT)
    cmake_minimum_required(VERSION 3.13)

    get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
    include(${FIPS_ROOT_DIR}/cmake/fips.cmake)
    fips_setup(PROJECT Nebula)
endif()

SET(CODE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/code)

fips_include_directories(code/addons)
fips_include_directories(extlibs/scripts)

fips_ide_group(Core)
fips_add_subdirectory(code)

if(NOT FIPS_IMPORT)
    fips_finish()
endif()