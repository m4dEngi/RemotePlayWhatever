#include "ClickablePanel.h"

wxDEFINE_EVENT(BTN_PANEL_LEFT_CLICK, wxCommandEvent);

ClickablePanel::ClickablePanel(wxWindow *parent, wxWindowID winID):
    wxPanel(parent, winID)
{
}

ClickablePanel::~ClickablePanel()
{
}

void ClickablePanel::RSetForegroundColour(const wxColour &colour)
{
    wxWindowList kidz = GetChildren();
    for(auto it = kidz.begin(); it != kidz.end(); ++it)
    {
        (*it)->SetForegroundColour(colour);
    }
}

void ClickablePanel::RSetBackgroundColour(const wxColour &colour)
{
    wxWindowList kidz = GetChildren();
    for(auto it = kidz.begin(); it != kidz.end(); ++it)
    {
        (*it)->SetBackgroundColour(colour);
    }
}

void ClickablePanel::RBindBaseEvents()
{
    Bind( wxEVT_ENTER_WINDOW, &ClickablePanel::OnMouseEnter, this);
    Bind( wxEVT_LEAVE_WINDOW, &ClickablePanel::OnMouseLeave, this);
    Bind( wxEVT_LEFT_DOWN   , &ClickablePanel::OnMouseLeftDown, this);
    Bind( wxEVT_LEFT_UP     , &ClickablePanel::OnMouseLeftUp, this);

    wxWindowList kidz = GetChildren();
    for(auto it = kidz.begin(); it != kidz.end(); ++it)
    {
        (*it)->Bind( wxEVT_LEFT_UP, &ClickablePanel::OnMouseLeftUp, this);
        (*it)->Bind( wxEVT_LEFT_DOWN, &ClickablePanel::OnMouseLeftDown, this);
    }
}

void ClickablePanel::OnMouseEnter(wxMouseEvent& event)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
    RSetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
    Refresh();
}

void ClickablePanel::OnMouseLeave(wxMouseEvent &event)
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
    RSetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    Refresh();
}

void ClickablePanel::OnMouseLeftDown(wxMouseEvent &event)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Refresh();
}

void ClickablePanel::OnMouseLeftUp(wxMouseEvent &event)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
    RSetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT));
    Refresh();

    wxCommandEvent outEvt(BTN_PANEL_LEFT_CLICK, GetId());
    outEvt.SetEventObject(this);
    ProcessWindowEvent(outEvt);
}



BtnPanel::BtnPanel(wxWindow *parent, wxWindowID winID, wxString label):
    ClickablePanel(parent, winID)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

    wxBoxSizer* rootSizer = new wxBoxSizer( wxVERTICAL );

    rootSizer->Add( 0, 8, 1, wxEXPAND, 0 );

    m_label = new wxStaticText(this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    rootSizer->Add(m_label, 0, wxALL | wxEXPAND, 5 );

    rootSizer->Add( 0, 8, 1, wxEXPAND, 0 );

    SetSizer( rootSizer );

    Layout();

    RBindBaseEvents();
}

BtnPanel::~BtnPanel()
{

}

void BtnPanel::SetLabel(wxString &label)
{
    m_label->SetLabel(label);
}

