#ifndef FRIENDSLISTFRAME_H
#define FRIENDSLISTFRAME_H

#include <wx/wx.h>
#include "RemotePlayInviteHandler.h"

class FriendsListFrame : public wxFrame
{
public:
    FriendsListFrame(RemotePlayInviteHandler* handler);

private:
    void OnFPClick(wxCommandEvent& event);
    RemotePlayInviteHandler* m_pRemoteInvite;

    STEAM_CALLBACK(FriendsListFrame, OnRemotePlayInviteResult, RemotePlayInviteResult_t, m_remoteInviteResultCb);
};

#endif // FRIENDSLISTFRAME_H
