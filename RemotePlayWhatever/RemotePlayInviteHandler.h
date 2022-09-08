#ifndef REMOTEPLAYINVITEHANDLER_H
#define REMOTEPLAYINVITEHANDLER_H

#include <Steamworks.h>

class RemotePlayInviteHandler
{
public:
    RemotePlayInviteHandler();
    virtual ~RemotePlayInviteHandler() {}

    void SendInvite(CSteamID invitee);
    void CancelInvite(CSteamID invitee, uint64 guestID);

    void SetNonSteamAppID(AppId_t appID);
    void SetGuestID(uint64 guestID);

private:
    bool m_enabledDesktopStreaming;
    uint64 m_remoteGuestID;
    AppId_t m_nonsteamAppID;

    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayStop, RemoteClientStopStreamSession_t, m_remoteStopCb);
    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayInviteResult, RemotePlayInviteResult_t, m_remoteInviteResultCb);
};

#endif // REMOTEPLAYINVITEHANDLER_H
