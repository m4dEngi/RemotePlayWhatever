#ifndef FRIENDSLISTFRAME_H
#define FRIENDSLISTFRAME_H
#include <map>
#include <wx/wx.h>
#include "FriendPanel.h"
#include "RemotePlayInviteHandler.h"

enum ERPWEvtID
{
    rpwID_INVITE_GUEST,
    rpwID_INVITE_FRIEND,
};

class FriendsListFrame : public wxFrame
{
public:
    FriendsListFrame(RemotePlayInviteHandler* handler);

private:
    void AddFreiendPanel(CSteamID friendID);
    void RemoveFriendPanel(CSteamID friendID);

    bool ShowNonSteamWarning();
    bool CanSendInvite();

    RemotePlayInviteHandler* m_pRemoteInvite;
    wxScrolledWindow* m_friendsListContainer;
    wxBoxSizer* m_friendsListSizer;

    std::map<uint64, FriendPanel*> m_friendPanels;

    void OnFriendPanelClick(wxCommandEvent& event);
    void OnGuestPanelClick(wxCommandEvent& event);

    STEAM_CALLBACK(FriendsListFrame, OnRemotePlayInviteResult, RemotePlayInviteResult_t, m_remoteInviteResultCb);
    STEAM_CALLBACK(FriendsListFrame, OnPersonaStateChange, PersonaStateChange_t, m_personaStateCb);
};

#endif // FRIENDSLISTFRAME_H
