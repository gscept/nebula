#-------------------------------------------------------------------------------
# Render
#-------------------------------------------------------------------------------

fips_begin_module(resource)
	fips_vs_warning_level(3)
	fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation)
	fips_deps(foundation)
		fips_files(stdneb.h stdneb.cc)
		fips_dir(resources)
			fips_files(
				resource.cc
				resource.h
				resourcecontainer.cc
				resourcecontainer.h
				resourceid.h
				resourcepool.cc
				resourcepool.h
				resourceloaderthread.cc
				resourceloaderthread.h
				resourcemanager.cc
				resourcemanager.h
				resourcememorypool.cc
				resourcememorypool.h
				resourcesaver.cc
				resourcesaver.h
				resourcestreampool.cc
				resourcestreampool.h
			)
fips_end_module()