project(MBRadio)
set(PROJECT_NAME "MBRadio")
##BEGIN MBPM_VARIABLES
set(MBPM_DEPENDENCIES 
	"MBPlay"
	"MrBoboSockets"
	"MBMime"
	"MBParsing"
	"MBUtility"
	"MBUnicode"
	"MrBigInt"
	"MBCrypto"
	"cryptopp"
	"MBAudioEngine"
	"MBMedia"
	"ffmpeg#avformat"
	"ffmpeg#avcodec"
	"ffmpeg#avdevice"
	"ffmpeg#avfilter"
	"ffmpeg#avutil"
	"ffmpeg#swresample"
	"ffmpeg#swscale"
	"MBGraphicsEngine"
	"stb"
	"assimp"
	"irrxml"
	"zlib"
	"glfw"
	"gl3w"
	"MBCLI"
	"DiscordSDK"
	"MrPostOGet"
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
$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/ffmpeg//
$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/assimp//include/
$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/gl3w//include/
)
set(MBPM_TARGET_EXTPACKET_LIBRARIES )
set(MBPM_TARGET_COMPILE_OPTIONS )
set(MBPM_TARGET_LINK_OPTIONS )
#MBPM_Functions#variables MBPM_DEPENDENCIES
#MBPM_TARGET_EXTPACKET_LIBRARIES
#MBPM_TARGET_COMPILE_OPTIONS
#MBPM_TARGET_LINK_OPTIONS
#Environment variables MBPM_PACKETS_INSTALL_DIRECTORY
set(MBPM_CXX_FLAGS ${CMAKE_CXX_FLAGS})
set(MBPM_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
set(MBPM_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
macro(MBPM_UpdateLibraries TargetToUpdate LinkType BuildType)
	set(MBPM_TARGET_EXTPACKET_LIBRARIES "")
	foreach(PACKET IN LISTS MBPM_DEPENDENCIES)
		set(NewLibraryName "")
		set(PacketName ${PACKET})
		set(LibraryName ${PACKET})
		if(${PACKET} MATCHES "#")
			string(REPLACE "#" ";" PACKET_PART_LIST ${PACKET})
			list(GET PACKET_PART_LIST 0 PacketName)
			list(GET PACKET_PART_LIST 1 LibraryName)
		endif()
		set(NewLibraryToAdd "$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/${PacketName}/MBPM_Builds/")
		if(${LinkType} STREQUAL "STATIC")
			set(NewLibraryName "${LibraryName}_S")
		elseif(${LinkType} STREQUAL "DYNAMIC")
			set(NewLibraryName "${LibraryName}_D")
		endif()
		if(${BuildType} STREQUAL "DEBUG")
			set(NewLibraryName "${NewLibraryName}D")
		elseif(${BuildType} STREQUAL "RELEASE")
			set(NewLibraryName "${NewLibraryName}R")
		endif()
		if(UNIX)
			set(NewLibraryName "lib${NewLibraryName}.")
			if(${LinkType} STREQUAL "STATIC")
				set(NewLibraryName "${NewLibraryName}a")
			else()
				set(NewLibraryName "${NewLibraryName}so")
			endif()
		else()
			if(${LinkType} STREQUAL "STATIC")
				set(NewLibraryName "${NewLibraryName}.lib")
			else()
				set(NewLibraryName "${NewLibraryName}.dll")				
			endif()
		endif()
		set(NewLibraryToAdd "${NewLibraryToAdd}${NewLibraryName}")
		list(APPEND MBPM_TARGET_EXTPACKET_LIBRARIES ${NewLibraryToAdd})
	endforeach()
endmacro()

macro(MBPM_UpdateTargetVariables TargetToUpdate LinkType BuildType)
	MBPM_UpdateLibraries(${TargetToUpdate} ${LinkType} ${BuildType})
endmacro()

macro(MBPM_UpdateOutput TargetToUpdate)
	set_target_properties(${TargetToUpdate} 
	PROPERTIES 
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MBPM_Builds/"
	)
	#set_target_properties(${TargetToUpdate} PROPERTIES PREFIX "")
	#set_target_properties(${TargetToUpdate} PROPERTIES SUFFIX "")
	set_target_properties(${TargetToUpdate} PROPERTIES OUTPUT_NAME ${TargetToUpdate})
endmacro()

macro(MBPM_SetTargetCompileOptions TargetToUpdate LinkType BuildType)
	set(CMAKE_CXX_FLAGS " ")
	set(CMAKE_CXX_FLAGS_DEBUG " ")
	set(CMAKE_CXX_FLAGS_RELEASE " ")
	set(CMAKE_C_FLAGS " ")
	set(CMAKE_C_FLAGS_DEBUG " ")
	set(CMAKE_C_FLAGS_RELEASE " ")

	if(${BuildType} STREQUAL "DEBUG" AND ${LinkType} STREQUAL "STATIC")
		set(NEW_TARGET_COMPILE_OPTIONS "${MBPM_CXX_FLAGS} ${MBPM_CXX_FLAGS_DEBUG}")
	else()
		set(NEW_TARGET_COMPILE_OPTIONS "${MBPM_CXX_FLAGS} ${MBPM_CXX_FLAGS_RELEASE}")
	endif()
	string(REPLACE " " ";" NEW_TARGET_COMPILE_OPTIONS_LIST ${NEW_TARGET_COMPILE_OPTIONS})
	target_compile_options(${TargetToUpdate} PRIVATE ${NEW_TARGET_COMPILE_OPTIONS_LIST})
	message(${TargetToUpdate} " Compiled with follwing flags: ""${NEW_TARGET_COMPILE_OPTIONS_LIST}")
endmacro()

macro(MBPM_LinkTargetPacketLibraries Target)
	target_link_libraries(${Target} ${MBPM_TARGET_EXTPACKET_LIBRARIES})
	target_link_libraries(${Target} ${MBPM_SystemLibraries})
endmacro()

macro(MBPM_SetTargetIncludeDirectories Target)
	target_include_directories(${Target} PUBLIC 
	"${CMAKE_CURRENT_SOURCE_DIR}/"
	"$ENV{MBPM_PACKETS_INSTALL_DIRECTORY}/"
	)
	target_include_directories(${Target} PUBLIC ${MBPM_DependanciesIncludeDirectories})
endmacro()

macro(MBPM_ApplyTargetConfiguration TargetToUpdate LinkType BuildType)
	MBPM_UpdateTargetVariables(${TargetToUpdate} ${LinkType} ${BuildType})
	MBPM_UpdateOutput(${TargetToUpdate})
	MBPM_SetTargetCompileOptions(${TargetToUpdate} ${LinkType} ${BuildType})
	MBPM_SetTargetIncludeDirectories(${TargetToUpdate})
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

add_library(MBRadio_SD STATIC ${COMMON_FILES})
MBPM_ApplyTargetConfiguration("MBRadio_SD" "STATIC" "DEBUG")
target_compile_features("MBRadio_SD" PRIVATE cxx_std_17)
target_link_libraries("MBRadio_SD" ${COMMON_LIBRARIES})

add_library(MBRadio_SR STATIC ${COMMON_FILES})
MBPM_ApplyTargetConfiguration("MBRadio_SR" "STATIC" "RELEASE")
target_compile_features("MBRadio_SR" PRIVATE cxx_std_17)
target_link_libraries("MBRadio_SR" ${COMMON_LIBRARIES})

add_executable("mbradio" ${EXECUTABLE_SOURCES})
MBPM_ApplyTargetConfiguration("mbradio" "STATIC" "DEBUG")
target_compile_features("mbradio" PUBLIC cxx_std_17)
MBPM_LinkTargetPacketLibraries("mbradio")
message(${MBPM_TARGET_EXTPACKET_LIBRARIES})
target_link_libraries("mbradio" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/discord_game_sdk.dll.lib")
