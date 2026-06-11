nebula_begin_app(testruntimeloader cmdline)

fips_files(
    main.cc
    runtimemoduletest.cc
    runtimemoduletest.h
)

fips_deps(foundation application testbase)
target_precompile_headers(testruntimeloader PRIVATE [["foundation/stdneb.h"]] [["application/stdneb.h"]])

nebula_end_app()
