fips_begin_lib(editor)
fips_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
fips_include_directories(${CODE_ROOT}/foundation)
fips_dir(.)

    file(GLOB_RECURSE py_files RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/scripts/" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.py" )
    
    if(EXISTS ${depoutput})
        file(READ ${depoutput} deps)
    endif()

    get_filename_component(project_folder_name ${FIPS_PROJECT_DIR} NAME_WE)
    SET(script_output_dir ${FIPS_DEPLOY_DIR}/${project_folder_name}/${FIPS_CONFIG}/editorscripts)

    foreach(py_file ${py_files})
        add_custom_command(OUTPUT ${FIPS_DEPLOY_DIR}/scripts/${py_file}
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/scripts/${py_file} ${script_output_dir}/${py_file}
            MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/scripts/${py_file}
            DEPENDS ${deps}
            COMMENT "Copying ${py_file} to ${script_output_dir}/${py_file}"
            VERBATIM
        )
    endforeach()

    fips_files_ex(scripts/ *.py GROUP "scripts")

fips_dir(editorfeature)
    fips_files(
        editorfeatureunit.cc
        editorfeatureunit.h
    )
fips_dir(editor)
    fips_files(
        editor.cc
        editor.h
        cmds.cc
        cmds.h
        command.h
        command.cc
        commandmanager.h
        commandmanager.cc
        entityloader.h
        entityloader.cc
    )
    fips_dir(editor/bindings)
        fips_files(
            editorbindings.cc
            editorbindings.h
        )
    fips_dir(editor/ui)
        fips_files(
            uimanager.cc
            uimanager.h
            window.cc
            window.h
            windowserver.cc
            windowserver.h
        )
        fips_dir(editor/ui/dialogs)
            fips_files(
                filedialog.cc
                filedialog.h
            )
        fips_dir(editor/ui/windows)
            fips_files(
                assetbrowser.cc
                assetbrowser.h
                console.cc
                console.h
                scriptedwindow.cc
                scriptedwindow.h
                scene.cc
                scene.h
                history.cc
                history.h
                outline.cc
                outline.h
                previewer.cc
                previewer.h
                styleeditor.cc
                styleeditor.h
                toolbar.cc
                toolbar.h
                inspector.cc
                inspector.h
            )
        fips_dir(editor/ui/modules)
            fips_files(
                viewport.cc
                viewport.h
            )
    fips_dir(editor/tools)
        fips_files(
            camera.cc
            camera.h
            selectiontool.cc
            selectiontool.h
        )
fips_deps(foundation application graphicsfeature physicsfeature audio)
fips_end_lib()
