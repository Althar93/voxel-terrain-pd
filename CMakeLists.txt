cmake_minimum_required(VERSION 3.14)
set(CMAKE_C_STANDARD 11)

set(ENVSDK $ENV{PLAYDATE_SDK_PATH})

if (NOT ${ENVSDK} STREQUAL "")
	# Convert path from Windows
	file(TO_CMAKE_PATH ${ENVSDK} SDK)
else()
	execute_process(
			COMMAND bash -c "egrep '^\\s*SDKRoot' $HOME/.Playdate/config"
			COMMAND head -n 1
			COMMAND cut -c9-
			OUTPUT_VARIABLE SDK
			OUTPUT_STRIP_TRAILING_WHITESPACE
	)
endif()

if (NOT EXISTS ${SDK})
	message(FATAL_ERROR "SDK Path not found; set ENV value PLAYDATE_SDK_PATH")
	return()
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release")
set(CMAKE_XCODE_GENERATE_SCHEME TRUE)

# Directories
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Source
file(GLOB SRC
    "src/*.c"
)

# Includes
file(GLOB INCLUDES
    "include/*.h"
)

# Include dirs
include_directories(
    ${INCLUDE_DIR}
)

# Images
file(GLOB IMAGES
	"Source/images/*"
)

# Game Name Customization
set(PLAYDATE_GAME_NAME voxel_terrain)
set(PLAYDATE_GAME_DEVICE voxel_terrain_DEVICE)

project(${PLAYDATE_GAME_NAME} C ASM)

if (TOOLCHAIN STREQUAL "armgcc")
	add_executable(${PLAYDATE_GAME_DEVICE} ${SRC} ${INCLUDES})
else()
	add_library(${PLAYDATE_GAME_NAME} SHARED ${SRC} ${INCLUDES} ${IMAGES})
endif()

# Debugging with simulator
if(WIN32)
	set(VS_DEBUGGER_COMMAND ${PLAYDATE_SDK_PATH}/bin/PlaydateSimulator.exe)
endif()

include(${SDK}/C_API/buildsupport/playdate_game.cmake)

