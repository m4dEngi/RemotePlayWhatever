#include <wx/wx.h>
#include <wx/clipbrd.h>
#include <cstring>
#include "FriendsListFrame.h"
#include "FriendPanel.h"


FriendsListFrame::FriendsListFrame(RemotePlayInviteHandler* handler):
    wxFrame(NULL, wxID_ANY, _("Remote Play Whatever"), wxPoint(50, 50), wxSize(275, 650)),
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
            wxString::Format("Could not create remote play session! (Result:%d)", inviteResultCb->m_eResult),
            "Remote Play Whatever",
            wxOK | wxICON_ERROR
        );
    }
    else
    {
        if(inviteResultCb->m_player.m_playerID.IsValid())
        {
            wxMessageBox(
                "Invite sent",
                "Remote Play Whatever",
                wxOK | wxICON_INFORMATION
            );
        }
        else
        {
            if(wxTheClipboard->Open())
            {
               wxTheClipboard->SetData(new wxTextDataObject(inviteResultCb->m_szConnectURL));
               wxMessageBox(
                   "Guest invite link created and copied to clipboard",
                   "Remote Play Whatever",
                   wxOK | wxICON_INFORMATION
               );
               wxTheClipboard->Flush();
               wxTheClipboard->Close();
            }
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

