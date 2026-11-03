#include <wx/config.h>
#include <wx/menu.h>
#include "TrayIcon.h"
#include "FriendsListFrame.h"
#include "ControlFrame.h"
#include "RemotePlayInviteHandler.h"
#include "UiHelpers.h"

enum
{
    idTrayOpen = wxID_HIGHEST + 500,
    idTrayControls,
    idTrayCopyLink,
    idTrayEndSession,
    idTraySounds,
    idTrayQuit,
};

RpwTrayIcon::RpwTrayIcon(FriendsListFrame* main, ControlFrame* controls, RemotePlayInviteHandler* handler) :
    m_main(main),
    m_controls(controls),
    m_handler(handler),
    m_icon(wxBitmapBundle::FromIconBundle(AppIcons()))
{
    SetIcon(m_icon, "Remote Play Whatever");
    Bind(wxEVT_TASKBAR_LEFT_UP, &RpwTrayIcon::OnLeftClick, this);
    Bind(wxEVT_MENU, &RpwTrayIcon::OnMenu, this);
}

void RpwTrayIcon::SetStatus(const wxString& status)
{
    // Windows cuts tray tooltips off at 127 characters.
    wxString tip = "Remote Play Whatever\n" + status;
    if (tip.length() > 120)
    {
        tip = tip.Left(117) + "...";
    }
    SetIcon(m_icon, tip);
}

void RpwTrayIcon::ShowHiddenHintOnce()
{
#if defined(__WXMSW__) && wxUSE_TASKBARICON_BALLOONS
    wxConfigBase* config = wxConfigBase::Get();
    if (!config || config->ReadBool("/Options/TrayHintShown", false))
    {
        return;
    }
    config->Write("/Options/TrayHintShown", true);
    config->Flush();
    ShowBalloon("Remote Play Whatever is still running",
                "It's by the clock (under ^ if you don't see it). Click it, or open the app again, to bring the window back.",
                0, wxICON_INFORMATION);
#endif
}

wxMenu* RpwTrayIcon::CreatePopupMenu()
{
    wxMenu* menu = new wxMenu;
    menu->Append(idTrayOpen, "Open Remote Play Whatever");
    menu->Append(idTrayControls, "Show guest controls");
    menu->AppendSeparator();
    menu->Append(idTrayCopyLink, "Copy guest link")->Enable(!m_handler->GuestLink().empty());
    menu->Append(idTrayEndSession, "End session")->Enable(m_handler->IsSessionActive());
    menu->AppendSeparator();
    menu->AppendCheckItem(idTraySounds, "Sounds when guests join or leave")->Check(SoundsEnabled());
    menu->AppendSeparator();
    menu->Append(idTrayQuit, "Quit");
    return menu;
}

void RpwTrayIcon::OnLeftClick(wxTaskBarIconEvent&)
{
    m_main->ShowFromTray();
}

void RpwTrayIcon::OnMenu(wxCommandEvent& event)
{
    switch (event.GetId())
    {
    case idTrayOpen:
        m_main->ShowFromTray();
        break;
    case idTrayControls:
        if (m_controls)
        {
            m_controls->Show();
            m_controls->Raise();
        }
        break;
    case idTrayCopyLink:
        m_main->CopyGuestLink();
        break;
    case idTrayEndSession:
        m_main->ConfirmEndSession();
        break;
    case idTraySounds:
        SetSoundsEnabled(!SoundsEnabled());
        break;
    case idTrayQuit:
        m_main->Close(true);
        break;
    default:
        event.Skip();
        break;
    }
}
