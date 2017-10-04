cmake_minimum_required(VERSION 3.7)
set(CMAKE_MODULE_PATH /home/user/Libs/cmake)
set(CMAKE_CXX_STANDARD 11)

# Setup LLVM paths (needed for openmp on OSX)
if (APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L/usr/local/opt/llvm/lib -I/usr/local/opt/llvm/include")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp")

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

# Include common files
include_directories(${PROJECT_SOURCE_DIR}/common)
file(GLOB COMMON
    common/*.hpp
    common/*.cpp
)

# Include source files
include_directories(${PROJECT_SOURCE_DIR}/src)
file(GLOB_RECURSE  SOURCES
    src/*.h
    src/*.cpp
)

# Include and make glew subproject
include_directories(${PROJECT_SOURCE_DIR}/lib/glew/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glew)

# Include and make glfw subproject
include_directories(${PROJECT_SOURCE_DIR}/lib/glfw/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glfw)

# Include the glm header library
include_directories(${PROJECT_SOURCE_DIR}/lib/glm)

# Include and add portaudio library
include_directories(${PROJECT_SOURCE_DIR}/lib/portaudio/include)
link_directories(${PROJECT_SOURCE_DIR}/lib/portaudio/libs)
if (APPLE)
    set(PORT_AUDIO_LIBRARY portaudio_mac)
else()
    set(PORT_AUDIO_LIBRARY portaudio)
endif()

# Include the kiss fft library
include_directories(${PROJECT_SOURCE_DIR}/lib/kissfft)

# Incldue the imgui library
include_directories(${PROJECT_SOURCE_DIR}/lib/imgui)

# Include any external library files
file(GLOB EXTERNAL
    lib/glm/glm/glm.hpp
    lib/kissfft/*.h
    lib/kissfft/*.c
    lib/imgui/*.h
    lib/imgui/*.cpp
)

# Include required frameworks
find_library(coreservices_lib CoreServices)
find_library(corefoundation_lib CoreFoundation)
find_library(audiounit_lib AudioUnit)
find_library(audiotoolbox_lib AudioToolbox)
find_library(coreaudio_lib CoreAudio)

if (APPLE)
    set(frameworks
        ${coreservices_lib}
        ${corefoundation_lib}
        ${audiounit_lib}
        ${audiotoolbox_lib}
        ${coreaudio_lib}
    )
endif()

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${COMMON} ${EXTERNAL})

# Link the external libraries
target_link_libraries(${PROJECT_NAME} glfw glew ${frameworks} ${PORT_AUDIO_LIBRARY} ${OPENGL_LIBRARIES})

# Copy library dlls to the build directory
if (WIN32)
    file(COPY ${PROJECT_SOURCE_DIR}/lib/portaudio/libs/libportaudio-2.dll DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()

# Copy resources to the build directory
set(RESOURCES_SUBDIRECTORY resources)
file(COPY ${CMAKE_SOURCE_DIR}/${RESOURCES_SUBDIRECTORY} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if (APPLE)
    add_custom_command(TARGET ${PROJECT_NAME}
        POST_BUILD COMMAND
        ${CMAKE_INSTALL_NAME_TOOL} -add_rpath /usr/local/opt/llvm/lib
        $<TARGET_FILE:${PROJECT_NAME}>
    )
endif()
