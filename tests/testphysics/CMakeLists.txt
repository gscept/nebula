#-------------------------------------------------------------------------------
# testphysics
#-------------------------------------------------------------------------------

nebula_begin_app(testphysics cmdline)
fips_src(. *.* GROUP test)
fips_deps(foundation testbase physics)
target_precompile_headers(testphysics REUSE_FROM foundation)
nebula_end_app()
