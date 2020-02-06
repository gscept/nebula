#-------------------------------------------------------------------------------
# Game
#-------------------------------------------------------------------------------

nebula_begin_module(application)
	fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/audio)
	fips_deps(foundation render audio resource physics attr scripting dynui)
	fips_dir(.)
		fips_files(
			application/stdneb.h
			application/stdneb.cc
		)
	fips_dir(game)
		fips_files(
			entity.h
			entity.cc
			featureunit.h
			featureunit.cc
			gameserver.h
			gameserver.cc
			manager.h
			manager.cc
		)
	fips_dir(game/database)
		fips_files(
			database.h
			database.cc
			table.h
			table.cc
			valuetable.h
			valuetable.cc
		)
	fips_dir(game/messaging)
		fips_files(
			message.h
			message.cc
		)
	fips_dir(appgame)
		fips_files(
			gameapplication.h
			gameapplication.cc
		)
	fips_dir(.)
nebula_end_module()

target_link_options(application PUBLIC "/WHOLEARCHIVE:application")
target_include_directories(application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
