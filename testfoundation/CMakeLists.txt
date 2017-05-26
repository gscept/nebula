#-------------------------------------------------------------------------------
# test foundation
#-------------------------------------------------------------------------------

fips_begin_app(testfoundation cmdline)
fips_src(. *.* GROUP test foundation)
fips_deps(foundation testbase)
fips_end_app()