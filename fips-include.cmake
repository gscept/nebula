fips_ide_group(nebula)
SET(NROOT ${CMAKE_CURRENT_LIST_DIR})
SET(CODE_ROOT ${CMAKE_CURRENT_LIST_DIR}/code)

option(N_USE_PRECOMPILED_HEADERS "Use precompiled headers" OFF)

if(FIPS_WINDOWS)
	option(N_STATIC_BUILD "Use static runtime in windows builds" ON)
	if(N_STATIC_BUILD)
		add_definitions(-D__N_STATIC_BUILD)
	endif()
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX /fp:fast /GS-")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX /fp:fast /GS-")
	SET(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /RTC1 /RTCc")
	SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /RTC1")
    if(N_DEBUG_SYMBOLS)
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
        SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Zi")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG")
    endif()
elseif(FIPS_LINUX)
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -msse4.2 -march=sandybridge -ffast-math -fPIC -fno-trapping-math -funsafe-math-optimizations -ffinite-math-only -mrecip=all -Wall")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse4.2 -march=sandybridge -ffast-math -fPIC -fno-trapping-math -funsafe-math-optimizations -ffinite-math-only -mrecip=all -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4 -mavx -w")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(FIPS_WINDOWS)
	option(N_MATH_DIRECTX "Use DirectXMath" ON)
endif()
if(N_MATH_DIRECTX)
    add_definitions(-D__USE_MATH_DIRECTX)
else()
    add_definitions(-D__USE_VECMATH)
    OPTION(N_USE_AVX "Use AVX instructionset" OFF)
endif()

add_definitions(-DIL_STATIC_LIB=1)

set(N_QT4 OFF)
set(N_QT5 OFF)
set(DEFQT "N_QT4")
set(N_QT ${DEFQT} CACHE STRING "Qt Version")
set_property(CACHE N_QT PROPERTY STRINGS "N_QT4" "N_QT5")
set(${N_QT} ON)

find_package(PythonLibs 3.5 REQUIRED)

#physx

if(NOT PX_TARGET)
    MESSAGE(WARNING "PX_TARGET undefined, select a physx build folder (eg. win.x86_64.vs141)")
endif()

SET(PX_DIR_DEBUG ${FIPS_DEPLOY_DIR}/physx/bin/${PX_TARGET}/debug/)
SET(PX_DIR_RELEASE ${FIPS_DEPLOY_DIR}/physx/bin/${PX_TARGET}/release/)

SET(PX_LIBRARY_NAMES PhysX_64  PhysXCommon_64 PhysXCooking_64  PhysXFoundation_64  PhysXCharacterKinematic_static_64 PhysXExtensions_static_64 PhysXPvdSDK_static_64  PhysXTask_static_64 PhysXVehicle_static_64)
SET(PX_STATIC_NAMES PhysXCharacterKinematic_static_64 PhysXExtensions_static_64 PhysXPvdSDK_static_64  PhysXTask_static_64 PhysXVehicle_static_64)
SET(PX_DEBUG_LIBRARIES)
SET(PX_RELEASE_LIBRARIES)

foreach(CUR_LIB ${PX_LIBRARY_NAMES})
    find_library(PX_DBG_${CUR_LIB} ${CUR_LIB} PATHS ${PX_DIR_DEBUG})
    LIST(APPEND PX_DEBUG_LIBRARIES ${PX_DBG_${CUR_LIB}})
    find_library(PX_REL_${CUR_LIB} ${CUR_LIB} PATHS ${PX_DIR_RELEASE})
    LIST(APPEND PX_RELEASE_LIBRARIES ${PX_REL_${CUR_LIB}})
endforeach()

add_library(PxLibs INTERFACE)
target_link_libraries(PxLibs INTERFACE $<$<CONFIG:Debug>:${PX_DEBUG_LIBRARIES}> $<$<CONFIG:Release>:${PX_RELASE_LIBRARIES}>)
target_include_directories(PxLibs INTERFACE ${FIPS_DEPLOY_DIR}/physx/include)

set(DEF_RENDERER "N_RENDERER_VULKAN")
set(N_RENDERER ${DEF_RENDERER} CACHE STRING "Nebula 3D Render Device")
set_property(CACHE N_RENDERER PROPERTY STRINGS "N_RENDERER_VULKAN" "N_RENDERER_D3D11" "N_RENDERER_OGL4")
set(${N_RENDERER} ON)

if(N_QT5)
	add_definitions(-D__USE_QT5)
endif()

if(N_QT4)
	add_definitions(-D__USE_QT4)
endif()

if(N_RENDERER_VULKAN)
	OPTION(N_VALIDATION "Use Vulkan validation" OFF)
	if (N_VALIDATION)
		add_definitions(-DNEBULAT_VULKAN_VALIDATION)
	endif()
	add_definitions(-DNEBULA_DEFAULT_FRAMESHADER_NAME="vkdebug")
	add_definitions(-D__VULKAN__)
	add_definitions(-DGRAPHICS_IMPLEMENTATION_NAMESPACE=Vulkan)
elseif(N_RENDERER_OGL4)
	add_definitions(-DNEBULA_DEFAULT_FRAMESHADER_NAME="ogl4debug")
	add_definitions(-D__OGL4__)
	add_definitions(-DGRAPHICS_IMPLEMENTATION_NAMESPACE=OpenGL4)
elseif(N_RENDERER_D3D12)
	OPTION(N_VALIDATION "Use D3D12 validation" OFF)
	if (N_VALIDATION)
		add_definitions(-DNEBULAT_D3D12_VALIDATION)
	endif()
	add_definitions(-DNEBULA_DEFAULT_FRAMESHADER_NAME="d3d12debug")
	add_definitions(-D__D3D12__)
	add_definitions(-DGRAPHICS_IMPLEMENTATION_NAMESPACE=D3D12)
elseif(N_RENDERER_METAL)
	OPTION(N_VALIDATION "Use Metal validation" OFF)
	if (N_VALIDATION)
		add_definitions(-DNEBULAT_METAL_VALIDATION)
	endif()
	add_definitions(-DNEBULA_DEFAULT_FRAMESHADER_NAME="metaldebug")
	add_definitions(-D__METAL__)
	add_definitions(-DGRAPHICS_IMPLEMENTATION_NAMESPACE=Metal)
endif()

option(N_BUILD_NVTT "use NVTT" OFF)

option(N_NEBULA_DEBUG_SHADERS "Compile shaders with debug flag" OFF)

macro(add_shaders_intern)
    if(SHADERC)
        if(N_NEBULA_DEBUG_SHADERS)
            set(shader_debug "-debug")
        endif()

        foreach(shd ${ARGN})
            get_filename_component(basename ${shd} NAME_WE)
            get_filename_component(foldername ${shd} DIRECTORY)

            # first calculate dependencies
            set(depoutput ${CMAKE_BINARY_DIR}/shaders/${basename}.dep)
            # create it the first time by force, after that with dependencies
            # since custom command does not want to play ball atm, we just generate it every time
            if(NOT EXISTS ${depoutput} OR ${shd} IS_NEWER_THAN ${depoutput})
                execute_process(COMMAND ${SHADERC} -M -i ${shd} -I ${NROOT}/work/shaders/vk -I ${foldername} -o ${CMAKE_BINARY_DIR} -h ${CMAKE_BINARY_DIR}/shaders/${CurTargetName} -t shader)
            endif()

            # sadly this doesnt work for some reason
            #add_custom_command(OUTPUT ${depoutput}
            #COMMAND ${SHADERC} -M -i ${shd} -I ${NROOT}/work/shaders -I ${foldername} -o ${CMAKE_BINARY_DIR} -t shader
            #DEPENDS ${SHADERC} ${shd}
            #WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
            #COMMENT ""
            #VERBATIM
            #)
            if(EXISTS ${depoutput})
                file(READ ${depoutput} deps)
            endif()

            set(output ${EXPORT_DIR}/shaders/${basename}.fxb)
            add_custom_command(OUTPUT ${output}
                COMMAND ${SHADERC} -i ${shd} -I ${NROOT}/work/shaders/vk -I ${foldername} -o ${EXPORT_DIR} -h ${CMAKE_BINARY_DIR}/shaders/${CurTargetName} -t shader ${shader_debug}
                MAIN_DEPENDENCY ${shd}
                DEPENDS ${SHADERC} ${deps}
                WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
                COMMENT ""
                VERBATIM
                )
            fips_files(${shd})

            SOURCE_GROUP("res\\shaders" FILES ${shd})
        endforeach()
    endif()
endmacro()

macro(nebula_add_nidl)
    SOURCE_GROUP("NIDL Files" FILES ${ARGN})
    set(target_has_nidl 1)
    foreach(nidl ${ARGN})
        get_filename_component(f_abs ${CurDir}${nidl} ABSOLUTE)
        get_filename_component(f_dir ${f_abs} PATH)
        STRING(REPLACE ".nidl" ".cc" out_source ${nidl})
        STRING(REPLACE ".nidl" ".h" out_header ${nidl})
        STRING(FIND "${CMAKE_CURRENT_SOURCE_DIR}"  "/" last REVERSE)
        STRING(SUBSTRING "${CMAKE_CURRENT_SOURCE_DIR}" ${last}+1 -1 folder)
        set(abs_output_folder "${CMAKE_BINARY_DIR}/nidl/${CurTargetName}/${CurDir}")
        add_custom_command(OUTPUT "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_header}"
            PRE_BUILD COMMAND ${PYTHON} ${NROOT}/fips-files/generators/NIDL.py "${f_abs}" "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_header}"
            WORKING_DIRECTORY "${NROOT}"
            MAIN_DEPENDENCY "${f_abs}"
            DEPENDS ${NROOT}/fips-files/generators/NIDL.py
            VERBATIM PRE_BUILD)
        SOURCE_GROUP("${CurGroup}\\Generated" FILES "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_header}" )
        source_group("${CurGroup}" FILES ${f_abs})
        list(APPEND CurSources "${abs_output_folder}/${out_source}" "${abs_output_folder}/${out_header}")
    endforeach()
endmacro()

macro(add_frameshader)
    if(SHADERC)
    foreach(frm ${ARGN})
            get_filename_component(basename ${frm} NAME)
            set(output ${EXPORT_DIR}/frame/${basename})
            add_custom_command(OUTPUT ${output}
                COMMAND ${SHADERC} -i ${frm} -o ${EXPORT_DIR} -t frame
                MAIN_DEPENDENCY ${frm}
                DEPENDS ${SHADERC}
                WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
                COMMENT ""
                VERBATIM
                )
            fips_files(${frm})
            SOURCE_GROUP("res\\frameshaders" FILES ${frm})
        endforeach()
    endif()
endmacro()

macro(add_material)
    if(SHADERC)
    foreach(mat ${ARGN})
            get_filename_component(basename ${mat} NAME)
            set(output ${EXPORT_DIR}/materials/${basename})
            add_custom_command(OUTPUT ${output}
                COMMAND ${SHADERC} -i ${mat} -o ${EXPORT_DIR} -t material
                MAIN_DEPENDENCY ${mat}
                DEPENDS ${SHADERC}
                WORKING_DIRECTORY ${FIPS_PROJECT_DIR}
                COMMENT ""
                VERBATIM
                )
            fips_files(${mat})
            SOURCE_GROUP("res\\materials" FILES ${mat})
        endforeach()
    endif()
endmacro()

macro(add_nebula_shaders)
    if(NOT SHADERC)
        MESSAGE(WARNING "Not compiling shaders, ShaderC not found, did you compile nebula-toolkit?")
    else()
        if(FIPS_WINDOWS)

            get_filename_component(workdir "[HKEY_CURRENT_USER\\SOFTWARE\\gscept\\ToolkitShared;workdir]" ABSOLUTE)
            # get_filename_component returns /registry when a key is not found...
            if(${workdir} STREQUAL "/registry")
                MESSAGE(WARNING "Registry keys for project not found, did you set your workdir?")
                return()
            endif()
            set(EXPORT_DIR "${workdir}/export_win32")
        else()
            # use environment
            set(EXPORT_DIR $ENV{NEBULA_WORK}/export_win32)
        endif()

        file(GLOB_RECURSE FXH "${NROOT}/work/shaders/vk/*.fxh")
        SOURCE_GROUP("res\\shaders\\headers" FILES ${FXH})
        fips_files(${FXH})
        file(GLOB_RECURSE FX "${NROOT}/work/shaders/vk/*.fx")
        foreach(shd ${FX})
            add_shaders_intern(${shd})
        endforeach()

        file(GLOB_RECURSE FRM "${NROOT}/work/frame/win32/*.json")
        foreach(shd ${FRM})
            add_frameshader(${shd})
        endforeach()

         file(GLOB_RECURSE MAT "${NROOT}/work/materials/*.xml")
        foreach(shd ${MAT})
            add_material(${shd})
        endforeach()

    endif()
endmacro()

macro(add_shaders)
    if(NOT SHADERC)
        MESSAGE(WARNING "Not compiling shaders, ShaderC not found, did you compile nebula-toolkit?")
    else()
        if(FIPS_WINDOWS)

            get_filename_component(workdir "[HKEY_CURRENT_USER\\SOFTWARE\\gscept\\ToolkitShared;workdir]" ABSOLUTE)
            # get_filename_component returns /registry when a key is not found...
            if(${workdir} STREQUAL "/registry")
                MESSAGE(WARNING "Registry keys for project not found, did you set your workdir?")
                return()
            endif()
            set(EXPORT_DIR "${workdir}/export_win32")
        else()
            # use environment
            set(EXPORT_DIR $ENV{NEBULA_WORK}/export_win32)
        endif()
        foreach(shd ${ARGN})
            add_shaders_intern(${CMAKE_CURRENT_SOURCE_DIR}/${shd})
        endforeach()

    endif()
endmacro()

macro(nebula_begin_app name type)
    fips_begin_app(${name} ${type})
    set(target_has_nidl 0)
    set(target_has_shaders 0)
endmacro()

macro(nebula_end_app)
    set(curtarget ${CurTargetName})
    fips_end_app()
    if(target_has_nidl)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/nidl/${CurTargetName}")
    endif()
    if (target_has_shaders)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/shaders/${CurTargetName}")
    endif()
endmacro()

macro(nebula_begin_module name)
    fips_begin_module(${name})
    set(target_has_nidl 0)
    set(target_has_shaders 0)
endmacro()

macro(nebula_end_module)
    set(curtarget ${CurTargetName})
    fips_end_module()
    if(target_has_nidl)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/nidl/${CurTargetName}")
    endif()
    if (target_has_shaders)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/shaders/${CurTargetName}")
    endif()
endmacro()

macro(nebula_begin_lib name)
    fips_begin_lib(${name})
    set(target_has_nidl 0)
    set(target_has_shaders 0)
endmacro()

macro(nebula_end_lib)
    set(curtarget ${CurTargetName})
    fips_end_lib()
    if(target_has_nidl)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/nidl/${CurTargetName}")
    endif()
    if (target_has_shaders)
        target_include_directories(${curtarget} PUBLIC "${CMAKE_BINARY_DIR}/shaders/${CurTargetName}")
    endif()
endmacro()
