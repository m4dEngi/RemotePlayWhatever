#include "RemoteApp.h"
#include "ClientContext.h"
#include "ClientTasks.h"

bool RemoteApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    if (!GClientContext()->Init())
    {
        wxMessageBox
        (
            "Could not initialize steam client library!",
            "Warning",
            wxOK | wxICON_ERROR
        );

        wxTheApp->Exit();
    }

    m_callbackRunner.Start(200);

    m_taskBarIcon = new RemoteAppTaskBarIcon();
    m_taskBarIcon->SetIcon(wxICON(sample), wxT("Steam Remote Play Whatever"));

    return true;
}

int RemoteApp::OnExit()
{
    if (m_callbackRunner.IsRunning())
    {
        m_callbackRunner.Stop();
    }
    return 0;
}

// system tray icon/menu


void RemoteAppTaskBarIcon::OnMenuExit(wxCommandEvent&)
{
    wxTheApp->Exit();
}

void RemoteAppTaskBarIcon::OnMenuSteamFriend(wxCommandEvent& evt)
{
    if (GClientContext()->ClientUser()->BIsAnyGameRunning())
    {
        GClientTaskRunner()->QueueTask(new RemotePlayInviteTask(CSteamID(m_dynamicFriends[evt.GetId()])));
    }
}

wxMenu* RemoteAppTaskBarIcon::CreatePopupMenu()
{
    m_dynamicFriends.clear();

    wxMenu* menu = new wxMenu;
    wxMenu* submenuSteam = new wxMenu;
    for (int i = 0; i < GClientContext()->ClientFriends()->GetFriendCount(k_EFriendFlagImmediate); ++i)
    {
        CSteamID idFriend = GClientContext()->ClientFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
        if (GClientContext()->ClientFriends()->GetFriendPersonaState(idFriend) != k_EPersonaStateOffline)
        {
            m_dynamicFriends[i] = idFriend.ConvertToUint64();
            wxMenuItem* friendItem = submenuSteam->Append(i, wxString(
                GClientContext()->ClientFriends()->GetFriendPersonaName(idFriend),
                wxConvUTF8
            ));

            Bind(wxEVT_MENU, &RemoteAppTaskBarIcon::OnMenuSteamFriend, this, wxID_ANY, wxID_ANY);
        }
    }

    menu->Append(wxID_ANY, wxT("Friends"), submenuSteam);
    menu->AppendSeparator();
    menu->Append(TRAY_EXIT, wxT("E&xit"));

    Bind(wxEVT_MENU, &RemoteAppTaskBarIcon::OnMenuExit, this, TRAY_EXIT);

    return menu;
}

// steam callback handler

void RemoteAppCallbackRunner::Notify()
{
    GClientContext()->RunCallbacks();
    GClientTaskRunner()->RunTasks();
}
