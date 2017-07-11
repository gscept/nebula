#-------------------------------------------------------------------------------
# Game
#-------------------------------------------------------------------------------

fips_begin_module(game)
	fips_include_directories(.)
	fips_deps(foundation)
	fips_dir(id)
	fips_files(id.h id.cc)
	fips_dir(.)
	fips_dir(component)
	fips_files(componentdata.h componentdata.cc)
	fips_dir(.)	
fips_end_module()

