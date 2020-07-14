
fips_begin_app(testrender windowed)
fips_src(. *.* GROUP test foundation render resources)
fips_deps(foundation render resource testbase imgui dynui)
fips_end_app()