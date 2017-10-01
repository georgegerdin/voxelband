# bgfx.cmake - bgfx building in cmake
# Written in 2017 by Joshua Brookover <joshua.al.brookover@gmail.com>

# To the extent possible under law, the author(s) have dedicated all copyright
# and related and neighboring rights to this software to the public domain
# worldwide. This software is distributed without any warranty.

# You should have received a copy of the CC0 Public Domain Dedication along with
# this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

if( TARGET ocornut-imgui )
	return()
endif()

file( GLOB OCORNUT_IMGUI_SOURCES 
	${VOXELBAND_DIR}/extlibs/ocornut-imgui/*.cpp 
	${VOXELBAND_DIR}/extlibs/ocornut-imgui/*.h 
	${VOXELBAND_DIR}/extlibs/ocornut-imgui/*.inl 
	)

find_package(bx)

add_library( ocornut-imgui STATIC EXCLUDE_FROM_ALL ${OCORNUT_IMGUI_SOURCES} )
target_compile_definitions( ocornut-imgui PRIVATE 
	"-D_CRT_SECURE_NO_WARNINGS" 
	"-D__STDC_FORMAT_MACROS" 
	)
		
target_include_directories( ocornut-imgui PUBLIC ${VOXELBAND_DIR}/extlibs )
target_link_libraries( ocornut-imgui PUBLIC bx::bx )
set_target_properties( ocornut-imgui PROPERTIES FOLDER "extlibs" )
