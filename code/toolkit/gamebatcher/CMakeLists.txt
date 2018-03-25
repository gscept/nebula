#-------------------------------------------------------------------------------
# gamebatcher
#-------------------------------------------------------------------------------

fips_begin_module(gamebatcher)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    fips_deps(foundation toolkitutil)
        fips_files(
            gamebatcher.cc
            gamebatcherapp.cc
            gamebatcherapp.h
        )
fips_end_module()