# bgfx.cmake - bgfx building in cmake
# Written in 2017 by Joshua Brookover <joshua.al.brookover@gmail.com>

# To the extent possible under law, the author(s) have dedicated all copyright
# and related and neighboring rights to this software to the public domain
# worldwide. This software is distributed without any warranty.

# You should have received a copy of the CC0 Public Domain Dedication along with
# this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

if( TARGET ib-compress )
	return()
endif()

file( GLOB IB_COMPRESS_SOURCES 
	${VOXELBAND_DIR}/extlibs/ib-compress/*.cpp 
	${VOXELBAND_DIR}/extlibs/ib-compress/*.h 
	)

add_library( ib-compress STATIC ${IB_COMPRESS_SOURCES} )
target_include_directories( ib-compress PUBLIC ${VOXELBAND_DIR}/extlibs )
set_target_properties( ib-compress PROPERTIES FOLDER "extlibs" )
