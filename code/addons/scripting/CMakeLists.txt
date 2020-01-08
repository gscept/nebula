#-------------------------------------------------------------------------------
# scripting
#-------------------------------------------------------------------------------

nebula_begin_module(scripting)
fips_libs(foundation)

fips_files(
    scriptserver.cc
    scriptserver.h
)
fips_dir(python)
    fips_files(
        pythonserver.cc
        pythonserver.h
        conversion.h
        conversion.cc
    )
nebula_end_module()

get_filename_component(PY_DBG_PATH ${Python_LIBRARY_DEBUG} DIRECTORY)
get_filename_component(PY_OPT_PATH ${Python_LIBRARY_RELEASE} DIRECTORY)
target_link_options(scripting PUBLIC "/WHOLEARCHIVE:scripting")
target_link_directories(scripting PUBLIC ${PY_DBG_PATH} ${PY_OPT_PATH})

if (CMAKE_VERSION VERSION_EQUAL 3.16 OR 
    CMAKE_VERSION VERSION_GREATER 3.16)
    target_include_directories(scripting PUBLIC ${_Python_INCLUDE_DIR})
else()
    target_include_directories(scripting PUBLIC ${Python_INCLUDE_DIR})
endif()
