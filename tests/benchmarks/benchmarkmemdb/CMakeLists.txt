#-------------------------------------------------------------------------------
# benchmarkmemdb
#-------------------------------------------------------------------------------

fips_begin_app(benchmarkmemdb cmdline)
fips_src(. *.* GROUP benchmark)
fips_deps(foundation benchmarkbase memdb application)
fips_end_app()