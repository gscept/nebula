#-------------------------------------------------------------------------------
# assetbatcher
#-------------------------------------------------------------------------------
fips_begin_app(assetbatcher cmdline)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    fips_deps(foundation toolkitutil toolkit-common)
        fips_files(
            assetbatcher.cc
            assetbatcherapp.cc
            assetbatcherapp.h
        )
fips_end_app()