# Use project source dir for demo output. We have to set
# the release and debug specific defines too, for MSVC.
if(TB_DEMO_IN_SRC)
    SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR})
    SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR})
endif()

if(CMAKE_SYSTEM MATCHES "Windows")
 add_definitions(-DGLFW_EXPOSE_NATIVE_WIN32)
 add_definitions(-DGLFW_EXPOSE_NATIVE_WGL)
 set(EXTRA_LIBS ${EXTRA_LIBS} winmm)
endif(CMAKE_SYSTEM MATCHES "Windows")

# Configure glfw
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_INSTALL OFF)
#set(GLFW_USE_OPTIMUS_HPG ON)

# Add sources
set(LOCAL_SRCS "")
aux_source_directory(./demo01 LOCAL_SRCS)
aux_source_directory(./platform LOCAL_SRCS)
include_directories(".")

# Add glfw project
target_include_directories(TurboBadgerLib PUBLIC "thirdparty/glfw/include")
include_directories("thirdparty/glfw/include")
add_subdirectory(thirdparty/glfw)

# Platform specific
if(CMAKE_COMPILER_IS_MINGW)
 # Avoid dll dependencies by linking statically.
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -static")
endif(CMAKE_COMPILER_IS_MINGW)

add_executable(TurboBadgerDemo WIN32 ${LOCAL_SRCS})
target_link_libraries(TurboBadgerDemo TurboBadgerLib ${EXTRA_LIBS} glfw ${GLFW_LIBRARIES})
install(TARGETS TurboBadgerDemo
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
