CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
project(LDIOpenGL)

set(CMAKE_BUILD_TYPE RELEASE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/modules")

#================= Project Files =================#
#file(GLOB_RECURSE HEADER_FILES include/*.hpp)
#file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.tpp)
#file(GLOB_RECURSE SHADER_FILES shaders/*.glsl)
set(HEADER_FILES
    include/LDIMesh.hpp
    include/LDIModel.hpp
    include/LDIShader.hpp
)

set(SOURCE_FILES
    src/LDIMesh.cpp
    src/LDIModel.cpp
    src/LDIShader.cpp
)

set(SHADER_FILES
    shaders/basic.vert
    shaders/basic.frag
    shaders/ldi_fboPass.vert
    shaders/ldi_fboPass.frag
    shaders/fboPass.vert
    shaders/fboPass.frag
    shaders/initPixelHashTable.glsl
    shaders/fillPixelHashTable.frag
    shaders/initPrefixSum.glsl
    shaders/prefixSum.glsl
)

#================= Libraries Dependancies =================#
find_package(OpenGL REQUIRED)
if (OPENGL_FOUND)
    include_directories(${OPENGL_INCLUDE_DIRS})
    link_libraries(${OPENGL_LIBRARIES})
endif()

find_package(GLEW REQUIRED)
if (GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIRS})
    link_libraries(${GLEW_LIBRARIES})
endif()

#========= Linking with local Assimp =========#
#include_directories(${CMAKE_CURRENT_SOURCE_DIR}/extlib/assimp/include)
#find_library(ASSIMP_LIBRARY assimp ${CMAKE_CURRENT_SOURCE_DIR}/extlib/assimp/lib)
#link_libraries(${ASSIMP_LIBRARY})

add_subdirectory(apps/getLDI)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

add_definitions(-DCMAKE_CWD="${CMAKE_CURRENT_SOURCE_DIR}")
add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES} ${SHADER_FILES})

find_library(ASSIMP_LIBRARY assimp ${CMAKE_CURRENT_SOURCE_DIR}/extlib/assimp/lib)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/extlib/assimp/include)
target_link_libraries(${PROJECT_NAME} PUBLIC ${ASSIMP_LIBRARY})
