#-------------------------------------------------------------------------------
# benchmarkmemdb
#-------------------------------------------------------------------------------

fips_begin_app(benchmarkmemdb cmdline)
fips_src(. *.* GROUP benchmark)
fips_deps(foundation benchmarkbase memdb application)
target_precompile_headers(benchmarkmemdb REUSE_FROM foundation)
fips_end_app()