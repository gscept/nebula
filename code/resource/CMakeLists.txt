#-------------------------------------------------------------------------------
# Resources
#-------------------------------------------------------------------------------

nebula_begin_module(resource)
    target_include_directories(resource PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/resource)
    target_precompile_headers(resource PRIVATE <foundation/stdneb.h>)
    fips_deps(foundation)
        fips_dir(resources)
            fips_files(
                resource.cc
                resource.h
                resourceid.h
                resourceloaderthread.cc
                resourceloaderthread.h
                resourcesaver.cc
                resourcesaver.h
                resourceserver.cc
                resourceserver.h
                resourceloader.cc
                resourceloader.h
            )
nebula_end_module()
