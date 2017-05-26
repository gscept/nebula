# Nebula Trifid
# main cmake file that sets up fips

# 3.7.2 required for using FindVulkan
cmake_minimum_required(VERSION 3.7.2)

get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include(${FIPS_ROOT_DIR}/cmake/fips.cmake)

fips_setup(PROJECT Nebula)

fips_include_directories(code/foundation)
fips_include_directories(extlibs/mixed)

fips_ide_group(Core)
fips_add_subdirectory(code/foundation)
fips_add_subdirectory(code/render)

fips_ide_group(Extlibs)
fips_add_subdirectory(extlibs)

fips_finish()
