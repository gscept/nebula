#-------------------------------------------------------------------------------
# testbase
#-------------------------------------------------------------------------------

fips_begin_lib(testbase)
fips_deps(foundation)
fips_src(. *.* GROUP testbase)
target_precompile_headers(testbase REUSE_FROM foundation)
fips_end_lib()
