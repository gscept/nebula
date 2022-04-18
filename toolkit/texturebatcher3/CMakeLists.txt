#-------------------------------------------------------------------------------
# Texturebatcher
#-------------------------------------------------------------------------------

fips_begin_app(texturebatcher cmdline)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    fips_deps(foundation toolkitutil toolkit-common)
        fips_files(
            texturebatcher3.cc
            texturebatcherapp.cc
            texturebatcherapp.h
        )
fips_end_app()