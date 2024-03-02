#-------------------------------------------------------------------------------
# Audio system
#-------------------------------------------------------------------------------

nebula_begin_module(audio)
    target_include_directories(audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    fips_deps(foundation resource soloud)
    target_precompile_headers(audio PRIVATE <foundation/stdneb.h>)
    
    fips_dir(audio)
    fips_files(
        audiodevice.h
        audiodevice.cc
        audioserver.h
        audioserver.cc
        audioclip.h
    )

nebula_end_module()

if(FIPS_WINDOWS)
    target_link_options(audio PUBLIC "/WHOLEARCHIVE:audio")
endif()

