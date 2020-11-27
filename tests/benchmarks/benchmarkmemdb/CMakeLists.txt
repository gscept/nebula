#-------------------------------------------------------------------------------
# benchmarkmemdb
#-------------------------------------------------------------------------------

fips_begin_app(benchmarkmemdb cmdline)
fips_src(. *.* GROUP benchmark)
fips_deps(foundation benchmarkbase memdb)
fips_end_app()