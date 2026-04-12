nebula_begin_shared_module(testruntimemodule)
    fips_files(
        runtimemodulefeature.cc
    )
    fips_deps(foundation application)
nebula_end_shared_module()
