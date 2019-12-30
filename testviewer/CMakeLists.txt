
nebula_begin_app(testviewer cmdline)
fips_src(. *.* GROUP test foundation render)
fips_deps(foundation testbase render dynui)
nebula_end_app()
