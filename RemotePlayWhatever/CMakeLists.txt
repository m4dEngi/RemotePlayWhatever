# CMakeList.txt : CMake project for RemotePlayWhatever, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

find_package(wxWidgets COMPONENTS core base adv)

# OpenSteamworks paths
set(OSW_INCLUDE_PATH "..\\..\\open-steamworks\\OpenSteamworks")
set(OSW_LIB_PATH "..\\..\\..\\..\\open-steamworks\\OpenSteamAPI\\lib\\Win32\\OpenSteamAPI.lib")

# Add source to this project's executable.
add_executable (RemotePlayWhatever WIN32
 "RemotePlayWhatever.cpp" 
 "RemotePlayWhatever.h"
 "RemoteInviteHandler.cpp"
 "RemoteInviteHandler.h"
 "ClientContext.h" 
 "ClientContext.cpp"
 "RemoteApp.h" 
 "RemoteApp.cpp"
 "appresource.rc"
)

target_compile_definitions(RemotePlayWhatever PRIVATE STEAMWORKS_CLIENT_INTERFACES)
target_include_directories(RemotePlayWhatever PUBLIC ${OSW_INCLUDE_PATH} ${wxWidgets_INCLUDE_DIRS})
target_link_libraries(RemotePlayWhatever ${OSW_LIB_PATH} ${wxWidgets_LIBRARIES})