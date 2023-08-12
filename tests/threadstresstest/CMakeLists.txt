
fips_begin_app(threadstresstest cmdline)
fips_src(. *.* )
fips_deps(foundation testbase)
target_precompile_headers(testjobs REUSE_FROM foundation)
fips_end_app()