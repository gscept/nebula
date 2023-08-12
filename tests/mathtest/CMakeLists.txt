
nebula_begin_app(testmath windowed)
fips_src(. *.* GROUP test foundation)
fips_deps(foundation testbase)
target_precompile_headers(testmath REUSE_FROM foundation)
nebula_end_app()