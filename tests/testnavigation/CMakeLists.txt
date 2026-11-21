#-------------------------------------------------------------------------------
# testnavigation
#-------------------------------------------------------------------------------

nebula_begin_app(testnavigation cmdline)
fips_src(. *.* GROUP test)
fips_deps(foundation testbase nflatbuffer physics navbuild Detour)
target_precompile_headers(testnavigation REUSE_FROM foundation)
nebula_end_app()
