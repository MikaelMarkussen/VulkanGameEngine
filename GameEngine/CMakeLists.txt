# CMakeList.txt : CMake project for GameEngine, include source and define
# project specific logic here.
#
# Add source to this project's executable.

project(GameEngine CXX)

add_executable (GameEngine 
				"main.cpp"  
				"Window.h" 
				"Window.cpp"   
				"Vertex.h"  
)

set_property(TARGET GameEngine PROPERTY CXX_STANDARD 20)
set_property(TARGET GameEngine PROPERTY CXX_STANDARD_REQUIRED ON)

add_compile_definitions(PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_compile_definitions(GameEngine PRIVATE 
    PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    $<$<CONFIG:Debug>:DEBUG_BUILD>
    $<$<CONFIG:Release>:RELEASE_BUILD>
)

find_package(Vulkan REQUIRED)
target_link_libraries(GameEngine PRIVATE Vulkan::Vulkan)

add_subdirectory(Libs/EASTL)
target_link_libraries(GameEngine PRIVATE EASTL)

add_subdirectory(Libs/glfw)
target_link_libraries(GameEngine PRIVATE glfw)



set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)


if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(DEBUG_BUILD)
else()
  add_compile_definitions(RELEASE_BUILD)
endif()