#include "wxSteamStuff.h"
#include "FriendPanel.h"

wxDEFINE_EVENT(FP_LEFT_CLICK, wxCommandEvent);

FriendPanel::FriendPanel(wxWindow* parent, CSteamID friendID):
    wxPanel(parent),
    m_steamID(friendID)
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

    rootSizer->Add( m_personaImg, 0, wxALL, 5 );

    wxBoxSizer* textSizer = new wxBoxSizer( wxVERTICAL );

    textSizer->Add( 0, 0, 1, wxEXPAND, 5 );
    if(friendID.IsValid())
    {
        const char* personaName = GClientContext()->SteamFriends()->GetFriendPersonaName(friendID);
        m_personaName = wxString(personaName, wxConvUTF8);
    }
    m_personaNameLabel = new wxStaticText(this, wxID_ANY, m_personaName, wxDefaultPosition, wxDefaultSize, 0 );
    textSizer->Add( m_personaNameLabel, 0, wxALL | wxEXPAND, 5 );
    textSizer->Add( 0, 0, 1, wxEXPAND, 5 );
    rootSizer->Add( textSizer, 1, wxALL|wxEXPAND, 5 );

    SetSizer( rootSizer );
    Layout();

    Bind( wxEVT_ENTER_WINDOW, &FriendPanel::OnMouseEnter, this);
    Bind( wxEVT_LEAVE_WINDOW, &FriendPanel::OnMouseLeave, this);
    Bind( wxEVT_LEFT_DOWN   , &FriendPanel::OnMouseLeftDown, this);
    Bind( wxEVT_LEFT_UP     , &FriendPanel::OnMouseLeftUp, this);

    // ... and children ffs...
    m_personaImg->Bind( wxEVT_LEFT_UP, &FriendPanel::OnMouseLeftUp, this);
    m_personaImg->Bind( wxEVT_LEFT_DOWN, &FriendPanel::OnMouseLeftDown, this);
    m_personaNameLabel->Bind( wxEVT_LEFT_UP, &FriendPanel::OnMouseLeftUp, this);
    m_personaNameLabel->Bind( wxEVT_LEFT_DOWN, &FriendPanel::OnMouseLeftDown, this);

}

FriendPanel::FriendPanel(wxWindow *parent, wxString label):
    wxPanel(parent),
    m_steamID(CSteamID(0, k_EUniversePublic, k_EAccountTypeIndividual))
{
    wxBoxSizer* rootSizer = new wxBoxSizer( wxHORIZONTAL );

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    wxBoxSizer* textSizer = new wxBoxSizer( wxVERTICAL );

    textSizer->Add( 0, 0, 1, wxEXPAND, 5 );

    m_personaNameLabel = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    textSizer->Add( m_personaNameLabel, 0, wxALL | wxEXPAND, 5 );

    textSizer->Add( 0, 0, 1, wxEXPAND, 5 );
    rootSizer->Add( textSizer, 1, wxALL|wxEXPAND, 5 );

    SetSizer( rootSizer );
    Layout();

    Bind( wxEVT_ENTER_WINDOW, &FriendPanel::OnMouseEnter, this);
    Bind( wxEVT_LEAVE_WINDOW, &FriendPanel::OnMouseLeave, this);
    Bind( wxEVT_LEFT_DOWN   , &FriendPanel::OnMouseLeftDown, this);
    Bind( wxEVT_LEFT_UP     , &FriendPanel::OnMouseLeftUp, this);

    m_personaNameLabel->Bind( wxEVT_LEFT_UP, &FriendPanel::OnMouseLeftUp, this);
    m_personaNameLabel->Bind( wxEVT_LEFT_DOWN, &FriendPanel::OnMouseLeftDown, this);
}

FriendPanel::~FriendPanel()
{
}

void FriendPanel::OnMouseEnter(wxMouseEvent& event)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
    m_personaNameLabel->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
    Refresh();
}

void FriendPanel::OnMouseLeave(wxMouseEvent &event)
{
    // check if we're actually leaving panel or just hovering over children
    wxPoint evtPoint = event.GetPosition();
    wxSize parentSize = this->GetSize();
    if( evtPoint.x > 0 && evtPoint.x < parentSize.x &&
        evtPoint.y > 0 && evtPoint.y < parentSize.y
      )
    {
        return;
    }

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    m_personaNameLabel->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    Refresh();
}

void FriendPanel::OnMouseLeftDown(wxMouseEvent &event)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Refresh();
}

void FriendPanel::OnMouseLeftUp(wxMouseEvent &event)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    m_personaNameLabel->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    Refresh();

    wxCommandEvent outEvt(FP_LEFT_CLICK, GetId());
    outEvt.SetEventObject(this);
    outEvt.SetClientData(&m_steamID);
    ProcessWindowEvent(outEvt);
}

CSteamID FriendPanel::GetSteamID()
{
    return m_steamID.GetSteamID();
}

wxString FriendPanel::GetPersonaName()
{
    return m_personaName;
}

void FriendPanel::SetPersonaNameOverride(const wxString &name)
{
    m_personaNameLabel->SetLabel(name);
}

