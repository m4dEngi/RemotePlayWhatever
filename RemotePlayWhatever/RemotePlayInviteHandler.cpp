#include <wx/clipbrd.h>
#include <wx/msgdlg.h> 
#include "SteamStuff.h"
#include "RemotePlayInviteHandler.h"

RemotePlayInviteHandler::RemotePlayInviteHandler() :
    m_enabledDesktopStreaming(false),
    m_remoteGuestID(1),
    m_nonsteamAppID(480),
    m_remoteInviteResultCb(this, &RemotePlayInviteHandler::OnRemotePlayInviteResult),
    m_remoteStopCb(this, &RemotePlayInviteHandler::OnRemotePlayStop)
{
}

void RemotePlayInviteHandler::SendInvite(CSteamID invitee)
{
    CGameID gameID = GetRunningGameID();

    if (!gameID.IsValid())
    {
        return;
    }

    RemotePlayPlayer_t rppInvitee = { invitee, m_remoteGuestID, 0, 0, 0 };
    ++m_remoteGuestID;

    if (gameID.IsSteamApp() && gameID.AppID() != m_nonsteamAppID)
    {
        GClientContext()->RemoteClientManager()->CreateRemotePlayInviteAndSession(rppInvitee, gameID.AppID());
    }
    else
    {
        GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(true);

        m_enabledDesktopStreaming = true;

        GClientContext()->RemoteClientManager()->CreateRemotePlayInviteAndSession(rppInvitee, m_nonsteamAppID);
    }
}

void RemotePlayInviteHandler::OnRemotePlayInviteResult(RemotePlayInviteResult_t* inviteResultCb)
{
    if (inviteResultCb->m_eResult != k_ERemoteClientLaunchResultOK)
    {
        wxMessageBox(
            wxString::Format("Could not create remote play session! (Result:%d)", inviteResultCb->m_eResult), 
            "Remote Play Whatever",
            wxOK | wxICON_ERROR
        );

        return;
    }

    char* buf = new char[1280];
    sprintf(buf, "Follow this link to join remote game: %s", inviteResultCb->m_szConnectURL);
    GClientContext()->SteamFriends()->ReplyToFriendMessage(inviteResultCb->m_player.m_playerID, buf);
    delete[] buf;
}

void RemotePlayInviteHandler::OnRemotePlayStop(RemoteClientStopStreamSession_t* streamStopCb)
{
    if (!GClientContext()->RemoteClientManager()->BIsStreamingSessionActive() &&
        m_enabledDesktopStreaming
       )
    {
        GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(false);
        m_remoteGuestID = 1;
    }
}
