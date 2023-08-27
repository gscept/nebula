#-------------------------------------------------------------------------------
# assetbatcher
#-------------------------------------------------------------------------------
fips_begin_app(assetbatcher cmdline)
    
    target_include_directories(assetbatcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    target_precompile_headers(assetbatcher REUSE_FROM toolkitutil)
    fips_deps(foundation toolkitutil toolkit-common)
        fips_files(
            assetbatcher.cc
            assetbatcherapp.cc
            assetbatcherapp.h
        )
fips_end_app()