cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
set(PROJECT_NAME GoodBoy)
project(${PROJECT_NAME})

# Function to preserve source tree hierarchy of project
function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build_utils")

# Find SDL2
find_package(SDL2 REQUIRED COMPONENTS main)

# Define executable target
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2main_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCE_DIR
        "*.h"
        "*.cpp"
)
add_executable(${PROJECT_NAME} ${SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBS})

assign_source_group(${SOURCE_DIR})

# Copy DLLs to output folder on Windows
if(WIN32)
    foreach(DLL ${assimp_DLLS} ${SDL2_DLLS} ${LUA_DLLS})
        message("Copying ${DLL} to output folder")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endforeach()
    
endif()

# Enable highest warning levels + treated as errors
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
else(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif(MSVC)
