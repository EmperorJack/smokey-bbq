
cmake_minimum_required(VERSION 2.8)

project(GLEW C)

# enable IDE project folders
SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

find_package(OpenGL REQUIRED)

SET(headers
	"include/GL/glew.h"
	"include/GL/glxew.h"
	"include/GL/wglew.h"
)

SET(sources
	"src/glew.c"
)

# glew lib target
add_library(glew ${headers} ${sources})
set_property(TARGET glew PROPERTY FOLDER "GLEW")

# include dir usage requirements
target_include_directories(glew PUBLIC "${PROJECT_SOURCE_DIR}/include")

# library usage requirements
target_link_libraries(glew PRIVATE ${OPENGL_LIBRARIES})

# system-specific library usage requirements
if(WIN32)
	target_link_libraries(glew PRIVATE kernel32)
endif()

# usage requirements as shared/static lib
if(BUILD_SHARED_LIBS)
	# define macro for building DLL
	target_compile_definitions(glew PRIVATE GLEW_BUILD)
else()
	# define macro for static linking
	target_compile_definitions(glew PUBLIC GLEW_STATIC)
endif()


