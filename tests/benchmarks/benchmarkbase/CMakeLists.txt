#-------------------------------------------------------------------------------
# Benchmarkbase
#-------------------------------------------------------------------------------

fips_begin_lib(benchmarkbase)
fips_deps(foundation)
fips_src(. *.* GROUP benchmarkbase)
target_precompile_headers(benchmarkbase REUSE_FROM foundation)
fips_end_lib()
