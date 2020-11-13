#ifndef REMOTEINVITEHANDLER_H
#define REMOTEINVITEHANDLER_H

#include <Steamworks.h>

class RemotePlayInviteHandler
{
public:
    RemotePlayInviteHandler();
    virtual ~RemotePlayInviteHandler() {}

    void SendInvite(CSteamID invitee);

private:
    bool m_enabledDesktopStreaming;

    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayStart, RemoteClientStartStreamSession_t, m_remoteStartCb);
    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayStop, RemoteClientStopStreamSession_t , m_remoteStopCb);
};


#endif // REMOTEINVITEHANDLER_H
