cmake_minimum_required(VERSION 3.1)

#-----------------------------------------------------------------------------
#                           GENERAL CONFIGURATION
#-----------------------------------------------------------------------------
project(LibVMI_Baseline VERSION 0.1.0 LANGUAGES C ASM)
set(VERSION "0.1.0")

# build type
set(default_build_type "Debug")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
        STRING "Choose the type of build." FORCE)
    # Set the possible values of build type for cmake-gui
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
        STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#-----------------------------------------------------------------------------
#                              DEPENDENCIES
#-----------------------------------------------------------------------------
# include()
# find_package()

#-----------------------------------------------------------------------------
#                           BUILD TYPES & FLAGS
#-----------------------------------------------------------------------------
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g3 -O0")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Wno-cast-function-type -Werror -O2")

# default hardening flags that have no performance hit
set(HARDENING_LINKER_FLAGS "-Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")
# update default flags
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${HARDENING_LINKER_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${HARDENING_LINKER_FLAGS}")

# strip library in release mode
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s")

#-----------------------------------------------------------------------------
#                             BUILD OPTIONS
#-----------------------------------------------------------------------------
option(VMI_DEBUG "Debug output level" OFF)

# default values
set(MAX_PAGE_CACHE_SIZE "512")

#-----------------------------------------------------------------------------
#                               SOURCES
#-----------------------------------------------------------------------------
include_directories(${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} src)
add_definitions(-DHAVE_CONFIG_H)

add_subdirectory(src)
