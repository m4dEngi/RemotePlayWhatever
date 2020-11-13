#include "RemoteInviteHandler.h"
#include "ClientContext.h"

RemotePlayInviteHandler::RemotePlayInviteHandler() :
    m_remoteStartCb(this, &RemotePlayInviteHandler::OnRemotePlayStart),
    m_remoteStopCb(this, &RemotePlayInviteHandler::OnRemotePlayStop),
    m_enabledDesktopStreaming(false)
{
}

void RemotePlayInviteHandler::SendInvite(CSteamID invitee)
{
    CGameID gameID = GClientContext()->ClientUser()->GetRunningGameID(0);
    if(gameID.IsSteamApp())
    {
        GClientContext()->RemoteClientManager()->CreateRemotePlayInviteAndSession(invitee, gameID.AppID());
    }
    else
    {
        GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(true);

        m_enabledDesktopStreaming = true;

        GClientContext()->RemoteClientManager()->CreateRemotePlayInviteAndSession(invitee, 480);
    }
}

void RemotePlayInviteHandler::OnRemotePlayStart(RemoteClientStartStreamSession_t* streamStartedCb)
{
    if (GClientContext()->ClientUser()->GetRunningGameID(0).AppID() == 0)
    {
        char buf[256] = { '\0' };
        std::string authKey;
        char hexBuf[3] = { '\0' };
        for (int i = 0; i < streamStartedCb->m_cubAuthToken; ++i)
        {
            sprintf(hexBuf, "%02x", streamStartedCb->m_rgubAuthToken[i]);
            authKey += hexBuf;
        }

        sprintf(buf,
            "Please open this link in your default browser to join: steam://remoteplay/connect/%llu?appid=%d&auth=%s&transport=k_EStreamTransportSDR&relay=%s&restricted_countries=CN,XC",
            GClientContext()->ClientUser()->GetSteamID().ConvertToUint64(),
            480,
            authKey.c_str(),
            streamStartedCb->m_szRelayServer
        );

        GClientContext()->ClientFriends()->SendMsgToFriend(streamStartedCb->m_player, k_EChatEntryTypeChatMsg, buf);
    }
}

void RemotePlayInviteHandler::OnRemotePlayStop(RemoteClientStopStreamSession_t* streamStopCb)
{
    if (!GClientContext()->RemoteClientManager()->BIsStreamingSessionActive() &&
        m_enabledDesktopStreaming
    )
    {
        GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(false);
    }
}
