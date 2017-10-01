cmake_minimum_required(VERSION 3.6)

project(voxelband)

find_package(bgfx)
find_package(Boost 1.65 COMPONENTS filesystem system)

set( VOXELBAND_DIR "${CMAKE_CURRENT_SOURCE_DIR}" CACHE STRING "Location of Voxelband." )

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

include("cmake/utils.cmake")
include("cmake/ocornut-imgui.cmake")
include("cmake/ib-compress.cmake")

add_target(voxelband
	INCLUDE_DIRS
	include/framework
	)
add_target(framework
	LIBRARY
	SRC_DIRS
	src/framework/debugdraw
	src/framework/entry
	src/framework/font
	src/framework/imgui
	INCLUDE_DIRS
	include/framework/debugdraw
	include/framework/entry
	include/framework/font
	include/framework/imgui
	)

target_link_libraries(framework PUBLIC bgfx::bgfx ocornut-imgui ib-compress)
target_link_libraries(voxelband framework ${Boost_LIBRARIES})
target_include_directories(voxelband PRIVATE ${BOOST_INCLUDE_DIRS})
target_compile_features(voxelband PRIVATE  cxx_generic_lambdas cxx_range_for cxx_strong_enums cxx_constexpr)

message(STATUS "@CMAKE_CURRENT_SOURCE_DIR@ = ${CMAKE_CURRENT_SOURCE_DIR}")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.txt ${CMAKE_CURRENT_BINARY_DIR}/config.txt @ONLY)
