#include <iostream>
#include "SteamStuff.h"
#include "RemotePlayInviteHandler.h"

RemotePlayInviteHandler::RemotePlayInviteHandler() :
    m_enabledDesktopStreaming(false),
    m_remoteGuestID(1),
    m_nonsteamAppID(0),
    m_groupID(1),
    m_groupState(GroupNone),
    m_groupAppID(0),
    m_groupTicks(0),
    m_autoFallback(false),
    m_sessionGame(0),
    m_sessionGameGoneTicks(0),
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

// Queue an invite and kick off async group creation. The actual Steam invite is
// fired from GroupTick() once the group is ready, so this returns immediately
// and never blocks the UI (stock RPW hardcoded groupID=1 and never created one).
void RemotePlayInviteHandler::SendInvite(CSteamID invitee)
{
    CGameID gameID = GetRunningGameID();

    if (!gameID.IsValid())
    {
        return;
    }

    bool desktop = !(gameID.IsSteamApp() && gameID.AppID() != m_nonsteamAppID);
    AppId_t appID = desktop ? m_nonsteamAppID : gameID.AppID();

    // Leftovers from a session for a game that has since closed or changed.
    if (m_sessionGame && m_sessionGame != gameID.ToUint64())
    {
        EndSession();
    }
    m_sessionGame = gameID.ToUint64();
    m_sessionGameGoneTicks = 0;

    if (desktop && !m_enabledDesktopStreaming)
    {
        GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(true);
        m_enabledDesktopStreaming = true;
    }

    CRemoteClientManagerByName* rcm = GClientContext()->RemoteClientManager();

    // A group from an earlier invite can be gone by now (everyone left, or the
    // game was restarted or switched). Check it again instead of inviting into
    // a group that no longer exists, which leaves guests only able to watch.
    if (m_groupState == GroupReady || m_groupState == GroupFailed)
    {
        uint32 live = rcm->GetLocalRemotePlayTogetherGroupID();
        if (live && appID == m_groupAppID)
        {
            m_groupID = live;
            m_groupState = GroupReady;
        }
        else
        {
            m_groupState = GroupNone;
        }
    }

    PendingInvite pi;
    pi.invitee = invitee;
    pi.appID = appID;
    m_pending.push_back(pi);

    if (m_groupState == GroupNone)
    {
        m_groupAppID = appID;
        uint32 g = rcm->GetLocalRemotePlayTogetherGroupID();
        if (g)
        {
            m_groupID = g;
            m_groupState = GroupReady;      // reuse an already-hosted group
        }
        else
        {
            if (appID)
            {
                rcm->ShowRemotePlayTogetherUI(appID);
            }
            m_groupState = GroupWaiting;
            m_groupTicks = 0;
        }
    }
    // If Waiting, the invite goes out once the group is ready.
}

// Drives async group creation and flushes queued invites. Runs every ~200ms.
void RemotePlayInviteHandler::GroupTick()
{
    if (m_groupState == GroupWaiting)
    {
        CRemoteClientManagerByName* rcm = GClientContext()->RemoteClientManager();
        uint32 g = rcm->GetLocalRemotePlayTogetherGroupID();  // polling settles it
        if (g)
        {
            m_groupID = g;
            m_groupState = GroupReady;
        }
        else
        {
            ++m_groupTicks;
            // Give Steam ~2.4s to set the group up after showing its UI, then
            // ask it to create one, and ask again at ~5s in case it didn't.
            if (m_groupTicks == 12 || m_groupTicks == 25)
            {
                rcm->BCreateRemotePlayGroup();
            }
            if (m_groupTicks >= 50)                        // ~10s and still no group
            {
                m_groupState = GroupFailed;
                if (m_autoFallback)
                {
                    std::cout << "Warning: Steam didn't create a Remote Play Together group; "
                                 "inviting anyway (guests may only be able to watch)." << std::endl;
                    ResolveGroupFailure(true);
                }
            }
        }
    }

    if (m_groupState == GroupReady && !m_pending.empty())
    {
        CRemoteClientManagerByName* rcm = GClientContext()->RemoteClientManager();
        for (size_t i = 0; i < m_pending.size(); ++i)
        {
            uint32 guest = rcm->GetAvailableRemotePlayTogetherGuestID(m_groupID);
            if (!guest)
            {
                guest = m_remoteGuestID;
            }
            m_remoteGuestID = guest + 1;

            RemotePlayPlayer_t rppInvitee = { m_pending[i].invitee, guest, m_groupID, 0, 0, 0 };
            rcm->BCreateRemotePlayInviteAndSession(rppInvitee, m_pending[i].appID);
            m_invited.push_back(rppInvitee);
            clientvt::Log("[rp] invite sent: steam=%llu guest=%u group=%u app=%u\n",
                          (unsigned long long)rppInvitee.m_playerID.ConvertToUint64(), guest, m_groupID,
                          (unsigned)m_pending[i].appID);
        }
        m_pending.clear();
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

bool RemotePlayInviteHandler::IsSessionActive() const
{
    return !m_guests.empty() || !m_invited.empty() || !m_pending.empty() ||
           m_enabledDesktopStreaming || m_groupState == GroupWaiting;
}

void RemotePlayInviteHandler::EndSession()
{
    CRemoteClientManagerByName* rcm = GClientContext()->RemoteClientManager();

    // Connected guests: end their streams and their places in the session.
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        if (m_guests[i].sessionID)
        {
            rcm->StopStreamingSession(m_guests[i].sessionID);
        }
        rcm->CancelRemotePlayInviteAndSession(m_guests[i].player);
    }

    // Invites nobody has used yet, including guest links.
    for (size_t i = 0; i < m_invited.size(); ++i)
    {
        rcm->CancelRemotePlayInviteAndSession(m_invited[i]);
    }

    if (m_enabledDesktopStreaming || rcm->BStreamingDesktopToRemotePlayTogetherEnabled())
    {
        rcm->SetStreamingDesktopToRemotePlayTogetherEnabled(false);
    }

    ResetSession();
}

void RemotePlayInviteHandler::ResetSession()
{
    m_enabledDesktopStreaming = false;
    m_sessionGame = 0;
    m_sessionGameGoneTicks = 0;
    m_remoteGuestID = 1;
    m_groupState = GroupNone;
    m_pending.clear();
    m_invited.clear();
    m_guests.clear();
    ++m_rosterRevision;
}

void RemotePlayInviteHandler::SessionTick()
{
    if (!m_sessionGame || !IsSessionActive())
    {
        return;
    }

    CGameID running = GetRunningGameID();
    if (running.IsValid() && running.ToUint64() == m_sessionGame)
    {
        m_sessionGameGoneTicks = 0;
        return;
    }

    // Give it ~3s in case Steam briefly reports no game (e.g. while an
    // emulator recreates its window).
    if (++m_sessionGameGoneTicks >= 15)
    {
        bool desktop = m_enabledDesktopStreaming;
        EndSession();
        m_notice = desktop ?
            "The game closed, so screen sharing stopped and everyone was disconnected." :
            "The game closed, so the Remote Play session ended.";
    }
}

std::string RemotePlayInviteHandler::TakeNotice()
{
    std::string notice;
    notice.swap(m_notice);
    return notice;
}

void RemotePlayInviteHandler::ResolveGroupFailure(bool sendAnyway)
{
    if (m_groupState != GroupFailed)
    {
        return;
    }

    if (sendAnyway)
    {
        // Stock RPW behaviour: invite with the default group id.
        m_groupID = 1;
        m_groupState = GroupReady;
    }
    else
    {
        m_pending.clear();
        m_groupState = GroupNone;
        // Nothing was sent, so don't leave the screen shared for nobody.
        if (m_guests.empty() && m_invited.empty())
        {
            EndSession();
        }
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

void RemotePlayInviteHandler::SetGuestInput(uint64 key, bool kbd, bool mouse, bool ctrl)
{
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        if (m_guests[i].key == key)
        {
            m_guests[i].wantKbd = kbd;
            m_guests[i].wantMouse = mouse;
            m_guests[i].wantCtrl = ctrl;
            m_guests[i].hasSaved = false;
            ApplyGuest(m_guests[i]);
            ++m_rosterRevision;
            return;
        }
    }
}

static bool& Peripheral(ConnectedGuest& g, int which)
{
    if (which == 0) return g.wantKbd;
    if (which == 1) return g.wantMouse;
    return g.wantCtrl;
}

bool RemotePlayInviteHandler::AllGuestsHave(int which) const
{
    if (m_guests.empty())
    {
        return false;
    }
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        const ConnectedGuest& g = m_guests[i];
        bool on = which == 0 ? g.wantKbd : which == 1 ? g.wantMouse : g.wantCtrl;
        if (!on)
        {
            return false;
        }
    }
    return true;
}

void RemotePlayInviteHandler::SetAllGuestsInput(int which, bool on)
{
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        Peripheral(m_guests[i], which) = on;
        m_guests[i].hasSaved = false;
        ApplyGuest(m_guests[i]);
    }
    ++m_rosterRevision;
}

void RemotePlayInviteHandler::TakeAllControl()
{
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        ConnectedGuest& g = m_guests[i];
        if (!g.hasSaved)
        {
            g.savedKbd = g.wantKbd;
            g.savedMouse = g.wantMouse;
            g.savedCtrl = g.wantCtrl;
            g.hasSaved = true;
        }
        g.wantKbd = g.wantMouse = g.wantCtrl = false;
        ApplyGuest(g);
    }
    ++m_rosterRevision;
}

void RemotePlayInviteHandler::GiveBackControl()
{
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        ConnectedGuest& g = m_guests[i];
        if (g.hasSaved)
        {
            g.wantKbd = g.savedKbd;
            g.wantMouse = g.savedMouse;
            g.wantCtrl = g.savedCtrl;
            g.hasSaved = false;
        }
        else
        {
            g.wantKbd = g.wantMouse = g.wantCtrl = true;
        }
        ApplyGuest(g);
    }
    ++m_rosterRevision;
}

uint64 RemotePlayInviteHandler::GuestKey(const RemotePlayPlayer_t& player)
{
    if (player.m_playerID.GetAccountID() != 0)
    {
        return player.m_playerID.ConvertToUint64();
    }
    return 0xFFFFFFFF00000000ULL | player.m_unGuestID;
}

// A guest connected. Steam hands us their real RemotePlayPlayer_t (SteamID,
// guest id, group id) in the callback, so we can grant control immediately.
void RemotePlayInviteHandler::OnRemotePlayStart(RemoteClientStartStreamSession_t* cb)
{
    clientvt::Log("[rp] stream start: session=%u steam=%llu guest=%u group=%u game=%llu rpt=%d video=%d input=%d"
                  " | liveGroup=%u state=%d invited=%zu\n",
                  cb->m_unStreamingSessionID, (unsigned long long)cb->m_player.m_playerID.ConvertToUint64(),
                  cb->m_player.m_unGuestID, cb->m_player.m_unOverlayPIDGroupID,
                  (unsigned long long)cb->m_GameID.ToUint64(), (int)cb->m_bRemotePlayTogether,
                  (int)cb->m_bEnableVideoStreaming, (int)cb->m_bEnableInputStreaming,
                  m_groupID, (int)m_groupState, m_invited.size());

    // Our own stream isn't a guest.
    if (cb->m_player.m_playerID.GetAccountID() != 0 &&
        cb->m_player.m_playerID == GClientContext()->SteamUser()->GetSteamID())
    {
        return;
    }

    uint64 key = GuestKey(cb->m_player);

    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        if (m_guests[i].key == key)
        {
            m_guests[i].player = cb->m_player;  // refresh with latest struct
            m_guests[i].sessionID = cb->m_unStreamingSessionID;
            ApplyGuest(m_guests[i]);
            return;
        }
    }

    ConnectedGuest g;
    g.player = cb->m_player;
    g.key = key;
    g.steamID = cb->m_player.m_playerID.GetAccountID() != 0 ? cb->m_player.m_playerID.ConvertToUint64() : 0;
    g.sessionID = cb->m_unStreamingSessionID;
    g.wantKbd = true;
    g.wantMouse = true;
    g.wantCtrl = true;              // default: give a new guest full control
    g.hasSaved = false;
    g.savedKbd = g.savedMouse = g.savedCtrl = false;
    m_guests.push_back(g);
    ++m_rosterRevision;

    if (g.steamID)
    {
        // Someone who joined through a guest link may not be a friend, so ask
        // Steam for their name for the control window.
        GClientContext()->SteamFriends()->RequestUserInformation(cb->m_player.m_playerID, true);
    }
    ApplyGuest(g);
}

void RemotePlayInviteHandler::OnRemotePlayInviteResult(RemotePlayInviteResult_t* inviteResultCb)
{
    if (inviteResultCb->m_eResult == k_ERemoteClientLaunchResultOK)
    {
        if (inviteResultCb->m_player.m_playerID.IsValid())
        {
            std::string msg = "Follow this link to join remote game: " + ClickableURL(inviteResultCb->m_szConnectURL);
            GClientContext()->SteamFriends()->ReplyToFriendMessage(inviteResultCb->m_player.m_playerID, msg.c_str());
        }
        GClientContext()->RemoteClientManager()->ShowRemotePlayTogetherUI(GetRunningGameID().AppID());
    }
}

void RemotePlayInviteHandler::OnRemotePlayStop(RemoteClientStopStreamSession_t* streamStopCb)
{
    clientvt::Log("[rp] stream stop: session=%u steam=%llu guest=%u group=%u\n",
                  streamStopCb->m_unStreamingSessionID,
                  (unsigned long long)streamStopCb->m_player.m_playerID.ConvertToUint64(),
                  streamStopCb->m_player.m_unGuestID, streamStopCb->m_player.m_unOverlayPIDGroupID);

    if (streamStopCb->m_player.m_playerID == GClientContext()->SteamUser()->GetSteamID())
    {
        // The whole session is gone; a future invite must create a fresh group.
        GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(false);
        ResetSession();
        return;
    }

    uint64 stoppedKey = GuestKey(streamStopCb->m_player);
    for (size_t i = 0; i < m_guests.size(); ++i)
    {
        if (m_guests[i].key == stoppedKey)
        {
            m_guests.erase(m_guests.begin() + i);
            ++m_rosterRevision;
            break;
        }
    }
}
