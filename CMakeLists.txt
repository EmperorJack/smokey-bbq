cmake_minimum_required(VERSION 3.7)
set(CMAKE_MODULE_PATH /home/user/Libs/cmake)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -fpermissive")

# Profiling
set(PROFILING_ENABLED FALSE)
if(PROFILING_ENABLED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -pg")
endif()

set(PROJECT_NAME SmokeyBBQ)
project(${PROJECT_NAME})

# Include OpenGL
find_package(OpenGL REQUIRED)

# Include common
include_directories(${PROJECT_SOURCE_DIR}/common)
file(GLOB COMMON
    common/*.h
    common/*.cpp
)

# Include source files
include_directories(${PROJECT_SOURCE_DIR}/src)
file(GLOB SOURCES
    src/*.h
    src/*.cpp
)

# Include and make glew subproject
include_directories(${PROJECT_SOURCE_DIR}/lib/glew/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glew)

# Include and make glfw subproject
include_directories(${PROJECT_SOURCE_DIR}/lib/glfw/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glfw)

# Include and add glm header library
include_directories(${PROJECT_SOURCE_DIR}/lib/glm)
file(GLOB EXTERNAL lib/glm/glm/glm.hpp)

# Include and add portaudio library
include_directories(${PROJECT_SOURCE_DIR}/lib/portaudio/include)
link_directories(${PROJECT_SOURCE_DIR}/lib/portaudio/libs)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${COMMON} ${EXTERNAL})

# Link the external libraries
target_link_libraries(${PROJECT_NAME} glfw glew portaudio ${OPENGL_LIBRARIES})

# Copy shaders to the build directory
set(SHADER_SUBDIRECTORY resources/shaders)
file(GLOB shaders ${CMAKE_SOURCE_DIR}/${SHADER_SUBDIRECTORY}/*)
foreach(shader ${shaders})
    get_filename_component(outputFileName ${shader} NAME)
    configure_file(${shader} ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_SUBDIRECTORY}/${outputFileName} COPYONLY)
endforeach()
