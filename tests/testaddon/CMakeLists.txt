
nebula_begin_app(testaddon cmdline)
fips_src(. *.* GROUP test)
fips_deps(foundation testbase db)
target_precompile_headers(testaddon REUSE_FROM foundation)
nebula_end_app()
