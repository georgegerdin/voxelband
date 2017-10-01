include( CMakeParseArguments )

function( configure_debugging ARG_TARGET )
	if( MSVC )
		# Visual Studio Options
		set(
			OPTIONS
			WORKING_DIR LocalDebuggerWorkingDirectory
			DEBUGGER_ENV LocalDebuggerEnvironment
			COMMAND LocalDebuggerCommand
			COMMAND_ARGS LocalDebuggerCommandArguments
		)

		# Valid Configurations
		set(
			CONFIGS
			Debug
			Release
			MinSizeRel
			RelWithDebInfo
		)

		# Processors
		set(
			PROCESSORS
			Win32
			x64
		)

		# Begin hackery
		if( ${CMAKE_SIZEOF_VOID_P} EQUAL 8 )
			set( ACTIVE_PROCESSOR "x64" )
		else()
			set( ACTIVE_PROCESSOR "Win32" )
		endif()
		# Fix issues with semicolons, thx cmake
		foreach( ARG ${ARGN} )
			string( REPLACE ";" "\\\\\\\\\\\\\\;" RES "${ARG}" )
			list( APPEND ARGS "${RES}" )
		endforeach()
		# Build options for cmake_parse_arguments, result is ONE_ARG variable
		set( ODD ON )
		foreach( OPTION ${OPTIONS} )
			if( ODD )
				set( ARG ${OPTION} )
				list( APPEND ONE_ARG ${ARG} )
				foreach( CONFIG ${CONFIGS} )
					string( TOUPPER ${CONFIG} CONFIG )
					list( APPEND ONE_ARG ${ARG}_${CONFIG} )
					foreach( PROCESSOR ${PROCESSORS} )
						string( TOUPPER ${PROCESSOR} PROCESSOR )
						list( APPEND ONE_ARG ${ARG}_${CONFIG}_${PROCESSOR} )
					endforeach()
				endforeach()
				foreach( PROCESSOR ${PROCESSORS} )
					string( TOUPPER ${PROCESSOR} PROCESSOR )
					list( APPEND ONE_ARG ${ARG}_${PROCESSOR} )
				endforeach()
				set( ODD OFF )
			else()
				set( ODD ON )
			endif()
		endforeach()
		cmake_parse_arguments( ARG "" "${ONE_ARG}" "" ${ARGS} )
		# Parse options, fills in all variables of format ARG_(ARG)_(CONFIG)_(PROCESSOR), for example ARG_WORKING_DIR_DEBUG_X64
		set( ODD ON )
		foreach( OPTION ${OPTIONS} )
			if( ODD )
				set( ARG ${OPTION} )
				foreach( CONFIG ${CONFIGS} )
					string( TOUPPER ${CONFIG} CONFIG_CAP )
					if( "${ARG_${ARG}_${CONFIG_CAP}}" STREQUAL "" )
						set( ARG_${ARG}_${CONFIG_CAP} ${ARG_${ARG}} )
					endif()
					foreach( PROCESSOR ${PROCESSORS} )
						string( TOUPPER ${PROCESSOR} PROCESSOR_CAP )
						if( "${ARG_${ARG}_${CONFIG_CAP}_${PROCESSOR_CAP}}" STREQUAL "" )
							if( "${ARG_${ARG}_${PROCESSOR_CAP}}" STREQUAL "" )
								set( ARG_${ARG}_${CONFIG_CAP}_${PROCESSOR_CAP} ${ARG_${ARG}_${CONFIG_CAP}} )
							else()
								set( ARG_${ARG}_${CONFIG_CAP}_${PROCESSOR_CAP} ${ARG_${ARG}_${PROCESSOR_CAP}} )
							endif()
						endif()
						if( NOT "${ARG_${ARG}_${CONFIG_CAP}_${PROCESSOR_CAP}}" STREQUAL "" )
						endif()
					endforeach()
				endforeach()
				set( ODD OFF )
			else()
				set( ODD ON )
			endif()
		endforeach()
		# Create string to put in proj.vcxproj.user file
		set( RESULT "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<Project ToolsVersion=\"12.0\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">" )
		foreach( CONFIG ${CONFIGS} )
			string( TOUPPER ${CONFIG} CONFIG_CAPS )
			foreach( PROCESSOR ${PROCESSORS} )
				if( "${PROCESSOR}" STREQUAL "${ACTIVE_PROCESSOR}" )
					string( TOUPPER ${PROCESSOR} PROCESSOR_CAPS )
					set( RESULT "${RESULT}\n  <PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='${CONFIG}|${PROCESSOR}'\">" )
					set( ODD ON )
					foreach( OPTION ${OPTIONS} )
						if( ODD )
							set( ARG ${OPTION} )
							set( ODD OFF )
						else()
							set( VALUE ${ARG_${ARG}_${CONFIG_CAPS}_${PROCESSOR_CAPS}} )
							if( NOT "${VALUE}" STREQUAL "" )
								set( RESULT "${RESULT}\n    <${OPTION}>${VALUE}</${OPTION}>" )
							endif()
							set( ODD ON )
						endif()
					endforeach()
					set( RESULT "${RESULT}\n  </PropertyGroup>" )
				endif()
			endforeach()
		endforeach()
		set( RESULT "${RESULT}\n</Project>" )
		file( WRITE ${CMAKE_CURRENT_BINARY_DIR}/${ARG_TARGET}.vcxproj.user "${RESULT}" )
	endif()
endfunction()

function( shaderc_parse ARG_OUT )
	cmake_parse_arguments( ARG "DEPENDS;ANDROID;ASM_JS;IOS;LINUX;NACL;OSX;WINDOWS;PREPROCESS;RAW;FRAGMENT;VERTEX;VERBOSE;DEBUG;DISASM;WERROR" "FILE;OUTPUT;VARYINGDEF;BIN2C;PROFILE;O" "INCLUDES;DEFINES" ${ARGN} )
	set( CLI "" )

	# -f
	if( ARG_FILE )
		list( APPEND CLI "-f" "${ARG_FILE}" )
	endif()

	# -i
	if( ARG_INCLUDES )
#		message(STATUS "ARG_INCLUDES ${ARG_INCLUDES}")
		list( APPEND CLI "-i" )
		set( INCLUDES "" )
		foreach( INCLUDE ${ARG_INCLUDES} )
			if( NOT "${INCLUDES}" STREQUAL "" )
				set( INCLUDES "${INCLUDES}\\\\;${INCLUDE}" )
			else()
				set( INCLUDES "${INCLUDE}" )
			endif()
		endforeach()
		list( APPEND CLI "${INCLUDES}" )
	endif()
#message(STATUS "CLI ${CLI}")
	# -o
	if( ARG_OUTPUT )
		list( APPEND CLI "-o" "${ARG_OUTPUT}" )
	endif()

	# --bin2c
	if( ARG_BIN2C )
		list( APPEND CLI "--bin2c" "${ARG_BIN2C}" )
	endif()

	# --depends
	if( ARG_DEPENDS )
		list( APPEND CLI "--depends" )
	endif()

	# --platform
	set( PLATFORM "" )
	set( PLATFORMS "ANDROID;ASM_JS;IOS;LINUX;NACL;OSX;WINDOWS" )
	foreach( P ${PLATFORMS} )
		if( ARG_${P} )
			if( PLATFORM )
				message( SEND_ERROR "Call to shaderc_parse() cannot have both flags ${PLATFORM} and ${P}." )
				return()
			endif()
			set( PLATFORM "${P}" )
		endif()
	endforeach()
	if( "${PLATFORM}" STREQUAL "" )
		message( SEND_ERROR "Call to shaderc_parse() must have a platform flag: ${PLATFORMS}" )
		return()
	elseif( "${PLATFORM}" STREQUAL "ANDROID" )
		list( APPEND CLI "--platform" "android" )
	elseif( "${PLATFORM}" STREQUAL "ASM_JS" )
		list( APPEND CLI "--platform" "asm.js" )
	elseif( "${PLATFORM}" STREQUAL "IOS" )
		list( APPEND CLI "--platform" "ios" )
	elseif( "${PLATFORM}" STREQUAL "LINUX" )
		list( APPEND CLI "--platform" "linux" )
	elseif( "${PLATFORM}" STREQUAL "NACL" )
		list( APPEND CLI "--platform" "nacl" )
	elseif( "${PLATFORM}" STREQUAL "OSX" )
		list( APPEND CLI "--platform" "osx" )
	elseif( "${PLATFORM}" STREQUAL "WINDOWS" )
		list( APPEND CLI "--platform" "windows" )
	endif()

	# --preprocess
	if( ARG_PREPROCESS )
		list( APPEND CLI "--preprocess" )
	endif()

	# --define
	if( ARG_DEFINES )
		list( APPEND CLI "--defines" )
		set( DEFINES "" )
		foreach( DEFINE ${ARG_DEFINES} )
			if( NOT "${DEFINES}" STREQUAL "" )
				set( DEFINES "${DEFINES}\\\\;${DEFINE}" )
			else()
				set( DEFINES "${DEFINE}" )
			endif()
		endforeach()
		list( APPEND CLI "${DEFINES}" )
	endif()

	# --raw
	if( ARG_RAW )
		list( APPEND CLI "--raw" )
	endif()

	# --type
	set( TYPE "" )
	set( TYPES "FRAGMENT;VERTEX" )
	foreach( T ${TYPES} )
		if( ARG_${T} )
			if( TYPE )
				message( SEND_ERROR "Call to shaderc_parse() cannot have both flags ${TYPE} and ${T}." )
				return()
			endif()
			set( TYPE "${T}" )
		endif()
	endforeach()
	if( "${TYPE}" STREQUAL "" )
		message( SEND_ERROR "Call to shaderc_parse() must have a type flag: ${TYPES}" )
		return()
	elseif( "${TYPE}" STREQUAL "FRAGMENT" )
		list( APPEND CLI "--type" "fragment" )
	elseif( "${TYPE}" STREQUAL "VERTEX" )
		list( APPEND CLI "--type" "vertex" )
	endif()

	# --varyingdef
	if( ARG_VARYINGDEF )
		list( APPEND CLI "--varyingdef" "${ARG_VARYINGDEF}" )
	endif()

	# --verbose
	if( ARG_VERBOSE )
		list( APPEND CLI "--verbose" )
	endif()

	# --debug
	if( ARG_DEBUG )
		list( APPEND CLI "--debug" )
	endif()

	# --disasm
	if( ARG_DISASM )
		list( APPEND CLI "--disasm" )
	endif()

	# --profile
	if( ARG_PROFILE )
		list( APPEND CLI "--profile" "${ARG_PROFILE}" )
	endif()

	# -O
	if( ARG_O )
		list( APPEND CLI "-O" "${ARG_O}" )
	endif()

	# --Werror
	if( ARG_WERROR )
		list( APPEND CLI "--Werror" )
	endif()

	set( ${ARG_OUT} ${CLI} PARENT_SCOPE )
endfunction()


function( add_bgfx_shader FILE FOLDER INCLUDES )
	get_filename_component( FILENAME "${FILE}" NAME_WE )
	string( SUBSTRING "${FILENAME}" 0 2 TYPE )
	if( "${TYPE}" STREQUAL "fs" )
		set( TYPE "FRAGMENT" )
		set( D3D_PREFIX "ps" )
	elseif( "${TYPE}" STREQUAL "vs" )
		set( TYPE "VERTEX" )
		set( D3D_PREFIX "vs" )
	else()
		set( TYPE "" )
	endif()
	if( NOT "${TYPE}" STREQUAL "" )
		message(STATUS "Includes: ${INCLUDES}")
		set( COMMON FILE ${FILE} ${TYPE} INCLUDES ${INCLUDES} )
		set( OUTPUTS "" )
		set( OUTPUTS_PRETTY "" )
		if( WIN32 )
			# dx9
			set( DX9_OUTPUT ${CMAKE_BINARY_DIR}/shaders/dx9/${FILENAME}.bin )
			shaderc_parse( DX9 ${COMMON} WINDOWS PROFILE ${D3D_PREFIX}_3_0 OUTPUT ${DX9_OUTPUT} )
			list( APPEND OUTPUTS "DX9" )
			set( OUTPUTS_PRETTY "${OUTPUTS_PRETTY}DX9, " )

			# dx11
			set( DX11_OUTPUT ${CMAKE_BINARY_DIR}/shaders/dx11/${FILENAME}.bin )
			shaderc_parse( DX11 ${COMMON} WINDOWS PROFILE ${D3D_PREFIX}_4_0 OUTPUT ${DX11_OUTPUT})
			list( APPEND OUTPUTS "DX11" )
			set( OUTPUTS_PRETTY "${OUTPUTS_PRETTY}DX11, " )
		endif()
		if( APPLE )
			# metal
			set( METAL_OUTPUT ${CMAKE_BINARY_DIR}/shaders/metal/${FILENAME}.bin )
			shaderc_parse( METAL ${COMMON} OSX OUTPUT ${METAL_OUTPUT})
			list( APPEND OUTPUTS "METAL" )
			set( OUTPUTS_PRETTY "${OUTPUTS_PRETTY}Metal, " )
		endif()
		# gles
		set( GLES_OUTPUT ${CMAKE_BINARY_DIR}/shaders/gles/${FILENAME}.bin )
		shaderc_parse( GLES ${COMMON} ANDROID OUTPUT ${GLES_OUTPUT})
		list( APPEND OUTPUTS "GLES" )
		set( OUTPUTS_PRETTY "${OUTPUTS_PRETTY}GLES, " )
		# glsl
		set( GLSL_OUTPUT ${CMAKE_BINARY_DIR}/shaders/glsl/${FILENAME}.bin )
		shaderc_parse( GLSL ${COMMON} LINUX PROFILE 120 OUTPUT ${GLSL_OUTPUT})
		list( APPEND OUTPUTS "GLSL" )
		set( OUTPUTS_PRETTY "${OUTPUTS_PRETTY}GLSL" )
		set( OUTPUT_FILES "" )
		set( COMMANDS "" )
		foreach( OUT ${OUTPUTS} )
			list( APPEND OUTPUT_FILES ${${OUT}_OUTPUT} )
			list( APPEND COMMANDS COMMAND "$<TARGET_FILE:bgfx::shaderc>" ${${OUT}} )
			get_filename_component( OUT_DIR ${${OUT}_OUTPUT} DIRECTORY )
			file( MAKE_DIRECTORY ${OUT_DIR} )
		endforeach()
		file( RELATIVE_PATH PRINT_NAME ${CMAKE_BINARY_DIR} ${FILE} )
		add_custom_command(
			MAIN_DEPENDENCY
			${FILE}
			OUTPUT
			${OUTPUT_FILES}
			${COMMANDS}
			COMMENT "Compiling shader ${PRINT_NAME} for ${OUTPUTS_PRETTY} ${COMMANDS}"
		)
	endif()
endfunction()


function( add_target ARG_NAME )
	# Parse arguments
	cmake_parse_arguments( ARG "LIBRARY" "" "SRC_DIRS;INCLUDE_DIRS" ${ARGN} )

	# Get all source files
	list( APPEND ARG_SRC_DIRS "src/${ARG_NAME}" )
	list( APPEND ARG_INCLUDE_DIRS "include/${ARG_NAME}")
#	message(STATUS "ARG_SRC_DIRS: ${ARG_SRC_DIRS}")
#	message(STATUS "ARG_INCLUDE_DIRS: ${ARG_INCLUDE_DIRS}")
	
	set( SOURCES "" )
	set( SHADERS "" )
	foreach( DIR ${ARG_SRC_DIRS} )
		if( APPLE )
			file( GLOB GLOB_SOURCES ${DIR}/*.mm )
			list( APPEND SOURCES ${GLOB_SOURCES} )
		endif()
		file( GLOB GLOB_SOURCES 
			${DIR}/*.c ${DIR}/*.cpp ${DIR}/*.cc
			${DIR}/*.h ${DIR}/*.hpp ${DIR}/*.hh
			${DIR}/*.sc 
			)
		list( APPEND SOURCES ${GLOB_SOURCES} )
		file( GLOB GLOB_SHADERS ${DIR}/*.sc )
		list( APPEND SHADERS ${GLOB_SHADERS} )
	endforeach()
	
	foreach( DIR ${ARG_INCLUDE_DIRS} )
		if( APPLE )
			file( GLOB GLOB_SOURCES ${DIR}/*.mm )
			list( APPEND SOURCES ${GLOB_SOURCES} )
		endif()
		file( GLOB GLOB_SOURCES 
			${DIR}/*.c ${DIR}/*.cpp ${DIR}/*.cc
			${DIR}/*.h ${DIR}/*.hpp ${DIR}/*.hh
			${DIR}/*.sc 
			)
		list( APPEND SOURCES ${GLOB_SOURCES} )
		file( GLOB GLOB_SHADERS ${DIR}/*.sc )
		list( APPEND SHADERS ${GLOB_SHADERS} )
		list( APPEND ARG_SHADER_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/${DIR})
	endforeach()
	#message(STATUS "ARG_SHADER_INCLUDE_DIRS ${ARG_SHADER_INCLUDE_DIRS}")
	# Add target
	if( ARG_LIBRARY )
		add_library( ${ARG_NAME} STATIC EXCLUDE_FROM_ALL ${SOURCES} )
		if( UNIX AND NOT APPLE )
			target_link_libraries( ${ARG_NAME} PUBLIC X11 )
		endif()
	else()
		add_executable( ${ARG_NAME} WIN32 ${SOURCES} )
		
		#target_link_libraries( example-${ARG_NAME} example-common )
		#configure_debugging( ${ARG_NAME} WORKING_DIR ${CMAKE_CURRENT_LIST_DIR} )
		if( MSVC )
			set_target_properties( ${ARG_NAME} PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
		endif()
		
		foreach( SHADER ${SHADERS} )
			add_bgfx_shader( ${SHADER} ${ARG_NAME} ${ARG_SHADER_INCLUDE_DIRS})
		endforeach()
		source_group( "Shader Files" FILES ${SHADERS})
		
	endif()
	#message(STATUS "ARG_INCLUDE_DIRS: ${ARG_INCLUDE_DIRS}")
	
	foreach( DIR ${ARG_INCLUDE_DIRS} )
		target_include_directories( ${ARG_NAME} PUBLIC ${DIR})
	endforeach()
	
	
	target_compile_definitions( ${ARG_NAME} PRIVATE "-D_CRT_SECURE_NO_WARNINGS" "-D__STDC_FORMAT_MACROS" "-DENTRY_CONFIG_IMPLEMENT_MAIN=1" )
endfunction()