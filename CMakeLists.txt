# Nebula Trifid

cmake_minimum_required(VERSION 3.4)

get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include(${FIPS_ROOT_DIR}/cmake/fips.cmake)

fips_setup(PROJECT Nebula)

fips_ide_group(Core)
fips_add_subdirectory(code/foundation)
fips_add_subdirectory(code/render)

fips_finish()
