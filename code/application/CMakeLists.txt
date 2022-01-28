#-------------------------------------------------------------------------------
# Game
#-------------------------------------------------------------------------------

nebula_begin_module(application)
    add_nebula_blueprints()
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/audio)
    fips_deps(foundation audio resource scripting memdb imgui)
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
            api.h
            api.cc
            category.h
            category.cc
            entity.h
            entity.cc
            entitypool.h
            entitypool.cc
            featureunit.h
            featureunit.cc
            gameserver.h
            gameserver.cc
            manager.h
            manager.cc
            componentserialization.h
            componentserialization.cc
            componentinspection.h
            componentinspection.cc
            world.h
            world.cc
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
                blueprintmanager.h
                blueprintmanager.cc
                timemanager.h
                timemanager.cc
            )
        fips_dir(basegamefeature/components)
            nebula_add_nidl(
                owner.nidl
                transform.nidl
            )
    fips_dir(appgame)
        fips_files(
            gameapplication.h
            gameapplication.cc
        )
    fips_dir(.)
nebula_end_module()

if(FIPS_WINDOWS)
target_link_options(application PUBLIC "/WHOLEARCHIVE:application")
endif()
target_include_directories(application PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.16.0")
    target_precompile_headers(application PRIVATE "application/stdneb.h")
endif()
