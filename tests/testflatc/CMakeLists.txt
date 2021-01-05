
fips_begin_app(testflatc cmdline)
fips_src(. *.* GROUP test flatc)
fips_deps(foundation testbase nflatbuffer)
nebula_flatc(SYSTEM tests/flatc_test.fbs)
fips_include_directories(${CMAKE_BINARY_DIR}/flatbuffer/testflatc)
fips_end_app()
