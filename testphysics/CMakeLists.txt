
nebula_begin_app(testphysics cmdline)
fips_src(. *.* GROUP test foundation render)
fips_deps(foundation testbase render dynui physics)
nebula_end_app()