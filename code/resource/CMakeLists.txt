#-------------------------------------------------------------------------------
# Render
#-------------------------------------------------------------------------------

fips_begin_module(resource)
	fips_vs_warning_level(3)
	fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../foundation)
		fips_files(stdneb.h stdneb.cc)
		fips_dir(resources)
			fips_files(
				resource.cc
				resource.h
				resourcecontainer.cc
				resourcecontainer.h
				resourceid.h
				resourceloader.cc
				resourceloader.h
				resourcemanager.cc
				resourcemanager.h
			)
fips_end_module()