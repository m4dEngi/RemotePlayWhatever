#ifndef GAMEINFO_H
#define GAMEINFO_H
#include <wx/string.h>
#include <Steamworks.h>

// Name of an installed Steam game, read from its appmanifest file in the Steam
// library folders. Returns "App <id>" when the manifest can't be found.
wxString GetSteamAppName(AppId_t appID);

#endif // GAMEINFO_H
