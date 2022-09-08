#ifndef FRIENDPANEL_H
#define FRIENDPANEL_H
#include "ClickablePanel.h"
#include "wxSteamStuff.h"

class FriendPanel : public ClickablePanel
{
public:
    FriendPanel(wxWindow* parent, wxWindowID winID, CSteamID friendID);
    ~FriendPanel();

    CSteamID GetSteamID();

    wxString GetDisplayPersonaName();
    wxString GetDisplayPersonaState();

    void SetDisplayPersonaName(const wxString& name);
    void SetDisaplayPersonaState(const wxString& state);

private:
    wxStaticBitmap* m_personaImg;
    wxStaticText* m_personaNameLabel;
    wxStaticText* m_personaStateLabel;

    CSteamID m_steamID;
};

#endif // FRIENDPANEL_H
