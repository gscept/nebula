#-------------------------------------------------------------------------------
# dynui module
#-------------------------------------------------------------------------------

fips_begin_module(dynui)
fips_deps(render imgui)
fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/resource ${CODE_ROOT}/render ${CODE_ROOT}/foundation)	
fips_files(
	imguiaddon.cc 
	imguiaddon.h
	imguiinputhandler.cc
	imguiinputhandler.h
	imguirenderer.cc
	imguirenderer.h
	imguirtplugin.cc
	imguirtplugin.h
	)
fips_end_module()