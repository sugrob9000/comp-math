cmake_minimum_required(VERSION 3.17)
project(task1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

set(src-dir src)
set(libsrc-dir libsrc)
set(exec solver)

file(GLOB_RECURSE src-files CONFIGURE_DEPENDS ${src-dir}/*.cpp)

find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

add_subdirectory(${libsrc-dir})

file(GLOB_RECURSE src-files CONFIGURE_DEPENDS ${src-dir}/*.cpp)
add_executable(${exec} ${src-files})

target_include_directories(${exec} PRIVATE ${src-dir} ${libsrc-dir})

target_link_libraries(${exec}
	fmt imgui
	${SDL2_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
	set(cxx-warnings -Wall -Wextra -Wpedantic -Wshadow -Wattributes -Wstrict-aliasing)
	target_compile_options(${exec} PRIVATE ${cxx-warnings} -fmax-errors=1)

	if(CMAKE_BUILD_TYPE STREQUAL Debug)
		target_compile_options(${exec} PRIVATE -fsanitize=undefined -Og)
		target_link_options(${exec} PRIVATE -fsanitize=undefined)
	endif()
else()
	message(WARNING "Compiler other than GCC - will miss some options")
endif()

add_custom_target(run
	DEPENDS ${exec}
	COMMAND ${exec}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
