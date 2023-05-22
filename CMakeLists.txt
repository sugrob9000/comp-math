cmake_minimum_required(VERSION 3.17)
project(comp LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(src-dir src)
set(libsrc-dir libsrc)
set(exec solver)

find_package(glm REQUIRED)
find_package(SDL2 REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_subdirectory(${libsrc-dir})

file(GLOB_RECURSE src-files CONFIGURE_DEPENDS ${src-dir}/*.cpp ${src-dir}/*.hpp)
add_executable(${exec} ${src-files})

target_compile_features(${exec} PRIVATE cxx_std_20)

target_include_directories(${exec} PRIVATE ${src-dir})
# SYSTEM to ignore warnings in library headers
target_include_directories(${exec} SYSTEM PRIVATE ${libsrc-dir})

target_link_libraries(${exec}
	fmt::fmt imgui glm::glm
	${SDL2_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
	set(cxx-warnings -Wall -Wextra -Wpedantic -Wshadow -Wattributes -Wstrict-aliasing)
	target_compile_options(${exec} PRIVATE ${cxx-warnings} -fmax-errors=1 -fconcepts-diagnostics-depth=10)
	if(CMAKE_BUILD_TYPE STREQUAL Debug)
		target_compile_options(${exec} PRIVATE -fsanitize=undefined -O0)
		target_link_options(${exec} PRIVATE -fsanitize=undefined)
	endif()
endif()

add_custom_target(run
	DEPENDS ${exec}
	COMMAND ${exec}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
