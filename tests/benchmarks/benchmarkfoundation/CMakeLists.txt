#-------------------------------------------------------------------------------
# benchmarkfoundation
#-------------------------------------------------------------------------------

fips_begin_app(benchmarkfoundation cmdline)
fips_src(. *.* GROUP benchmark)
fips_deps(foundation benchmarkbase)
fips_end_app()