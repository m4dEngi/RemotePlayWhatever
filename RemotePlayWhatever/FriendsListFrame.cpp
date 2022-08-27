#include <wx/wx.h>
#include <wx/statline.h>
#include <wx/clipbrd.h>
#include <cstring>
#include "FriendsListFrame.h"
#include "FriendPanel.h"
#include "wxSteamStuff.h"


FriendsListFrame::FriendsListFrame(RemotePlayInviteHandler* handler):
    wxFrame(NULL, wxID_ANY, _("Remote Play Whatever"), wxPoint(50, 50), wxSize(275, 650)),
    m_remoteInviteResultCb(this, &FriendsListFrame::OnRemotePlayInviteResult)
{
    SetSizeHints( wxDefaultSize, wxDefaultSize );

    SetMinSize(wxSize(275, 300));
    SetMaxSize(wxSize(350, -1));

    m_pRemoteInvite = handler;

    wxBoxSizer* frameSizer;
    frameSizer = new wxBoxSizer( wxVERTICAL );

    wxScrolledWindow* friendsListContainer = new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
    friendsListContainer->SetScrollRate( 5, 5 );
    wxBoxSizer* friendsListSizer;
    friendsListSizer = new wxBoxSizer( wxVERTICAL );

    FriendPanel* fp = new FriendPanel(friendsListContainer, "Create guest invite link");
    friendsListSizer->Add(fp, 0, wxALL | wxEXPAND, 0);
    wxStaticLine* fpanelSeparator = new wxStaticLine(friendsListContainer, wxID_ANY, wxDefaultPosition, wxSize(-1,1));
    fpanelSeparator->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_SCROLLBAR));
    friendsListSizer->Add(fpanelSeparator, 0, wxEXPAND | wxALL, 0 );

    wxStaticText* flLabel = new wxStaticText(friendsListContainer, wxID_ANY, "or invite a friend", wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_HORIZONTAL);
    friendsListSizer->Add(flLabel, 0, wxALL | wxEXPAND, 10);

    int cFriends = GClientContext()->SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
    for(int i = 0; i < cFriends; ++i)
    {
        CSteamID friendSteamID = GClientContext()->SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
        EPersonaState personaState = GClientContext()->SteamFriends()->GetFriendPersonaState(friendSteamID);
        if(personaState != k_EPersonaStateOffline && personaState != k_EPersonaStateInvisible)
        {
            FriendPanel* fp = new FriendPanel(friendsListContainer, friendSteamID);
            friendsListSizer->Add(fp, 0, wxALL | wxEXPAND, 0);
            wxStaticLine* fpanelSeparator = new wxStaticLine(friendsListContainer, wxID_ANY, wxDefaultPosition, wxSize(-1,1));
            fpanelSeparator->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_SCROLLBAR));
            friendsListSizer->Add(fpanelSeparator, 0, wxALL | wxEXPAND, 0);
        }
    }

    friendsListContainer->SetSizer( friendsListSizer );
    friendsListContainer->Layout();
    friendsListSizer->Fit( friendsListContainer );
    frameSizer->Add( friendsListContainer, 1, wxEXPAND | wxALL, 0 );

    SetSizer( frameSizer );
    Layout();

    Bind(FP_LEFT_CLICK, &FriendsListFrame::OnFPClick, this);
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

void FriendsListFrame::OnFPClick(wxCommandEvent &event)
{
    FriendPanel* sender = (FriendPanel*)event.GetEventObject();
    CSteamID invitee = ((wxSteamID*)event.GetClientData())->GetSteamID();
    wxMessageDialog* dlg;
    if(invitee.IsValid())
    {
        dlg = new wxMessageDialog(this, "Send remote play invite to " + sender->GetPersonaName()  + "?", "Confirm Remote Play Invite", wxOK | wxCANCEL);
    }
    else
    {
        dlg = new wxMessageDialog(this, "Create guest invite link?", "Confirm Remote Play Invite", wxOK | wxCANCEL);
    }
    if(dlg->ShowModal() == wxID_OK)
    {
        m_pRemoteInvite->SendInvite(invitee);
    }
    delete dlg;
}

