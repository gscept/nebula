#-------------------------------------------------------------------------------
# Benchmarkbase
#-------------------------------------------------------------------------------

fips_begin_lib(benchmarkbase)
fips_deps(foundation)
fips_src(. *.* GROUP benchmarkbase)
fips_end_lib()
