cmake_minimum_required(VERSION 3.7)
set(CMAKE_MODULE_PATH /home/user/Libs/cmake)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -fpermissive")

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

# Include glew
include_directories("${PROJECT_SOURCE_DIR}/lib/glew/include")

# Include glew librarys
IF (WIN32)
    set(GLEW_LIBRARY ${PROJECT_SOURCE_DIR}/lib/glew/lib/libglew32.a) # Windows
ELSE()
    set(GLEW_LIBRARY ${PROJECT_SOURCE_DIR}/lib/glew/lib/libGLEW.a) # Linux
ENDIF()

# Include and make glfw subproject
include_directories(${PROJECT_SOURCE_DIR}/lib/glfw/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glfw)

# Include and make glm subproject
include_directories(${PROJECT_SOURCE_DIR}/lib/glm/include)
add_subdirectory(${PROJECT_SOURCE_DIR}/lib/glm)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES} ${COMMON})

# Link the external libraries
target_link_libraries(${PROJECT_NAME} glfw glm ${GLEW_LIBRARY} ${OPENGL_LIBRARIES})

# Copy shaders to the build directory
set(SHADER_SUBDIRECTORY resources/shaders)
file(GLOB shaders ${CMAKE_SOURCE_DIR}/${SHADER_SUBDIRECTORY}/*)
foreach(shader ${shaders})
    get_filename_component(outputFileName ${shader} NAME)
    configure_file(${shader} ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_SUBDIRECTORY}/${outputFileName} COPYONLY)
endforeach()
