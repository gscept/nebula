#-------------------------------------------------------------------------------
# test script
#-------------------------------------------------------------------------------
fips_begin_app(testscript cmdline)
fips_files(testscript.cc scripttestapplication.h scripttestapplication.cc)
fips_deps(foundation testbase scripting dynui)
#fips_libs(pybind11::embed)
fips_end_app()
