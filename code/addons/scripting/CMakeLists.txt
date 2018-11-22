#-------------------------------------------------------------------------------
# scripting
#-------------------------------------------------------------------------------

fips_begin_module(scripting)
fips_libs(foundation application)

fips_files(scriptserver.cc scriptserver.h bindings.h bindings.cc	
)
fips_dir(python)
fips_files(pythonserver.cc pythonserver.h)
fips_end_module()
target_link_libraries(scripting ${PYTHON_LIBRARY_RELEASE})
target_link_options(scripting PUBLIC "/WHOLEARCHIVE:scripting")
