
fips_begin_app(testispc cmdline)
fips_src(. *.* GROUP test ispc)
fips_deps(foundation testbase )
compile_ispc(test.ispc)
target_include_directories(testispc PUBLIC)
target_precompile_headers(testispc REUSE_FROM foundation)
fips_end_app()
