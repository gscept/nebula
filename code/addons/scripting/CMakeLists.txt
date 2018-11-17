#-------------------------------------------------------------------------------
# scripting
#-------------------------------------------------------------------------------

fips_begin_module(scripting)
fips_deps(foundation )
fips_include_directories(${PYTHON_INCLUDE_DIRS})
fips_files(scriptserver.cc scriptserver.h	
)
fips_dir(python)
fips_files(pythonserver.cc pythonserver.h)
fips_end_module()
target_include_directories(scripting PUBLIC ${PYTHON_INCLUDE_DIRS})
get_filename_component(pythonfolder "${PYTHON_LIBRARY_RELEASE}" DIRECTORY)
target_link_libraries(scripting ${PYTHON_LIBRARY_DEBUG})