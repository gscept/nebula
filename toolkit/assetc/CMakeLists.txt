#-------------------------------------------------------------------------------
# assetc
#-------------------------------------------------------------------------------
fips_begin_app(assetc cmdline)
    target_include_directories(assetc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/toolkit/toolkitutil)
    target_precompile_headers(assetbatcher REUSE_FROM toolkitutil)
    fips_deps(foundation toolkitutil toolkit-common)
        fips_files(
            assetconverter.cc
            assetconverterapp.cc
            assetconverterapp.h
        )
fips_end_app()
