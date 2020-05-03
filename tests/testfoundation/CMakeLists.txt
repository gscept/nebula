#-------------------------------------------------------------------------------
# test foundation
#-------------------------------------------------------------------------------

nebula_begin_app(testfoundation cmdline)
fips_src(. *.* GROUP test foundation)
fips_deps(foundation testbase)
nebula_end_app()