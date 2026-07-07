#include "SteamStuff.h"
#include "RemotePlayInviteHandler.h"
#include <wx/utils.h>   // wxMilliSleep (cross-platform)

RemotePlayInviteHandler::RemotePlayInviteHandler() :
    m_enabledDesktopStreaming(false),
    m_remoteGuestID(1),
    m_nonsteamAppID(0),
    m_groupID(1),
    m_rosterRevision(0),
    m_remoteStartCb(this, &RemotePlayInviteHandler::OnRemotePlayStart),
    m_remoteStopCb(this, &RemotePlayInviteHandler::OnRemotePlayStop),
    m_remoteInviteResultCb(this, &RemotePlayInviteHandler::OnRemotePlayInviteResult)
{
}

AppId_t RemotePlayInviteHandler::GetNonSteamAppID()
{
    return m_nonsteamAppID;
}

// Create/host a real group, polling GetLocalRemotePlayTogetherGroupID while it
// settles (reading it during the wait is what makes Steam bind the group), then
// BCreateRemotePlayGroup and poll again. Cached so only the first invite pays
// the settle cost. Returns the real group id (falls back to 1 if none appears).
uint32 RemotePlayInviteHandler::EnsureGroup(AppId_t appID)
{
    CRemoteClientManagerByName* rcm = GClientContext()->RemoteClientManager();

    uint32 g = rcm->GetLocalRemotePlayTogetherGroupID();
    if (g)
    {
        return g;
    }

    if (appID)
    {
        rcm->ShowRemotePlayTogetherUI(appID);
    }

    for (int i = 0; i < 12 && !g; ++i)
    {
        GClientContext()->RunCallbacks();
        wxMilliSleep(200);
        g = rcm->GetLocalRemotePlayTogetherGroupID();
    }

    if (!g)
    {
        rcm->BCreateRemotePlayGroup();
        for (int i = 0; i < 20 && !g; ++i)
        {
            GClientContext()->RunCallbacks();
            wxMilliSleep(200);
            g = rcm->GetLocalRemotePlayTogetherGroupID();
        }
    }

    return g ? g : 1;
}

void RemotePlayInviteHandler::SendInvite(CSteamID invitee)
{
    CGameID gameID = GetRunningGameID();

    if (!gameID.IsValid())
    {
        return;
    }

    bool desktop = !(gameID.IsSteamApp() && gameID.AppID() != m_nonsteamAppID);
    AppId_t appID = desktop ? m_nonsteamAppID : gameID.AppID();

    if (desktop)
    {
        GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(true);
        m_enabledDesktopStreaming = true;
    }

    // Host a real group and take the next guest slot Steam offers for it, so the
    // guest joins a controllable session (not the phantom groupID=1).
    m_groupID = EnsureGroup(appID);
    uint32 guest = GClientContext()->RemoteClientManager()->GetAvailableRemotePlayTogetherGuestID(m_groupID);
    if (!guest)
    {
        guest = m_remoteGuestID;
    }
    m_remoteGuestID = guest + 1;

    RemotePlayPlayer_t rppInvitee = { invitee, guest, m_groupID, 0, 0, 0 };
    GClientContext()->RemoteClientManager()->BCreateRemotePlayInviteAndSession(rppInvitee, appID);
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

// Enable the guest's chosen peripherals. SetPerUser* takes the exact
// RemotePlayPlayer_t Steam gave us on connect; on drifted builds this is the
// only way to hand a guest control (they never appear in the overlay list).
void RemotePlayInviteHandler::ApplyGuest(const ConnectedGuest& g)
{
    CRemoteClientManagerByName* rcm = GClientContext()->RemoteClientManager();
    rcm->SetPerUserKeyboardInputEnabled(g.player, g.wantKbd);
    rcm->SetPerUserMouseInputEnabled(g.player, g.wantMouse);
    rcm->SetPerUserControllerInputEnabled(g.player, g.wantCtrl);
}

void RemotePlayInviteHandler::GrantControlTick()
{
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        ApplyGuest(m_guests[i]);
    }
}

void RemotePlayInviteHandler::SetGuestInput(uint64 steamID, bool kbd, bool mouse, bool ctrl)
{
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        if (m_guests[i].steamID == steamID)
        {
            m_guests[i].wantKbd = kbd;
            m_guests[i].wantMouse = mouse;
            m_guests[i].wantCtrl = ctrl;
            ApplyGuest(m_guests[i]);
            return;
        }
    }
}

// A guest connected. Steam hands us their real RemotePlayPlayer_t (SteamID,
// guest id, group id) in the callback, so we can grant control immediately.
void RemotePlayInviteHandler::OnRemotePlayStart(RemoteClientStartStreamSession_t* cb)
{
    uint64 id = cb->m_player.m_playerID.ConvertToUint64();

    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        if (m_guests[i].steamID == id)
        {
            m_guests[i].player = cb->m_player;  // refresh with latest struct
            ApplyGuest(m_guests[i]);
            return;
        }
    }

    ConnectedGuest g;
    g.player = cb->m_player;
    g.steamID = id;
    g.wantKbd = true;
    g.wantMouse = true;
    g.wantCtrl = true;              // default: give a new guest full control
    m_guests.push_back(g);
    ++m_rosterRevision;
    ApplyGuest(g);
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
        m_guests.clear();
        ++m_rosterRevision;
        return;
    }

    uint64 stoppedId = streamStopCb->m_player.m_playerID.ConvertToUint64();
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        if (m_guests[i].steamID == stoppedId)
        {
            m_guests.erase(m_guests.begin() + i);
            ++m_rosterRevision;
            break;
        }
    }
}
