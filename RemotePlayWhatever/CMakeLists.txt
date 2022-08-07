# CMakeList.txt : CMake project for RemotePlayWhatever, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Windows")
    set(SYSTEM_OS "Win")
elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Linux")
    set(SYSTEM_OS "Linux")
elseif(${CMAKE_HOST_SYSTEM_NAME} MATCHES "Darwin")
    set(SYSTEM_OS "OSX")
else()
    message(FATAL_ERROR "Unsupported platform!")
endif()

if(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL x86_64)
    set(SYSTEM_ARCH "64")
elseif(${CMAKE_HOST_SYSTEM_PROCESSOR} STREQUAL arm64)
    set(SYSTEM_ARCH "ARM64")
else()
    set(SYSTEM_ARCH "32")
endif()

add_custom_target(OpenSteamAPI)
set(OSW_ROOT "${CMAKE_CURRENT_LIST_DIR}/../open-steamworks")
set(OSW_INCLUDE_PATH "${OSW_ROOT}/OpenSteamworks")
set(OSW_LIB_ROOT "${OSW_ROOT}/OpenSteamAPI")
if(${SYSTEM_OS} STREQUAL "Win")
    if(${SYSTEM_ARCH} STREQUAL "64")
        set(OSW_LIB_PATH "${OSW_LIB_ROOT}/lib/${SYSTEM_OS}${SYSTEM_ARCH}/OpenSteamAPI64.lib")
        set(OSW_DLL_PATH "${OSW_LIB_ROOT}/lib/${SYSTEM_OS}${SYSTEM_ARCH}/OpenSteamAPI64.dll")
        add_custom_command(TARGET OpenSteamAPI
            WORKING_DIRECTORY "${OSW_ROOT}"
            COMMAND msbuild /p:Platform=x64 /p:Configuration=Release
            VERBATIM
        )
    else()
        set(OSW_LIB_PATH "${OSW_LIB_ROOT}/lib/${SYSTEM_OS}${SYSTEM_ARCH}/OpenSteamAPI.lib")
        set(OSW_DLL_PATH "${OSW_LIB_ROOT}/lib/${SYSTEM_OS}${SYSTEM_ARCH}/OpenSteamAPI.dll")
        add_custom_command(TARGET OpenSteamAPI
            WORKING_DIRECTORY "${OSW_ROOT}"
            COMMAND msbuild /p:Platform=Win32 /p:Configuration=Release
            VERBATIM
        )
    endif()
else()
    set(OSW_LIB_PATH "${OSW_LIB_ROOT}/lib/${SYSTEM_OS}${SYSTEM_ARCH}/OpenSteamAPI.a")
    add_custom_command(TARGET OpenSteamAPI
        PRE_BUILD
        WORKING_DIRECTORY "${OSW_LIB_ROOT}/src"
        COMMAND make
        VERBATIM
    )
endif()

find_package(wxWidgets COMPONENTS core base adv)
if(NOT wxWidgets_FOUND)
    message(FATAL_ERROR "wxWidgets not found!")
endif()

include(${wxWidgets_USE_FILE})

set(APPSRCs "RemotePlayWhatever.cpp"
            "RemotePlayWhatever.h"
            "SteamStuff.h"
            "SteamStuff.cpp"
            "RemoteApp.h"
            "RemoteApp.cpp"
            "RemotePlayInviteHandler.h"
            "RemotePlayInviteHandler.cpp"
)

if(${SYSTEM_OS} STREQUAL "Win")
    add_executable(RemotePlayWhatever WIN32 ${APPSRCs} "appresource.rc")
    add_custom_command(TARGET RemotePlayWhatever POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${OSW_DLL_PATH} $<TARGET_FILE_DIR:RemotePlayWhatever>
    )
else()
    add_executable(RemotePlayWhatever ${APPSRCs})
endif()
add_dependencies(RemotePlayWhatever OpenSteamAPI)

target_compile_definitions(RemotePlayWhatever PRIVATE STEAMWORKS_CLIENT_INTERFACES)
target_include_directories(RemotePlayWhatever PUBLIC ${OSW_INCLUDE_PATH} ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(RemotePlayWhatever ${OSW_LIB_PATH} ${wxWidgets_LIBRARIES})
if(NOT ${SYSTEM_OS} STREQUAL "Win")
    target_link_libraries(RemotePlayWhatever ${CMAKE_DL_LIBS})
    set_target_properties(RemotePlayWhatever PROPERTIES OUTPUT_NAME "remoteplaywhatever")
    install(TARGETS RemotePlayWhatever DESTINATION "bin")
endif()
if(${SYSTEM_OS} MATCHES "Linux")
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

