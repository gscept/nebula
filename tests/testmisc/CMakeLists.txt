
fips_begin_app(testmisc cmdline)
fips_src(. *.* GROUP test foundation)
fips_deps(foundation testbase)
target_precompile_headers(testmisc REUSE_FROM foundation)
fips_end_app()