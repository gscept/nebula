#-------------------------------------------------------------------------------
# Game
#-------------------------------------------------------------------------------

nebula_begin_module(application)
	fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/audio)
	fips_deps(foundation render audio resource physics attr scripting dynui)
	fips_dir(.)
	if (FIPS_WINDOWS)
		fips_files(application.natvis)
	endif ()
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
			property.h
			property.cc
		)
	fips_dir(game/database)
		fips_files(
			database.h
			database.cc
			table.h
			table.cc
			statedescription.h
			statedescription.cc
			columndata.h
			columndata.cc
			attribute.h
		)
	fips_dir(game/messaging)
		fips_files(
			message.h
			message.cc
		)
	fips_dir(basegamefeature)
		fips_files(
			basegamefeatureunit.h
			basegamefeatureunit.cc
		)
		fips_dir(basegamefeature/managers)
			fips_files(
				entitymanager.h
				entitymanager.cc
				categorymanager.h
				categorymanager.cc
				factorymanager.h
				factorymanager.cc
				transformmanager.h
				transformmanager.cc
			)
		fips_dir(basegamefeature/properties)
			fips_files(
				transformableproperty.h
				transformableproperty.cc
			)
	fips_dir(graphicsfeature)
		fips_files(
			graphicsfeatureunit.h
			graphicsfeatureunit.cc
		)
		fips_dir(graphicsfeature/properties)
			fips_files(
				graphicsproperty.h
				graphicsproperty.cc
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
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
	target_precompile_headers(application PRIVATE "application/stdneb.h")
endif()
