# Locate Vulkan headers and binaries

if (WIN32)
    find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h PATHS
        "$ENV{VULKAN_SDK}/Include")
		
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        find_library(VULKAN_LIBRARY NAMES vulkan-1 PATHS
            "$ENV{VULKAN_SDK}/Lib" NO_DEFAULT_PATH)
    else()
        find_library(VULKAN_LIBRARY NAMES vulkan-1 PATHS
            "$ENV{VULKAN_SDK}/Lib32" NO_DEFAULT_PATH)
    endif()
else()
    find_path(VULKAN_INCLUDE_DIR NAMES vulkan/vulkan.h PATHS
        "$ENV{VULKAN_SDK}/include")
    find_library(VULKAN_LIBRARY NAMES vulkan PATHS
        "$ENV{VULKAN_SDK}/lib")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vulkan DEFAULT_MSG VULKAN_LIBRARY VULKAN_INCLUDE_DIR)

mark_as_advanced(VULKAN_INCLUDE_DIR VULKAN_LIBRARY)