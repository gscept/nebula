#-------------------------------------------------------------------------------
# test script
#-------------------------------------------------------------------------------
fips_begin_app(testscript cmdline)
fips_files(testscript.cc scripttestapplication.h scripttestapplication.cc)
fips_deps(foundation testbase)
fips_include_directories(${PYTHON_INCLUDE_DIRS})
link_directories("C:\\Program Files\\Python36\\libs")
#fips_libs(pybind11::embed)
fips_end_app()
