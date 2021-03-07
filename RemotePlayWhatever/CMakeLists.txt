# CMakeList.txt : CMake project for RemotePlayWhatever, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# set OpenSteamworks paths here
set(OSW_INCLUDE_PATH "..\\..\\open-steamworks\\OpenSteamworks")
set(OSW_LIB_PATH "..\\..\\..\\..\\open-steamworks\\OpenSteamAPI\\lib\\Win32\\OpenSteamAPI.lib")

find_package(wxWidgets COMPONENTS core base adv)
if(NOT wxWidgets_FOUND)
	message(FATAL_ERROR "wxWidgets not found!")
endif()

include(${wxWidgets_USE_FILE})

set(APPSRCs	"RemotePlayWhatever.cpp"
			"RemotePlayWhatever.h"
			"SteamStuff.h"
			"SteamStuff.cpp"
			"RemoteApp.h"
			"RemoteApp.cpp"
)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
	add_executable (RemotePlayWhatever WIN32 ${APPSRCs} "appresource.rc")
elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
	add_executable (RemotePlayWhatever ${APPSRCs})
else()
	message(FATAL_ERROR "Unsupported platform!")
endif()

target_compile_definitions(RemotePlayWhatever PRIVATE STEAMWORKS_CLIENT_INTERFACES)
target_include_directories(RemotePlayWhatever PUBLIC ${OSW_INCLUDE_PATH} ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(RemotePlayWhatever ${OSW_LIB_PATH} ${wxWidgets_LIBRARIES})
if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
	target_link_libraries(RemotePlayWhatever ${CMAKE_DL_LIBS})
	set_target_properties(RemotePlayWhatever PROPERTIES OUTPUT_NAME "remoteplaywhatever")
	install(TARGETS RemotePlayWhatever DESTINATION "bin")

	file(READ "/etc/issue" ISSUE)
	string(REGEX MATCH "Debian|Ubuntu" DISTRO ${ISSUE})
	if(DISTRO MATCHES "Ubuntu" OR DISTRO MATCHES "Debian")
		set(CPACK_GENERATOR "DEB")
		set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lets you force steam remote play together for any game")
		set(CPACK_PACKAGE_DESCRIPTION "Tiny app that lets you force steam remote play together for any game")
		set(CPACK_DEBIAN_PACKAGE_MAINTAINER "m4dEngi")
		set(CPACK_DEBIAN_PACKAGE_DEPENDS "libwxbase3.0-0v5 (>= 3.0.4+dfsg), libwxgtk3.0-gtk3-0v5 (>= 3.0.4+dfsg)")
		include(CPack)
	endif()
endif()

