cmake_minimum_required(VERSION 3.0)

project(CGS_erosion)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(IMGUI_FILES 
./3rd_party/imgui/imgui.cpp 
./3rd_party/imgui/imgui_draw.cpp 
./3rd_party/imgui/imgui_tables.cpp 
./3rd_party/imgui/imgui_widgets.cpp 
./3rd_party/imgui/imgui_demo.cpp 
./3rd_party/imgui/backends/imgui_impl_glfw.cpp # platform
./3rd_party/imgui/backends/imgui_impl_opengl3.cpp # render
)



add_subdirectory(./3rd_party/glfw-3.3.8)

include_directories(./)
include_directories(./source)
include_directories(./3rd_party)
include_directories(./3rd_party/glad/include)

include_directories(./3rd_party/imgui)
include_directories(./3rd_party/imgui/backends)



set(HEADERS
        )

set(SOURCES
	main.cpp
	3rd_party/glad/src/glad.c
        )

add_executable(CGS_erosion ${HEADERS} ${SOURCES} ${IMGUI_FILES})




target_link_libraries(CGS_erosion PUBLIC glfw)

