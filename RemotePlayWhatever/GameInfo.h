#ifndef GAMEINFO_H
#define GAMEINFO_H
#include <wx/string.h>
#include <Steamworks.h>

// Name of an installed Steam game, read from its appmanifest file in the Steam
// library folders. Returns "App <id>" when the manifest can't be found.
wxString GetSteamAppName(AppId_t appID);

// Name of a non-Steam game added to Steam (a shortcut such as an emulator),
// read from the signed-in account's shortcuts.vdf. Empty if it isn't found.
wxString GetShortcutName(uint32 shortcutID, uint32 accountID);

// Display name for whatever game is running: Steam games by app name, non-Steam
// shortcuts by the name they have in the Steam library. Empty if unknown.
wxString GetGameName(CGameID game, uint32 accountID);

#endif // GAMEINFO_H
