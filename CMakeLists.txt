cmake_minimum_required(VERSION 3.16)

project (ace)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/utility")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/git")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizers")


if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
endif()

message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "CMAKE_ARCHIVE_OUTPUT_DIRECTORY=${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}")
message(STATUS "CMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

include(ECMEnableSanitizers)
include(GetGitRevisionDescription)

# Include the module for checking compiler flags
include(CheckCXXCompilerFlag)

git_describe(GIT_VERSION --tags)

#parse the version information into pieces.
string(REGEX REPLACE "^v([0-9]+)\\..*" "\\1" GIT_VERSION_MAJOR "${GIT_VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" GIT_VERSION_MINOR "${GIT_VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" GIT_VERSION_PATCH "${GIT_VERSION}")
string(REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+(.*)" "\\1" GIT_VERSION_SHA1 "${GIT_VERSION}")
set(VERSION_SHORT "${GIT_VERSION_MAJOR}.${GIT_VERSION_MINOR}.${GIT_VERSION_PATCH}")

message(STATUS "Version ${GIT_VERSION}")
message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

option(BUILD_ENGINE_SHARED "Build as a shared library." ON)
option(BUILD_ENGINE_TESTS "Build the tests" OFF)
option(BUILD_ENGINE_WITH_CODE_STYLE_CHECKS "Build with code style checks." OFF)

set(BUILD_ENGINE_SHARED OFF CACHE BOOL "" FORCE)
if(BUILD_ENGINE_SHARED)
    set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
else()
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
endif()

if(BUILD_ENGINE_WITH_CODE_STYLE_CHECKS)
	set(BUILD_WITH_CODE_STYLE_CHECKS ON)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_SUPPORT_WINDOWS_EXPORT_ALL_SYMBOLS ON)
if(MSVC)
    add_compile_options(/Zc:__cplusplus)
    add_definitions("-D_SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS")
    # Add bigobj to allow larger object files
    #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:REF")

endif()

if(MINGW)
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-all-symbols" )
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols" )
   #add_compile_options("-Wl,--export-all-symbols")
endif()

# Function to check if a compiler flag is supported and add it if so
function(add_flag_if_supported flag)
  # Create a unique variable name for the flag
  string(REPLACE "-" "_" flag_var "${flag}")
  string(REPLACE "," "_" flag_var "${flag_var}")
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" flag_var "${flag_var}")
  set(flag_var "FLAG_SUPPORTED${flag_var}")

  # Check if the compiler supports the flag
  check_cxx_compiler_flag("${flag}" ${flag_var})
  if (${flag_var})
    # If supported, add the flag
    add_compile_options("${flag}")
    message(STATUS "Added compiler flag ${flag}")
  else()
    message(STATUS "Compiler does not support flag ${flag}")
  endif()
endfunction()

# Set compiler flags based on the compiler and platform
if (MSVC)
  # For Microsoft Visual C++, use /bigobj
  add_flag_if_supported("/bigobj")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # For GCC
  if (WIN32)
    # For MinGW or GCC on Windows, use -Wa,-mbig-obj
    add_flag_if_supported("-Wa,-mbig-obj")
  endif()
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # For Clang
  if (WIN32)
    # For Clang on Windows, try /bigobj
    add_flag_if_supported("/bigobj")
  endif()
endif()

# Set the sanitizers you want to enable
#set(ECM_ENABLE_SANITIZERS "address")

# Call the function to enable sanitizers for the current compiler
#enable_sanitizers_for_compiler()


set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
include(InstallRequiredSystemLibraries)

if(MINGW)
    get_filename_component( Mingw_Path ${CMAKE_CXX_COMPILER} PATH )
    file(GLOB MINGW_SYSTEM_RUNTIME_LIBS "${Mingw_Path}/*.dll")
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} ${MINGW_SYSTEM_RUNTIME_LIBS})
endif()
message(STATUS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS})

set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
#include(cmake/modules.cmake)

include(cmake/utility/target_ide_support.cmake)

if (MSVC)
    # Disable iterator debugging in Debug mode
    add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=1>)

endif()

add_subdirectory(deps)
#add_subdirectory(version)
add_subdirectory(engine)
add_subdirectory(editor)
add_subdirectory(game)


file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/engine_data/data/shaders/* ${PROJECT_SOURCE_DIR}/engine_data/data/scripts/*)
add_custom_target(engine_data
                  ALL
                  COMMAND ${CMAKE_COMMAND} -E copy ${BGFX_DIR}/src/bgfx_shader.sh ${PROJECT_SOURCE_DIR}/engine_data/data/shaders
                  COMMAND ${CMAKE_COMMAND} -E copy ${BGFX_DIR}/src/bgfx_compute.sh ${PROJECT_SOURCE_DIR}/engine_data/data/shaders
                  COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/engine_data ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/engine

                  SOURCES ${SOURCES}
)



file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/editor_data/data/shaders/* )
add_custom_target(editor_data
                  ALL
                  COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/editor_data ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/editor

                  SOURCES
                  ${SOURCES}
)



if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
    add_custom_target(copy_system_runtime_libs ALL
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        )
endif()

install(CODE "execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar cfv ${PROJECT_BINARY_DIR}/${GIT_VERSION}.zip --format=zip -- .
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )")

install(CODE "MESSAGE(\"-- Package ${GIT_VERSION}.zip - done\")")
