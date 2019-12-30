#-------------------------------------------------------------------------------
# dynui module
#-------------------------------------------------------------------------------

nebula_begin_module(dynui)
fips_deps(render imgui scripting)
fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/resource ${CODE_ROOT}/render ${CODE_ROOT}/foundation)	
add_shaders(im3d/im3d.fx imgui.fx)
fips_files(
	imguiinputhandler.cc
	imguiinputhandler.h
	imguicontext.cc
	imguicontext.h    
	)
fips_dir(console)
	fips_files(imguiconsole.cc imguiconsole.h
				imguiconsolehandler.cc imguiconsolehandler.h
		)
fips_dir(im3d)
    fips_files(im3d.h im3d.cpp im3d_config.h im3d_math.h im3dcontext.h im3dcontext.cc)    
fips_dir(graph)
	fips_files(imguigraph.h imguigraph.cc)

nebula_end_module()
