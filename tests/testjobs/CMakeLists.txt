
fips_begin_app(testjobs cmdline)
fips_src(. *.* GROUP test foundation)
fips_deps(foundation testbase)
target_precompile_headers(testjobs REUSE_FROM foundation)
fips_end_app()