#-------------------------------------------------------------------------------
# benchmarkfoundation
#-------------------------------------------------------------------------------

nebula_begin_app(benchmarkfoundation cmdline)
fips_src(. *.* GROUP benchmark)
fips_deps(foundation benchmarkbase)
target_precompile_headers(benchmarkfoundation REUSE_FROM foundation)
nebula_end_app()