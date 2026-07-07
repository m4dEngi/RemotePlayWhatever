#include "ControlFrame.h"
#include "wxSteamStuff.h"

static const wxColour kGreen(46, 160, 67);
static const wxColour kRed(180, 60, 60);

ControlFrame::ControlFrame(RemotePlayInviteHandler* handler) :
    wxFrame(NULL, wxID_ANY, "Remote Play Control", wxDefaultPosition, wxSize(420, 460)),
    m_handler(handler),
    m_timer(this),
    m_lastRevision((uint32)-1)
{
    m_container = new wxScrolledWindow(this, wxID_ANY);
    m_container->SetScrollRate(0, 10);
    m_sizer = new wxBoxSizer(wxVERTICAL);
    m_container->SetSizer(m_sizer);

    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(m_container, 1, wxEXPAND);
    SetSizer(frameSizer);

    Bind(wxEVT_TIMER, &ControlFrame::OnPoll, this);
    m_timer.Start(500);

    Rebuild();
}

void ControlFrame::OnPoll(wxTimerEvent&)
{
    if (m_handler->RosterRevision() != m_lastRevision)
    {
        Rebuild();
    }

    // Pop up automatically the moment a guest connects.
    if (!m_handler->Guests().empty() && !IsShown())
    {
        Show();
        Raise();
    }
}

// A colored toggle: green = the guest has this peripheral, red = revoked.
// Clicking flips just this one peripheral for this one guest.
void ControlFrame::AddToggle(wxWindow* parent, wxSizer* sizer, const wxString& label,
                             bool on, uint64 steamID, int which)
{
    wxButton* btn = new wxButton(parent, wxID_ANY, label, wxDefaultPosition, wxSize(84, 30));
    btn->SetBackgroundColour(on ? kGreen : kRed);
    btn->SetForegroundColour(*wxWHITE);
    btn->SetToolTip(on ? label + ": granted (click to take)" : label + ": paused (click to give)");
    sizer->Add(btn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 3);

    btn->Bind(wxEVT_BUTTON, [this, steamID, which](wxCommandEvent&)
    {
        const std::vector<ConnectedGuest>& guests = m_handler->Guests();
        for (size_t i = 0; i < guests.size(); ++i)
        {
            if (guests[i].steamID == steamID)
            {
                bool k = guests[i].wantKbd;
                bool m = guests[i].wantMouse;
                bool c = guests[i].wantCtrl;
                if (which == 0)      k = !k;
                else if (which == 1) m = !m;
                else                 c = !c;
                m_handler->SetGuestInput(steamID, k, m, c);
                break;
            }
        }
        Rebuild();   // refresh button colors
    });
}

void ControlFrame::Rebuild()
{
    m_lastRevision = m_handler->RosterRevision();

    Freeze();
    m_sizer->Clear(true);   // destroy existing row widgets

    const std::vector<ConnectedGuest>& guests = m_handler->Guests();

    if (guests.empty())
    {
        m_sizer->Add(new wxStaticText(m_container, wxID_ANY,
            "Waiting for guests to join...\n\nInvite a friend from the main window;\n"
            "they'll appear here when they connect."),
            0, wxALL, 14);
    }
    else
    {
        for (size_t i = 0; i < guests.size(); ++i)
        {
            const ConnectedGuest& g = guests[i];
            CSteamID sid = g.player.m_playerID;

            wxPanel* row = new wxPanel(m_container, wxID_ANY);
            row->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            wxBoxSizer* rs = new wxBoxSizer(wxHORIZONTAL);

            int av = GClientContext()->SteamFriends()->GetSmallFriendAvatar(sid);
            wxStaticBitmap* img = new wxStaticBitmap(row, wxID_ANY,
                av ? SteamImageToBitmap(av) : wxNullBitmap, wxDefaultPosition, wxSize(32, 32));
            rs->Add(img, 0, wxALL | wxALIGN_CENTER_VERTICAL, 6);

            wxString name = wxString(GClientContext()->SteamFriends()->GetFriendPersonaName(sid), wxConvUTF8);
            if (name.IsEmpty())
            {
                name = wxString::Format("Guest %llu", (unsigned long long)g.steamID);
            }
            wxStaticText* nm = new wxStaticText(row, wxID_ANY, name);
            rs->Add(nm, 1, wxALL | wxALIGN_CENTER_VERTICAL, 6);

            AddToggle(row, rs, "Keyboard",   g.wantKbd,   g.steamID, 0);
            AddToggle(row, rs, "Mouse",      g.wantMouse, g.steamID, 1);
            AddToggle(row, rs, "Controller", g.wantCtrl,  g.steamID, 2);

            row->SetSizer(rs);
            m_sizer->Add(row, 0, wxEXPAND | wxALL, 2);
        }
    }

    m_container->Layout();
    m_container->FitInside();
    Thaw();
}
