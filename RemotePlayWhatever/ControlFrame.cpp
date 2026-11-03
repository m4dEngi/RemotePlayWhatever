#include "ControlFrame.h"
#include "wxSteamStuff.h"
#include "UiHelpers.h"
#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

static const wxColour kGreen(46, 160, 67);
static const wxColour kRed(180, 60, 60);

// Key used by the "Everyone" row's toggles; never a real guest key.
static const uint64 kEveryone = 0;

enum
{
    kHotKeyKeyboard = 0xB001,
    kHotKeyMouse,
    kHotKeyController,
    kHotKeyTakeControl,
    kHotKeyGiveBack,
};

ControlFrame::ControlFrame(RemotePlayInviteHandler* handler) :
    wxFrame(NULL, wxID_ANY, "Remote Play Control", wxDefaultPosition, wxSize(520, 460)),
    m_handler(handler),
    m_hotkeyHint(nullptr),
    m_timer(this),
    m_lastRevision((uint32)-1),
    m_lastGuestCount(0),
    m_waitingForNames(false)
{
    SetIcons(AppIcons());

    m_container = new wxScrolledWindow(this, wxID_ANY);
    m_container->SetScrollRate(0, 10);
    m_sizer = new wxBoxSizer(wxVERTICAL);
    m_container->SetSizer(m_sizer);

    wxBoxSizer* frameSizer = new wxBoxSizer(wxVERTICAL);
    frameSizer->Add(m_container, 1, wxEXPAND);
    m_hotkeyHint = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_hotkeyHint->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
    frameSizer->Add(m_hotkeyHint, 0, wxALL | wxEXPAND, 8);

    wxButton* endButton = new wxButton(this, wxID_ANY, "End session");
    endButton->SetToolTip("Disconnect everyone, cancel unused invites and guest links, and stop sharing your screen.");
    endButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent&)
    {
        int answer = wxMessageBox(
            "Disconnect everyone and stop sharing?\n\nUnused invites and guest links will stop working too.",
            "Remote Play Control", wxYES_NO | wxICON_QUESTION, this);
        if (answer == wxYES)
        {
            m_handler->EndSession();
            Rebuild();
        }
    });
    frameSizer->Add(endButton, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxALIGN_RIGHT, 8);
    SetSizer(frameSizer);

    RegisterHotKeys();

    Bind(wxEVT_TIMER, &ControlFrame::OnPoll, this);
    Bind(wxEVT_CLOSE_WINDOW, &ControlFrame::OnClose, this);
    m_timer.Start(500);

    Rebuild();
}

ControlFrame::~ControlFrame()
{
#if wxUSE_HOTKEY
    for (size_t i = 0; i < m_hotkeys.size(); ++i)
    {
        UnregisterHotKey(m_hotkeys[i]);
    }
#endif
}

void ControlFrame::RegisterHotKeys()
{
    wxString hint;
#if wxUSE_HOTKEY
    struct HotKey { int id; int key; const char* what; };
    static const HotKey keys[] =
    {
        { kHotKeyKeyboard,    'K', "K keyboard on/off" },
        { kHotKeyMouse,       'M', "M mouse on/off" },
        { kHotKeyController,  'C', "C controller on/off" },
        { kHotKeyTakeControl, 'D', "D take all control" },
        { kHotKeyGiveBack,    'G', "G give it back" },
    };

    // A key combo another app already owns just isn't offered.
    for (const HotKey& k : keys)
    {
        if (RegisterHotKey(k.id, wxMOD_CONTROL | wxMOD_ALT, k.key))
        {
            m_hotkeys.push_back(k.id);
            hint += hint.empty() ? "" : ", ";
            hint += k.what;
        }
    }
    Bind(wxEVT_HOTKEY, &ControlFrame::OnHotKey, this);

    if (!hint.empty())
    {
        hint = "Shortcuts for everyone that work while you're in the game (Ctrl+Alt + key): " + hint + ".";
    }
#endif
    m_hotkeyHint->SetLabel(hint);
    m_hotkeyHint->Wrap(FromDIP(490));
    m_hotkeyHint->Show(!hint.empty());
}

void ControlFrame::OnHotKey(wxKeyEvent& evt)
{
    if (m_handler->Guests().empty())
    {
        return;
    }

    bool gave = true;
    switch (evt.GetId())
    {
    case kHotKeyKeyboard:    gave = !m_handler->AllGuestsHave(0); m_handler->SetAllGuestsInput(0, gave, false); break;
    case kHotKeyMouse:       gave = !m_handler->AllGuestsHave(1); m_handler->SetAllGuestsInput(1, gave, false); break;
    case kHotKeyController:  gave = !m_handler->AllGuestsHave(2); m_handler->SetAllGuestsInput(2, gave, false); break;
    case kHotKeyTakeControl: gave = false; m_handler->TakeAllControl(); break;
    case kHotKeyGiveBack:    gave = true;  m_handler->GiveBackControl(); break;
    default:                 return;
    }

#ifdef __WXMSW__
    // The window is usually hidden behind the game, so confirm with a sound.
    ::MessageBeep(gave ? MB_OK : MB_ICONEXCLAMATION);
#endif
    Rebuild();
}

void ControlFrame::OnClose(wxCloseEvent& evt)
{
    // Closing only hides the window; it comes back when the next guest joins.
    // (Quitting the app destroys it directly from the main window.)
    if (evt.CanVeto())
    {
        evt.Veto();
        Hide();
        return;
    }
    evt.Skip();
}

void ControlFrame::OnPoll(wxTimerEvent&)
{
    bool rebuild = m_handler->RosterRevision() != m_lastRevision;

    // Names of guests who joined through a link arrive from Steam a moment
    // after they connect; refresh once they're known.
    if (!rebuild && m_waitingForNames)
    {
        const std::vector<ConnectedGuest>& guests = m_handler->Guests();
        for (size_t i = 0; i < guests.size() && !rebuild; ++i)
        {
            bool placeholder = false;
            GuestName(guests[i], placeholder);
            rebuild = guests[i].steamID && !placeholder;
        }
    }

    if (rebuild)
    {
        Rebuild();
    }

    // Pop up when a new guest joins, without taking focus away from the game
    // (which can minimize a fullscreen game). Flash the taskbar button instead.
    size_t count = m_handler->Guests().size();
    if (count > m_lastGuestCount && !IsShown())
    {
        ShowWithoutActivating();
        RequestUserAttention(wxUSER_ATTENTION_INFO);
    }
    m_lastGuestCount = count;
}

// A colored toggle: green = the guest has this peripheral, red = revoked.
// Clicking flips just this one peripheral for this one guest, or for everyone
// on the "Everyone" row.
void ControlFrame::AddToggle(wxWindow* parent, wxSizer* sizer, const wxString& label,
                             bool on, uint64 guestKey, int which)
{
    wxButton* btn = new wxButton(parent, wxID_ANY, label, wxDefaultPosition, wxSize(84, 30));
    btn->SetBackgroundColour(on ? kGreen : kRed);
    btn->SetForegroundColour(*wxWHITE);
    btn->SetToolTip(on ? label + ": granted (click to take)" : label + ": paused (click to give)");
    sizer->Add(btn, 0, wxALL | wxALIGN_CENTER_VERTICAL, 3);

    btn->Bind(wxEVT_BUTTON, [this, guestKey, which](wxCommandEvent&)
    {
        if (guestKey == kEveryone)
        {
            m_handler->SetAllGuestsInput(which, !m_handler->AllGuestsHave(which), true);
        }
        else
        {
            const std::vector<ConnectedGuest>& guests = m_handler->Guests();
            for (size_t i = 0; i < guests.size(); ++i)
            {
                if (guests[i].key == guestKey)
                {
                    bool k = guests[i].wantKbd;
                    bool m = guests[i].wantMouse;
                    bool c = guests[i].wantCtrl;
                    if (which == 0)      k = !k;
                    else if (which == 1) m = !m;
                    else                 c = !c;
                    m_handler->SetGuestInput(guestKey, k, m, c);
                    break;
                }
            }
        }
        // Rebuilding destroys this button, so wait until its click is handled.
        CallAfter([this]() { Rebuild(); });
    });
}

wxString ControlFrame::GuestName(const ConnectedGuest& g, bool& placeholder)
{
    placeholder = false;
    if (!g.steamID)
    {
        return wxString::Format("Steam Link guest %u (no Steam account)", g.player.m_unGuestID);
    }

    const char* persona = GClientContext()->SteamFriends()->GetFriendPersonaName(g.player.m_playerID);
    wxString name = wxString::FromUTF8(persona ? persona : "");
    if (name.IsEmpty() || name == "[unknown]")
    {
        placeholder = true;
        return "Guest (joined with a link)";
    }
    return name;
}

void ControlFrame::Rebuild()
{
    m_lastRevision = m_handler->RosterRevision();
    m_waitingForNames = false;

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
        // With several guests, an "Everyone" row flips a peripheral for all of them.
        if (guests.size() >= 2)
        {
            wxPanel* row = new wxPanel(m_container, wxID_ANY);
            row->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            wxBoxSizer* rs = new wxBoxSizer(wxHORIZONTAL);
            rs->AddSpacer(44);
            wxStaticText* nm = new wxStaticText(row, wxID_ANY, "Everyone");
            nm->SetFont(nm->GetFont().Bold());
            rs->Add(nm, 1, wxALL | wxALIGN_CENTER_VERTICAL, 6);
            AddToggle(row, rs, "Keyboard",   m_handler->AllGuestsHave(0), kEveryone, 0);
            AddToggle(row, rs, "Mouse",      m_handler->AllGuestsHave(1), kEveryone, 1);
            AddToggle(row, rs, "Controller", m_handler->AllGuestsHave(2), kEveryone, 2);
            rs->AddSpacer(76);  // lines the toggles up with the guest rows' Remove column
            row->SetSizer(rs);
            m_sizer->Add(row, 0, wxEXPAND | wxALL, 2);
        }

        for (size_t i = 0; i < guests.size(); ++i)
        {
            const ConnectedGuest& g = guests[i];

            wxPanel* row = new wxPanel(m_container, wxID_ANY);
            row->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));
            wxBoxSizer* rs = new wxBoxSizer(wxHORIZONTAL);

            int av = g.steamID ? GClientContext()->SteamFriends()->GetSmallFriendAvatar(g.player.m_playerID) : 0;
            wxStaticBitmap* img = new wxStaticBitmap(row, wxID_ANY,
                av ? SteamImageToBitmap(av) : wxNullBitmap, wxDefaultPosition, wxSize(32, 32));
            rs->Add(img, 0, wxALL | wxALIGN_CENTER_VERTICAL, 6);

            bool placeholder = false;
            wxString name = GuestName(g, placeholder);
            m_waitingForNames = m_waitingForNames || placeholder;
            wxStaticText* nm = new wxStaticText(row, wxID_ANY, name);
            rs->Add(nm, 1, wxALL | wxALIGN_CENTER_VERTICAL, 6);

            AddToggle(row, rs, "Keyboard",   g.wantKbd,   g.key, 0);
            AddToggle(row, rs, "Mouse",      g.wantMouse, g.key, 1);
            AddToggle(row, rs, "Controller", g.wantCtrl,  g.key, 2);

            wxButton* remove = new wxButton(row, wxID_ANY, "Remove", wxDefaultPosition, wxSize(70, 30));
            remove->SetToolTip("Disconnect " + name + " from the session");
            const uint64 key = g.key;
            remove->Bind(wxEVT_BUTTON, [this, key, name](wxCommandEvent&)
            {
                // Ask after the click is handled: removing rebuilds the window,
                // which destroys this button.
                CallAfter([this, key, name]()
                {
                    int answer = wxMessageBox("Remove " + name + " from the session?",
                                              "Remote Play Control", wxYES_NO | wxICON_QUESTION, this);
                    if (answer == wxYES)
                    {
                        m_handler->RemoveGuest(key);
                        Rebuild();
                    }
                });
            });
            rs->Add(remove, 0, wxALL | wxALIGN_CENTER_VERTICAL, 3);

            row->SetSizer(rs);
            m_sizer->Add(row, 0, wxEXPAND | wxALL, 2);
        }
    }

    m_container->Layout();
    m_container->FitInside();
    Thaw();
}
