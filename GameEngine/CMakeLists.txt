# CMakeList.txt : CMake project for GameEngine, include source and define
# project specific logic here.
#
# Add source to this project's executable.


add_executable (GameEngine "main.cpp"  "Window.h" "Window.cpp"   "Vertex.h"
        VulkanCore/VulkanDevice.cpp
        VulkanCore/VulkanDevice.h
        VulkanCore/VulkanRenderer.cpp
        VulkanCore/VulkanRenderer.h
)

add_compile_definitions(PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET GameEngine PROPERTY CXX_STANDARD 20)
endif()


find_package(Vulkan REQUIRED)
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)
target_include_directories(${PROJECT_NAME} PUBLIC ${VULKAN_INCLUDE_DIRS})


set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(Libs/glfw)
target_link_libraries(GameEngine glfw)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(DEBUG_BUILD)
else()
  add_compile_definitions(RELEASE_BUILD)
endif()