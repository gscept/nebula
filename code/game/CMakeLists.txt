#-------------------------------------------------------------------------------
# Game
#-------------------------------------------------------------------------------

fips_begin_module(game)
	fips_include_directories(. ${CODE_ROOT}/foundation)
	fips_deps(foundation)
	fips_dir(.)
		fips_files(
			stdneb.h
			stdneb.cc
			component.h
			component.cc
		)
	fips_dir(game)
		fips_files(
			entity.h
			entity.cc
			entitymanager.h
			entitymanager.cc
		)
	fips_dir(component)
		fips_files(
			componentdata.h
			componentdata.cc
			componentplugin.h
			componentplugin.cc
		)
	fips_dir(component/plugins)
		fips_files(
			cplugin.h
			cplugin.cc
		)
	fips_dir(.)	
fips_end_module()
