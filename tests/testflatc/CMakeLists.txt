
fips_begin_app(testflatc cmdline)
fips_src(. *.* GROUP test flatc)
fips_deps(foundation testbase nflatbuffer)
nebula_flatc(tests/flatc_test.fbs)
target_include_directories(testflatc PUBLIC ${CMAKE_BINARY_DIR}/flatbuffer/testflatc)
fips_end_app()
