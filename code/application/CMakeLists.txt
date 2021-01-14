#-------------------------------------------------------------------------------
# Game
#-------------------------------------------------------------------------------

nebula_begin_module(application)
    add_nebula_blueprints()
    fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CODE_ROOT}/audio)
    fips_deps(foundation render audio resource physics scripting dynui memdb)
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
            featureunit.h
            featureunit.cc
            gameserver.h
            gameserver.cc
            manager.h
            manager.cc
            propertyserialization.h
            propertyserialization.cc
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
                blueprintmanager.h
                blueprintmanager.cc
                timemanager.h
                timemanager.cc
            )
        fips_dir(basegamefeature/messages)
            nebula_add_nidl(
                entitymessages.nidl
            )
        fips_dir(basegamefeature/properties)
            nebula_add_nidl(
                owner.nidl
                transform.nidl
            )
    fips_dir(graphicsfeature)
        fips_files(
            graphicsfeatureunit.h
            graphicsfeatureunit.cc
        )
        fips_dir(graphicsfeature/managers)
            fips_files(
                graphicsmanager.h
                graphicsmanager.cc
                cameramanager.h
                cameramanager.cc
            )
        fips_dir(graphicsfeature/properties)
            nebula_add_nidl(
                graphics.nidl
            )
    fips_dir(physicsfeature)
        fips_files(
            physicsfeatureunit.h
            physicsfeatureunit.cc
        )
        fips_dir(physicsfeature/managers)
            fips_files(
                physicsmanager.h
                physicsmanager.cc
            )
        fips_dir(physicsfeature/properties)
            nebula_add_nidl(
                physics.nidl
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
