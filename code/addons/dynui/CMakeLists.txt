#-------------------------------------------------------------------------------
# dynui module
#-------------------------------------------------------------------------------

fips_begin_module(dynui)
fips_deps(render imgui)
fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/resource ${CODE_ROOT}/render ${CODE_ROOT}/foundation)	
fips_files(
	imguiinputhandler.cc
	imguiinputhandler.h
	imguicontext.cc
	imguicontext.h
	)
fips_end_module()