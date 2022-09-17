#-------------------------------------------------------------------------------
# Render
#-------------------------------------------------------------------------------

nebula_begin_module(resource)
    fips_vs_warning_level(3)
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation)
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
target_include_directories(resource PUBLIC ${CODE_ROOT}/resource)
