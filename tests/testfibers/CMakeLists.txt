
nebula_begin_app(testfibers cmdline)
fips_src(. *.* GROUP test foundation)
fips_deps(foundation testbase)
target_precompile_headers(testfibers REUSE_FROM foundation)
nebula_end_app()
