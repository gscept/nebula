fips_reset(neweditor)
add_library(neweditor OBJECT)
fips_ide_group(neweditor)
fips_apply_target_ide_group(neweditor)

# The app compiles with hidden visibility. The entry points the managed side resolves by name have
# to be visible,
# so this module keeps the default
set_target_properties(neweditor PROPERTIES CXX_VISIBILITY_PRESET default)

target_include_directories(neweditor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_precompile_headers(neweditor PRIVATE [["foundation/stdneb.h"]])

# The icon table is generated from the font in the engine's font folder. The name list is the input.
# The output goes to the build folder, with the same path the include reads, so nothing generated
# lands in the source tree.
fips_generate(FROM lucide-icons.txt TYPE lucide_icons OUT_OF_SOURCE
    HEADER neweditor/base/iconregistry.h SOURCE neweditor/base/iconregistry.cc
    ARGS "{ font: ${NROOT}/syswork/data/fonts/lucide.ttf }")

fips_dir(base)
    fips_files(
        drawsurface.h
        drawsurface.cc
        editorlog.h
        editorlog.cc
        iconfont.h
        iconfont.cc
        palette.h
        palette.cc
        timeline.h
        timeline.cc
    )

fips_dir(api)
    fips_files(
        editorfeature.h
        editorfeature.cc
        gizmo.h
        gizmo.cc
        shell.h
        shell.cc
    )

fips_dir(viewtools)
    fips_files(
        toolinterface.h
        toolview.h
        toolview.cc
        toolcontext.h
        toolcontext.cc
        selectiontool.h
        selectiontool.cc
        translatetool.h
        translatetool.cc
        rotatetool.h
        rotatetool.cc
        scaletool.h
        scaletool.cc
        editorcamera.h
        editorcamera.cc
        axisindicator.h
        axisindicator.cc
    )

fips_dir(model)
    fips_files(
        clipboard.h
        clipboard.cc
        editcommand.h
        editcommand.cc
        editcommandmanager.h
        editcommandmanager.cc
        scene.h
        scene.cc
        scenedocument.h
        scenedocument.cc
        selectioncontext.h
        selectioncontext.cc
    )

fips_dir(commands)
    fips_files(
        addcomponentcommand.h
        addcomponentcommand.cc
        createcollectioncommand.h
        createcollectioncommand.cc
        createentitycommand.h
        createentitycommand.cc
        deletecollectioncommand.h
        deletecollectioncommand.cc
        deleteentitycommand.h
        deleteentitycommand.cc
        instantiateprefabcommand.h
        instantiateprefabcommand.cc
        moveentitiestocollectioncommand.h
        moveentitiestocollectioncommand.cc
        pasteentitiescommand.h
        pasteentitiescommand.cc
        propertycommand.h
        propertycommand.cc
        removecomponentcommand.h
        removecomponentcommand.cc
        revertcomponentcommand.h
        revertcomponentcommand.cc
        revertrecordcommand.h
        revertrecordcommand.cc
        setcollectionnamecommand.h
        setcollectionnamecommand.cc
        setcollectionparentcommand.h
        setcollectionparentcommand.cc
        setcomponentcommand.h
        setcomponentcommand.cc
        setentitynamecommand.h
        setentitynamecommand.cc
        setparentcommand.h
        setparentcommand.cc
        setselectioncommand.h
        setselectioncommand.cc
        unpackprefabcommand.h
        unpackprefabcommand.cc
    )

fips_dir(input)
    fips_files(
        editorkeybinds.h
        editorkeybinds.cc
    )

fips_dir(widgets)
    fips_files(
        statusbar.h
        statusbar.cc
        filebrowser.h
        filebrowser.cc
        confirmmodal.h
        confirmmodal.cc
        filebrowsermodal.h
        filebrowsermodal.cc
        commandpalette.h
        commandpalette.cc
        keybindsmodal.h
        keybindsmodal.cc
    )

fips_dir(console)
    fips_files(
        commandscript.h
        commandscript.cc
        editorcommands.h
        editorcommands.cc
        editorconsolehandler.h
        editorconsolehandler.cc
    )

fips_dir(core)
    fips_files(
        editorcore.h
        editorcore.cc
        editorfeatureunit.h
        editorfeatureunit.cc
        editorpanel.h
        editorpanel.cc
        panelsettings.h
        panelsettings.cc
    )

fips_dir(panels)
    fips_files(
        consolepanel.h
        consolepanel.cc
        filebrowserpanel.h
        filebrowserpanel.cc
        inspectorpanel.h
        inspectorpanel.cc
        outlinepanel.h
        outlinepanel.cc
        profilerpanel.h
        profilerpanel.cc
        viewportpanel.h
        viewportpanel.cc
    )

fips_dir(extensions)
    fips_files(
        propertyrecorder.h
        propertyrecorder.cc
    )

fips_dir(imguizmo)
    fips_files(
        imguizmo.h
        imguizmo.cc
    )

fips_deps(foundation application memdb imgui input)

fips_end_lib()
