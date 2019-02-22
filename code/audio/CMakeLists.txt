#-------------------------------------------------------------------------------
# Audio system
#-------------------------------------------------------------------------------

nebula_begin_module(audio)
	fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/foundation ${CODE_ROOT}/resource)
	fips_deps(foundation resource soloud)
	
	fips_dir(.)
	fips_files(
		audiodevice.h
		audiodevice.cc
		audioserver.h
		audioserver.cc
		audioclip.h
	)

nebula_end_module()

target_link_options(audio PUBLIC "/WHOLEARCHIVE:audio")
target_include_directories(audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})