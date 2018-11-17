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
