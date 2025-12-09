#-------------------------------------------------------------------------------
# Benchmarkbase
#-------------------------------------------------------------------------------

nebula_begin_lib(benchmarkbase)
fips_deps(foundation)
fips_src(. *.* GROUP benchmarkbase)
target_precompile_headers(benchmarkbase REUSE_FROM foundation)
nebula_end_lib()
