#-------------------------------------------------------------------------------
# benchmarkfoundation
#-------------------------------------------------------------------------------

fips_begin_app(benchmarkfoundation cmdline)
fips_src(. *.* GROUP benchmark)
fips_deps(foundation benchmarkbase)
target_precompile_headers(benchmarkfoundation REUSE_FROM foundation)
fips_end_app()