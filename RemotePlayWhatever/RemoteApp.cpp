#include "RemoteApp.h"
#include "SteamStuff.h"

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
    m_taskBarIcon->SetIcon(wxICON(ID_APPICON), wxT("Steam Remote Play Whatever"));
    m_taskBarIcon->ShowBalloon(wxT("Steam Remote Play Whatever"), wxT("Started"));

    return true;
}

int RemoteApp::OnExit()
{
    if (m_callbackRunner.IsRunning())
    {
        m_callbackRunner.Stop();
    }

    m_taskBarIcon->RemoveIcon();

    return 0;
}

// system tray icon/menu

void RemoteAppTaskBarIcon::OnMenuExit(wxCommandEvent&)
{
    wxTheApp->Exit();
}

void RemoteAppTaskBarIcon::OnMenuSteamFriend(wxCommandEvent& evt)
{
    m_remoteInvite.SendInvite(CSteamID(m_dynamicFriends[evt.GetId()]));
}

wxMenu* RemoteAppTaskBarIcon::BuildFriendsMenu()
{
    m_dynamicFriends.clear();

    // save friends index and name in multimap to have them sorted in context menu later
    auto comparator = [](const wxString& lhs, const wxString& rhs) {
        return lhs.Lower() < rhs.Lower();
    };
    std::multimap<wxString, int, decltype(comparator)> friendsItems(comparator);

    wxMenu* submenuSteam = new wxMenu();
    for (int i = 0; i < GClientContext()->SteamFriends()->GetFriendCount(k_EFriendFlagImmediate); ++i)
    {
        CSteamID idFriend = GClientContext()->SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
        if (GClientContext()->SteamFriends()->GetFriendPersonaState(idFriend) != k_EPersonaStateOffline)
        {
            friendsItems.insert(std::pair<wxString, int>(
                wxString(GClientContext()->SteamFriends()->GetFriendPersonaName(idFriend), wxConvUTF8),
                i
                ));

            m_dynamicFriends[i] = idFriend.ConvertToUint64();
        }
    }

    for (auto it = friendsItems.cbegin(); it != friendsItems.cend(); ++it)
    {
        submenuSteam->Append(it->second, it->first);
        Bind(wxEVT_MENU, &RemoteAppTaskBarIcon::OnMenuSteamFriend, this);
    }

    return submenuSteam;
}

wxMenu* RemoteAppTaskBarIcon::CreatePopupMenu()
{
    wxMenu* menu = new wxMenu();
    if (GClientContext()->SteamUser()->BLoggedOn() && GetRunningGameID().IsValid())
    {
        menu->Append(wxID_ANY, wxT("Send remote play invite to..."), BuildFriendsMenu());
    }
    else
    {
        menu->Append(wxID_ANY, "Not in game")->Enable(false);
    }

    menu->AppendSeparator();
    menu->Append(TRAY_EXIT, wxT("E&xit"));

    Bind(wxEVT_MENU, &RemoteAppTaskBarIcon::OnMenuExit, this, TRAY_EXIT);

    return menu;
}

// steam callback handler

void RemoteAppCallbackRunner::Notify()
{
    GClientContext()->RunCallbacks();
}
