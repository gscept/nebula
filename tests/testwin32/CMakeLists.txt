
IF(FIPS_WINDOWS)
fips_begin_app(testwin32 cmdline)
fips_src(. *.* GROUP test foundation)
fips_deps(foundation testbase)
fips_end_app()
ENDIF()