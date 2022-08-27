#ifndef FRIENDPANEL_H
#define FRIENDPANEL_H
#include <wx/wx.h>
#include "wxSteamStuff.h"

wxDECLARE_EVENT(FP_LEFT_CLICK, wxCommandEvent);

class FriendPanel : public wxPanel
{
public:
    FriendPanel(wxWindow* parent, CSteamID friendID);
    FriendPanel(wxWindow* parent, wxString label);
    ~FriendPanel();

    CSteamID GetSteamID();
    wxString GetPersonaName();

    void SetPersonaNameOverride(const wxString& name);

protected:
    wxStaticBitmap* m_personaImg;
    wxStaticText* m_personaNameLabel;

    wxSteamID m_steamID;
    wxString m_personaName;

private:

    void OnMouseEnter(wxMouseEvent& event);
    void OnMouseLeave(wxMouseEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnMouseLeftUp(wxMouseEvent& event);

};

#endif // FRIENDPANEL_H
