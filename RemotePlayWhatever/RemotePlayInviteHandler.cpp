#include "SteamStuff.h"
#include "RemotePlayInviteHandler.h"

RemotePlayInviteHandler::RemotePlayInviteHandler() :
    m_enabledDesktopStreaming(false),
    m_remoteGuestID(1),
    m_nonsteamAppID(0),
    m_groupID(1),
    m_remoteInviteResultCb(this, &RemotePlayInviteHandler::OnRemotePlayInviteResult),
    m_remoteStopCb(this, &RemotePlayInviteHandler::OnRemotePlayStop)
{
}

AppId_t RemotePlayInviteHandler::GetNonSteamAppID()
{
    return m_nonsteamAppID;
}

void RemotePlayInviteHandler::SendInvite(CSteamID invitee)
{
    CGameID gameID = GetRunningGameID();

    if (!gameID.IsValid())
    {
        return;
    }

    RemotePlayPlayer_t rppInvitee = { invitee, m_remoteGuestID, m_groupID, 0, 0, 0 };
    ++m_remoteGuestID;

    if (gameID.IsSteamApp() && gameID.AppID() != m_nonsteamAppID)
    {
        GClientContext()->RemoteClientManager()->BCreateRemotePlayInviteAndSession(rppInvitee, gameID.AppID());
    }
    else
    {
        GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(true);

        m_enabledDesktopStreaming = true;

        GClientContext()->RemoteClientManager()->BCreateRemotePlayInviteAndSession(rppInvitee, m_nonsteamAppID);
    }
}

void RemotePlayInviteHandler::CancelInvite(CSteamID invitee, uint32 guestID)
{
    if(GClientContext()->RemoteClientManager()->BIsStreamingSessionActive())
    {
        RemotePlayPlayer_t rppInvitee = { invitee, guestID, m_groupID, 0, 0, 0 };
        GClientContext()->RemoteClientManager()->CancelRemotePlayInviteAndSession(rppInvitee);
    }
}

void RemotePlayInviteHandler::SetNonSteamAppID(AppId_t appID)
{
    m_nonsteamAppID = appID;
}

void RemotePlayInviteHandler::SetGuestID(uint32 guestID)
{
    m_remoteGuestID = guestID;
}

void RemotePlayInviteHandler::OnRemotePlayInviteResult(RemotePlayInviteResult_t* inviteResultCb)
{
    if (inviteResultCb->m_eResult == k_ERemoteClientLaunchResultOK)
    {
        if (inviteResultCb->m_player.m_playerID.IsValid())
        {
            char* buf = new char[1280];
            sprintf(buf, "Follow this link to join remote game: %s", inviteResultCb->m_szConnectURL);
            GClientContext()->SteamFriends()->ReplyToFriendMessage(inviteResultCb->m_player.m_playerID, buf);
            delete[] buf;
        }
        GClientContext()->RemoteClientManager()->ShowRemotePlayTogetherUI(GetRunningGameID().AppID());
    }
}

void RemotePlayInviteHandler::OnRemotePlayStop(RemoteClientStopStreamSession_t* streamStopCb)
{
    if (streamStopCb->m_player.m_playerID == GClientContext()->SteamUser()->GetSteamID())
    {
        GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(false);
        m_remoteGuestID = 1;
    }
}
