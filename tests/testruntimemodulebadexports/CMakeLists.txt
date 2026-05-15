nebula_begin_shared_module(testruntimemodulebadexports)
    fips_files(
        badexportsmodule.cc
    )
    fips_deps(foundation application)
nebula_end_shared_module()
