
nebula_begin_app(testaddon cmdline)
fips_src(. *.* GROUP test)
fips_deps(foundation testbase db)
nebula_end_app()
