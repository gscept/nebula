
nebula_begin_app(testviewer cmdline)
fips_src(. *.* EXCEPT work/* GROUP test)
fips_src(scenes/ *.* GROUP scenes)
fips_deps(foundation testbase render dynui)
nebula_end_app()
