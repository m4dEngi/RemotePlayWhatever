#include "wxSteamStuff.h"
#include "FriendPanel.h"

FriendPanel::FriendPanel(wxWindow* parent, wxWindowID winID, CSteamID friendID):
    ClickablePanel(parent, winID),
    m_steamID(friendID),
    m_personaNameLabel(nullptr),
    m_personaStateLabel(nullptr)
{
    wxBoxSizer* rootSizer = new wxBoxSizer( wxHORIZONTAL );

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    int av = GClientContext()->SteamFriends()->GetSmallFriendAvatar(friendID);
    if(av)
    {
        m_personaImg = new wxStaticBitmap( this, wxID_ANY, SteamImageToBitmap(av), wxDefaultPosition, wxSize( 32,32 ), 0 );
    }
    else
    {
        m_personaImg = new wxStaticBitmap( this, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize( 32,32 ), 0 );
    }

    rootSizer->Add( m_personaImg, 0, wxALL, 10 );

    wxBoxSizer* textSizer = new wxBoxSizer( wxVERTICAL );

    textSizer->Add( 0, 0, 1, wxEXPAND, 5 );
    wxString personaName;
    if(friendID.IsValid())
    {
        personaName = wxString(GClientContext()->SteamFriends()->GetFriendPersonaName(friendID), wxConvUTF8);
    }
    m_personaNameLabel = new wxStaticText(this, wxID_ANY, personaName, wxDefaultPosition, wxDefaultSize, 0 );
    textSizer->Add( m_personaNameLabel, 0, wxALL | wxEXPAND, 5 );

    textSizer->Add( 0, 0, 1, wxEXPAND, 5 );
    rootSizer->Add( textSizer, 1, wxALL|wxEXPAND, 5 );

    SetSizer( rootSizer );
    Layout();

    RBindBaseEvents();
}

FriendPanel::~FriendPanel()
{
}

CSteamID FriendPanel::GetSteamID()
{
    return m_steamID;
}

wxString FriendPanel::GetDisplayPersonaName()
{
    return m_personaNameLabel->GetLabel();
}

wxString FriendPanel::GetDisplayPersonaState()
{
    return m_personaStateLabel->GetLabel();
}

void FriendPanel::SetDisplayPersonaName(const wxString &name)
{
    m_personaNameLabel->SetLabel(name);
}

void FriendPanel::SetDisaplayPersonaState(const wxString &state)
{
    m_personaStateLabel->SetLabel(state);
}

