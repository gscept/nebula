
IF(FIPS_WINDOWS)
nebula_begin_app(testwin32 cmdline)
fips_src(. *.* GROUP test foundation)
fips_deps(foundation testbase)
target_precompile_headers(testwin32 REUSE_FROM foundation)
nebula_end_app()
ENDIF()