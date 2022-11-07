project(MBRadio)
set(PROJECT_NAME "MBRadio")
##BEGIN MBPM_VARIABLES
set(MBPM_DEPENDENCIES 
	"MBPlay"
	"MrPostOGet"
	"MrBoboSockets"
	"MBMime"
	"MBGraphicsEngine"
	"MBCrypto"
	"MBCLI"
	"MBAudioEngine"
	"assimp"
	"MrBigInt"
	"MBUnicode"
	"MBParsing"
	"MBMedia"
	"zlib"
	"stb"
	"irrxml"
	"glfw"
	"gl3w"
	"ffmpeg#avformat"
	"ffmpeg#avcodec"
	"ffmpeg#avdevice"
	"ffmpeg#avfilter"
	"ffmpeg#avutil"
	"ffmpeg#swresample"
	"ffmpeg#swscale"
	"cryptopp"
	"MBUtility"
	"MBSystem"
	"DiscordSDK"
)
set(MBPM_SystemLibraries
Ws2_32.lib
Secur32.lib
Bcrypt.lib
Mfplat.lib
opengl32
Mfuuid.lib
	Strmiids.lib
)
set(MBPM_DependanciesIncludeDirectories
$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/assimp//include/
$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/gl3w//include/
$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg//
)
set(MBPM_TARGET_EXTPACKET_LIBRARIES )
set(MBPM_TARGET_COMPILE_OPTIONS )
set(MBPM_TARGET_LINK_OPTIONS )
#MBPM_Functions
set(MB_BUILD_TYPE "Debug") #Abstraction around CMAKE_BUILD_TYPE potentially being empty
set(MBPM_EXTPACKETS_BUILD_DIRECTORIES )
set(MBPM_EXTPACKETS_LIBRARIES)
macro(MBPM_UpdateLibraries)
	set(MBPM_EXTPACKETS_LIBRARIES "")
	set(MBPM_EXTPACKETS_BUILD_DIRECTORIES "")
	foreach(PACKET IN LISTS MBPM_DEPENDENCIES)
		set(NewLibraryName "")
		set(PacketName ${PACKET})
		set(LibraryName ${PACKET})
		if(${PACKET} MATCHES "#")
			string(REPLACE "#" ";" PACKET_PART_LIST ${PACKET})
			list(GET PACKET_PART_LIST 0 PacketName)
			list(GET PACKET_PART_LIST 1 LibraryName)
		endif()
		set(NewLibraryDirectory "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/${PacketName}/MBPM_Builds/${MB_BUILD_TYPE}/")
		list(APPEND MBPM_EXTPACKETS_LIBRARIES ${LibraryName})
		list(APPEND MBPM_EXTPACKETS_BUILD_DIRECTORIES ${NewLibraryDirectory})
	endforeach()
endmacro()
set(MBPM_VariablesUpdated "FALSE")
macro(MBPM_UpdateVariables)
	if(${MBPM_VariablesUpdated} STREQUAL "FALSE")
		if(NOT ${CMAKE_BUILD_TYPE} STREQUAL "" AND DEFINED CMAKE_BUILD_TYPE)
			set(MB_BUILD_TYPE ${CMAKE_BUILD_TYPE})
		endif()
		MBPM_UpdateLibraries()
	endif()	
	set(MBPM_VariablesUpdated "TRUE")
endmacro()

macro(MBPM_UpdateOutput TargetToUpdate)
	set(BuildDirectorySuffix ${MB_BUILD_TYPE})
	set_target_properties(${TargetToUpdate} 
	PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${BuildDirectorySuffix}/>
	LIBRARY_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${BuildDirectorySuffix}/>
	RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/${BuildDirectorySuffix}/>
	)
endmacro()

macro(MBPM_LinkTargetPacketLibraries Target)
	target_link_directories(${Target} PRIVATE ${MBPM_EXTPACKETS_BUILD_DIRECTORIES})
	target_link_libraries(${Target} ${MBPM_EXTPACKETS_LIBRARIES})
	target_link_libraries(${Target} ${MBPM_SystemLibraries})
endmacro()

macro(MBPM_SetTargetIncludeDirectories Target)
	target_include_directories(${Target} PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"
	"$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/"
	)
	target_include_directories(${Target} PUBLIC ${MBPM_DependanciesIncludeDirectories})
endmacro()

macro(MBPM_ApplyConfiguration TargetToUpdate)
	MBPM_UpdateVariables()
	MBPM_UpdateOutput(${TargetToUpdate})
	MBPM_SetTargetIncludeDirectories(${TargetToUpdate})
	MBPM_LinkTargetPacketLibraries(${TargetToUpdate})
endmacro()
##END MBPM_VARIABLES

set(PROJECT_SOURCES 

	"${CMAKE_CURRENT_SOURCE_DIR}//MBRadioMain.cpp"

	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/achievement_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/activity_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/application_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/image_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/lobby_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/network_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/overlay_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/relationship_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/storage_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/store_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/user_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/voice_manager.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/core.cpp"
	# q"${CMAKE_CURRENT_SOURCE_DIR}/Temp/types.cpp"
)
set(PROJECT_HEADERS 
	"${CMAKE_CURRENT_SOURCE_DIR}//MBRadio.h"
)
set(EXECUTABLE_SOURCES
	"MBRadioMain.cpp"
	"MBRadio.cpp"
	${PROJECT_SOURCES}
)

set(COMMON_FILES ${PROJECT_SOURCES} ${PROJECT_HEADERS})
set(COMMON_DYNAMIC_LIBRARIES 
)
set(COMMON_STATIC_LIBRARIES 
)
set(COMMON_LIBRARIES ${COMMON_STATIC_LIBRARIES} ${COMMON_DYNAMIC_LIBRARIES})

add_library(MBRadioLib STATIC ${COMMON_FILES})
MBPM_ApplyConfiguration("MBRadioLib" "STATIC" "DEBUG")
target_compile_features("MBRadioLib" PRIVATE cxx_std_17)
target_link_libraries("MBRadioLib" ${COMMON_LIBRARIES})

add_executable("mbradio" ${EXECUTABLE_SOURCES})
MBPM_ApplyConfiguration("mbradio")
target_compile_features("mbradio" PUBLIC cxx_std_17)
target_link_libraries("mbradio" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/discord_game_sdk.dll.lib")
