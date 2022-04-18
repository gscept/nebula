#-------------------------------------------------------------------------------
# fbxbatcher
#-------------------------------------------------------------------------------
fips_begin_app(fbxbatcher cmdline)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    fips_deps(foundation toolkitutil toolkit-common)
        fips_files(
            fbxbatcher3.cc
            fbxbatcher3app.cc
            fbxbatcher3app.h
        )
fips_end_app()