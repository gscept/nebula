
fips_begin_app(testvisibility cmdline)
fips_src(. *.* GROUP test foundation render)
fips_deps(foundation testbase render imgui dynui)
fips_end_app()