#-------------------------------------------------------------------------------
# scripting
#-------------------------------------------------------------------------------

nebula_begin_module(scripting)
fips_libs(foundation application)

fips_files(scriptserver.cc scriptserver.h bindings.h bindings.cc
)
fips_dir(python)
fips_files(pythonserver.cc pythonserver.h deargui.cpp conversion.h conversion.cc)
nebula_end_module()
get_filename_component(PY_DBG_PATH ${Python_LIBRARY_DEBUG} DIRECTORY)
get_filename_component(PY_OPT_PATH ${Python_LIBRARY_RELEASE} DIRECTORY)
target_link_options(scripting PUBLIC "/WHOLEARCHIVE:scripting")
target_include_directories(scripting PUBLIC ${Python_INCLUDE_DIR})
target_link_directories(scripting PUBLIC ${PY_DBG_PATH} ${PY_OPT_PATH})
target_compile_options(scripting PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
