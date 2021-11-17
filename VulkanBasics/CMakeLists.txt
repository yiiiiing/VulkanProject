cmake_minimum_required(VERSION 3.20)
project(VulkanBasics)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
add_executable(VulkanBasics main.cpp BasicApplication.cpp BasicApplication.h VulkanHelperFunctions.h BaseObject.cpp BaseObject.h Vertex.h BaseTexture.cpp BaseTexture.h)

# Check environment variables
if (NOT DEFINED ENV{GLFW_HOME})
    message(FATAL_ERROR "found no env named GLFW_HOME")
endif()
if (NOT DEFINED ENV{GLM_HOME})
    message(FATAL_ERROR "found no env named GLM_HOME")
endif()

set(GLFW_HOME $ENV{GLFW_HOME})
set(GLM_HOME $ENV{GLM_HOME})

# GLFW
add_library(glfw SHARED IMPORTED)
SET_TARGET_PROPERTIES(glfw PROPERTIES IMPORTED_LOCATION "${GLFW_HOME}/lib/libglfw.3.dylib")

# GLM
include_directories(${GLM_INCLUDE_DIRS})

# stb
set(STB_INCLUDE_PATH  "~/stb_src/")
include_directories(${STB_INCLUDE_PATH})

# tinyobjloader-master
set(OBJ_LOADER_PATH "~/tinyobjloader-master/")
include_directories(${OBJ_LOADER_PATH})

# Vulkan
find_package(Vulkan REQUIRED FATAL_ERROR)
target_link_libraries(${PROJECT_NAME} glfw Vulkan::Vulkan)
include_directories(${Vulkan_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})





# copy the shader files to the cmake-build-debug folder
file(COPY shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY textures DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY Mesh DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
