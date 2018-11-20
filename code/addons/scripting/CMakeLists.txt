#-------------------------------------------------------------------------------
# scripting
#-------------------------------------------------------------------------------

fips_begin_module(scripting)
fips_deps(foundation application)
fips_include_directories(${PYTHON_INCLUDE_DIRS})
fips_files(scriptserver.cc scriptserver.h bindings.h bindings.cc	
)
fips_dir(python)
fips_files(pythonserver.cc pythonserver.h)
fips_end_module()
target_include_directories(scripting PUBLIC ${PYTHON_INCLUDE_DIRS})
target_link_libraries(scripting ${PYTHON_LIBRARY_RELEASE})
target_link_options(scripting PUBLIC "/WHOLEARCHIVE:scripting")
