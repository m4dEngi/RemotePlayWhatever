#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <cstring>
#include "FriendsListFrame.h"
#include "FriendPanel.h"


FriendsListFrame::FriendsListFrame(RemotePlayInviteHandler* handler):
    wxFrame(NULL, wxID_ANY, _("Remote Play Whatever"), wxPoint(50, 50), wxSize(275, 650)),
    m_tick(this),
    m_askingGroupFallback(false),
    m_remoteInviteResultCb(this, &FriendsListFrame::OnRemotePlayInviteResult),
    m_personaStateCb(this, &FriendsListFrame::OnPersonaStateChange)
{
    SetSizeHints( wxDefaultSize, wxDefaultSize );

    SetMinSize(wxSize(275, 300));
    SetMaxSize(wxSize(350, -1));

    m_pRemoteInvite = handler;

    wxBoxSizer* frameSizer = new wxBoxSizer( wxVERTICAL );

    m_friendsListContainer = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
    m_friendsListContainer->SetScrollRate( 5, 5 );
    m_friendsListSizer = new wxBoxSizer( wxVERTICAL );

    BtnPanel* guestBtn = new BtnPanel(m_friendsListContainer, rpwID_INVITE_GUEST, "Create guest invite link");
    m_friendsListSizer->Add(guestBtn, 0, wxALL | wxEXPAND, 0);

    wxStaticText* flLabel = new wxStaticText(m_friendsListContainer, wxID_ANY, "or invite a friend", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    m_friendsListSizer->Add(flLabel, 0, wxALL | wxEXPAND, 10);

    int cFriends = GClientContext()->SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
    for(int i = 0; i < cFriends; ++i)
    {
        CSteamID friendSteamID = GClientContext()->SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
        EPersonaState personaState = GClientContext()->SteamFriends()->GetFriendPersonaState(friendSteamID);
        if(personaState != k_EPersonaStateOffline && personaState != k_EPersonaStateInvisible)
        {
            AddFreiendPanel(friendSteamID);
        }
    }

    m_friendsListContainer->SetSizer( m_friendsListSizer );
    m_friendsListContainer->Layout();
    m_friendsListSizer->Fit( m_friendsListContainer );
    frameSizer->Add( m_friendsListContainer, 1, wxEXPAND | wxALL, 0 );

    SetSizer( frameSizer );
    Layout();

    Bind(BTN_PANEL_LEFT_CLICK, &FriendsListFrame::OnFriendPanelClick, this, rpwID_INVITE_FRIEND);
    Bind(BTN_PANEL_LEFT_CLICK, &FriendsListFrame::OnGuestPanelClick, this, rpwID_INVITE_GUEST);
    Bind(wxEVT_TIMER, &FriendsListFrame::OnTick, this);
    m_tick.Start(500);
}

void FriendsListFrame::OnTick(wxTimerEvent&)
{
    // Timers keep firing while a message box is open, so don't ask twice.
    if (m_pRemoteInvite->HasGroupFailure() && !m_askingGroupFallback)
    {
        m_askingGroupFallback = true;
        int answer = wxMessageBox(
            "Steam didn't set up a Remote Play Together group for this game, so your "
            "guest might only be able to watch, not play.\n\n"
            "Send the invite anyway?\n\n"
            "If they can't play, quit the game, start it again through Steam, and send a new invite.",
            "Remote Play Whatever",
            wxYES_NO | wxICON_WARNING,
            this
        );
        m_pRemoteInvite->ResolveGroupFailure(answer == wxYES);
        m_askingGroupFallback = false;
    }
}

static wxString DescribeLaunchResult(ERemoteClientLaunchResult result)
{
    switch (result)
    {
    case k_ERemoteClientLaunchResultTimeout:
    case k_ERemoteClientLaunchResultStreamTimeout:      return "Steam timed out while starting the session.";
    case k_ERemoteClientLaunchResultOtherGameRunning:   return "Another game is already being shared.";
    case k_ERemoteClientLaunchResultAccessDenied:       return "Steam refused the request.";
    case k_ERemoteClientLaunchResultNetworkError:       return "There was a network error.";
    case k_ERemoteClientLaunchResultScreenLocked:       return "This PC's screen is locked.";
    case k_ERemoteClientLaunchResultUnsupported:        return "Steam says this game can't be shared.";
    case k_ERemoteClientLaunchResultDisabledLocal:      return "Remote Play is turned off in this PC's Steam settings.";
    case k_ERemoteClientLaunchResultDisabledRemote:     return "Remote Play is turned off on your friend's side.";
    case k_ERemoteClientLaunchResultBroadcasting:       return "Steam can't share the game while you're broadcasting.";
    case k_ERemoteClientLaunchResultBusy:               return "Steam is busy with another stream.";
    case k_ERemoteClientLaunchResultDriversNotInstalled:return "Steam's streaming drivers aren't installed.";
    case k_ERemoteClientLaunchResultCanceled:           return "The invite was canceled.";
    default:                                            return "Steam couldn't start the session.";
    }
}

void FriendsListFrame::AddFreiendPanel(CSteamID friendID)
{
    FriendPanel* fp = new FriendPanel(m_friendsListContainer, rpwID_INVITE_FRIEND, friendID);
    m_friendsListSizer->Add(fp, 0, wxBOTTOM | wxEXPAND, 1);

    uint64 friendID64 = friendID.ConvertToUint64();
    auto it = m_friendPanels.find(friendID64);
    if(it != m_friendPanels.end())
    {
        m_friendsListContainer->RemoveChild((*it).second);
        (*it).second->Destroy();
    }
    m_friendPanels[friendID.ConvertToUint64()] = fp;
}

void FriendsListFrame::RemoveFriendPanel(CSteamID friendID)
{
    uint64 friendID64 = friendID.ConvertToUint64();
    auto it = m_friendPanels.find(friendID64);
    if(it != m_friendPanels.end())
    {
        m_friendsListContainer->RemoveChild((*it).second);
        (*it).second->Destroy();
        m_friendPanels.erase(it);
    }
}

void FriendsListFrame::OnPersonaStateChange(PersonaStateChange_t* personaStateCb)
{
    if(personaStateCb->m_ulSteamID == GClientContext()->SteamUser()->GetSteamID())
    {
        return;
    }

    if(!(GClientContext()->SteamFriends()->GetFriendRelationship(personaStateCb->m_ulSteamID) &
        k_EFriendRelationshipFriend))
    {
        return;
    }

    if(personaStateCb->m_nChangeFlags & k_EPersonaChangeGoneOffline)
    {
        RemoveFriendPanel(personaStateCb->m_ulSteamID);
        Layout();
    }
    else if(personaStateCb->m_nChangeFlags & k_EPersonaChangeComeOnline)
    {
        AddFreiendPanel(personaStateCb->m_ulSteamID);
        Layout();
    }
}

void FriendsListFrame::OnRemotePlayInviteResult(RemotePlayInviteResult_t* inviteResultCb)
{
    if (inviteResultCb->m_eResult != k_ERemoteClientLaunchResultOK)
    {
        wxMessageBox(
            "Couldn't start Remote Play. " + DescribeLaunchResult(inviteResultCb->m_eResult) +
            wxString::Format("\n\n(Steam result %d)", (int)inviteResultCb->m_eResult),
            "Remote Play Whatever",
            wxOK | wxICON_ERROR
        );
    }
    else
    {
        if(inviteResultCb->m_player.m_playerID.IsValid())
        {
            const char* persona = GClientContext()->SteamFriends()->GetFriendPersonaName(inviteResultCb->m_player.m_playerID);
            wxMessageBox(
                "Invite sent to " + wxString::FromUTF8(persona && *persona ? persona : "your friend") +
                ". They'll get the link in Steam chat.",
                "Remote Play Whatever",
                wxOK | wxICON_INFORMATION
            );
        }
        else
        {
            wxString link = wxString::FromUTF8(ClickableURL(inviteResultCb->m_szConnectURL).c_str());
            bool copied = false;
            // Close the clipboard before showing the message box; keeping it open
            // while the box is up blocks pasting the link anywhere else.
            if(wxTheClipboard->Open())
            {
                copied = wxTheClipboard->SetData(new wxTextDataObject(link));
                wxTheClipboard->Flush();
                wxTheClipboard->Close();
            }
            wxMessageBox(
                (copied ? "Guest invite link copied to the clipboard:\n\n" : "Guest invite link:\n\n") + link +
                "\n\nSend it to whoever you want to play with. Anyone with the link can join.",
                "Remote Play Whatever",
                wxOK | wxICON_INFORMATION
            );
        }
    }
}

bool FriendsListFrame::CanSendInvite()
{
    if (!GClientContext()->SteamUser()->BLoggedOn() || !GetRunningGameID().IsValid())
    {
        wxMessageBox(
            "Could not detect game running. Start a game first!",
            "No game runnunig!",
            wxOK | wxICON_INFORMATION
        );
        return false;
    }
    return true;
}

bool FriendsListFrame::ShowNonSteamWarning()
{
    AppId_t nonSteamAppID = m_pRemoteInvite->GetNonSteamAppID();
    CGameID gameRunning = GetRunningGameID();

    if (gameRunning.IsSteamApp() && gameRunning.AppID() != nonSteamAppID)
    {
        return true;
    }

    bool dlgResult = false;
    wxMessageDialog* dlg = new wxMessageDialog(this,
                                               "Starting a Remote Play Together session for a non-Steam game\n"
                                               "will expose your entire desktop to the remote player.\n"
                                               "The Steam client might need to be restarted to stop\n"
                                               "the desktop sharing session!\n\n"
                                               "Continue?",
                                               "Non-Steam Game confirmation",
                                               wxOK | wxCANCEL);
    if(dlg->ShowModal() == wxID_OK)
    {
        dlgResult = true;
    }
    delete dlg;
    return dlgResult;
}

void FriendsListFrame::OnFriendPanelClick(wxCommandEvent &event)
{
    if(!CanSendInvite())
    {
        return;
    }

    if(!ShowNonSteamWarning())
    {
        return;
    }

    FriendPanel* sender = (FriendPanel*)event.GetEventObject();
    CSteamID invitee = sender->GetSteamID();
    wxMessageDialog* dlg = new wxMessageDialog(this, "Send remote play invite to " + sender->GetDisplayPersonaName()  + "?", "Confirm Remote Play Invite", wxOK | wxCANCEL);
    if(dlg->ShowModal() == wxID_OK)
    {
        m_pRemoteInvite->SendInvite(invitee);
    }
    delete dlg;
}

void FriendsListFrame::OnGuestPanelClick(wxCommandEvent &event)
{
    if(!CanSendInvite())
    {
        return;
    }

    if(!ShowNonSteamWarning())
    {
        return;
    }

    wxMessageDialog* dlg = new wxMessageDialog(this, "Create guest invite link?", "Confirm Remote Play Invite", wxOK | wxCANCEL);
    if(dlg->ShowModal() == wxID_OK)
    {
        m_pRemoteInvite->SendInvite(CSteamID(0, k_EUniversePublic, k_EAccountTypeIndividual));
    }
    delete dlg;
}

