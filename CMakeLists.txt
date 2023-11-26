cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Voxel_Fluid_Erosion VERSION 0.1)

find_package(OpenMP REQUIRED)


### CMake Options
if (PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(WARNING "The binary directory of CMake cannot be the same as source directory!")
endif()

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# if (WIN32)
#     add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
# endif()

if (NOT MSVC)
    find_program(CCACHE_PROGRAM ccache)
    if (CCACHE_PROGRAM)
        message(STATUS "Found CCache: ${CCACHE_PROGRAM}")
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${CCACHE_PROGRAM})
    endif()
endif()

# message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")
### End CMake options

# Compile dependencies
add_subdirectory(./3rd_party/glfw-3.3.8)
add_subdirectory(./3rd_party/FastNoise2)

# GLM
add_compile_definitions(GLM_LANG_STL11_FORCED) # fix GLM compile error in clang++

# offscreen rendering
if(OFFLINE_RENDERING)
    add_compile_definitions(OFFLINE_RENDERING)
endif()

set(IMGUI_FILES
./3rd_party/imgui/imgui.cpp
./3rd_party/imgui/imgui_draw.cpp
./3rd_party/imgui/imgui_tables.cpp
./3rd_party/imgui/imgui_widgets.cpp
./3rd_party/imgui/imgui_demo.cpp
./3rd_party/imgui/backends/imgui_impl_glfw.cpp # platform
./3rd_party/imgui/backends/imgui_impl_opengl3.cpp # render
)

include_directories(./
                    ./3rd_party
                    ./3rd_party/glad/include
                    ./3rd_party/imgui
                    ./3rd_party/imgui/backends
                    ./3rd_party/FastNoise2/include
                    ./3rd_party/FreeImage
)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/FreeImage/lib)


file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)

list(APPEND SOURCES
    3rd_party/glad/src/glad.c  # glad files
)

add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE include)

target_link_libraries(${PROJECT_NAME} PUBLIC glfw OpenMP::OpenMP_CXX FastNoise FreeImage)

include(CMakePrintHelpers)
cmake_print_properties(
TARGETS
OpenMP::OpenMP_CXX
PROPERTIES
INTERFACE_COMPILE_OPTIONS
INTERFACE_INCLUDE_DIRECTORIES
INTERFACE_LINK_LIBRARIES
)



# set output directories
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE   ${CMAKE_CURRENT_SOURCE_DIR}/bin/Release)
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG     ${CMAKE_CURRENT_SOURCE_DIR}/bin/Debug)
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/bin/Debug)
elseif(UNIX)
endif()

# Copy resources and shaders to target directory
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin/Debug)
else() # Release
    set(OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin/Release)
endif()

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resource
    ${OUTPUT_DIR}/resource
    COMMAND ${CMAKE_COMMAND} -E echo
    "copying ${CMAKE_CURRENT_SOURCE_DIR}/resource to ${OUTPUT_DIR}/resource"
    COMMENT "Copying resources to build directory")

add_custom_command(
    TARGET ${PROJECT_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shader
    ${OUTPUT_DIR}/shader
    COMMAND ${CMAKE_COMMAND} -E echo
    "copying ${CMAKE_CURRENT_SOURCE_DIR}/shader to ${OUTPUT_DIR}/shader"
    COMMENT "Copying shaders to build directory")

file(GLOB SRC_DLLS ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party/FreeImage/lib/*.dll)
add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${SRC_DLLS}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>/FreeImage.dll
        COMMAND ${CMAKE_COMMAND} -E echo
        COMMENT "Copying dlls to build directory")


# Add support for clangd
if (EXISTS "${CMAKE_CURRENT_BINARY_DIR}/compile_commands.json")
    ADD_CUSTOM_COMMAND(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}
    )
endif()


