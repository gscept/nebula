
fips_begin_app(testflatc cmdline)
fips_src(. *.* GROUP test flatc)
fips_deps(foundation testbase nflatbuffer)
nebula_flatc(PROJ tests/flatc_test.fbs)
target_include_directories(testflatc PUBLIC ${CMAKE_BINARY_DIR}/flatbuffer/testflatc)
target_precompile_headers(testflatc REUSE_FROM foundation)
fips_end_app()
