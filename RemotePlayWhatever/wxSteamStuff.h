#ifndef WXSTEAMSTUFF_H
#define WXSTEAMSTUFF_H
#include <wx/wx.h>
#include "SteamStuff.h"

class wxSteamID : public wxObject
{
public:
    wxSteamID(CSteamID steamID);
    CSteamID GetSteamID();

private:
    CSteamID m_steamID;
};

wxBitmap SteamImageToBitmap(int imageID);

#endif // WXSTEAMSTUFF_H
