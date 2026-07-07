#ifndef REMOTEPLAYINVITEHANDLER_H
#define REMOTEPLAYINVITEHANDLER_H

#include <Steamworks.h>
#include <vector>

// A guest that has connected to our Remote Play Together session. We remember
// the exact RemotePlayPlayer_t Steam handed us on connect (correct SteamID,
// guest id and group id) plus the per-peripheral control state the host wants.
struct ConnectedGuest
{
    RemotePlayPlayer_t player;
    uint64 steamID;
    bool wantKbd;
    bool wantMouse;
    bool wantCtrl;
};

class RemotePlayInviteHandler
{
public:
    RemotePlayInviteHandler();
    virtual ~RemotePlayInviteHandler() {}

    void SendInvite(CSteamID invitee);
    void CancelInvite(CSteamID invitee, uint32 guestID);

    void SetNonSteamAppID(AppId_t appID);
    void SetGuestID(uint32 guestID);

    AppId_t GetNonSteamAppID();

    // Called periodically (from the app callback timer) to re-assert each
    // guest's desired input state so Steam can't silently reset it.
    void GrantControlTick();

    // Roster access + per-guest control, driven by the control window UI.
    const std::vector<ConnectedGuest>& Guests() const { return m_guests; }
    void SetGuestInput(uint64 steamID, bool kbd, bool mouse, bool ctrl);
    // Bumped whenever the roster changes (guest joins/leaves) so the UI can
    // tell when to rebuild itself.
    uint32 RosterRevision() const { return m_rosterRevision; }

private:
    void ApplyGuest(const ConnectedGuest& g);
    // Create/host a real Remote Play Together group and return its id (the
    // missing piece: stock RPW hardcodes groupID=1 without ever creating one).
    uint32 EnsureGroup(AppId_t appID);

    bool m_enabledDesktopStreaming;
    uint32 m_remoteGuestID;
    uint32 m_groupID;
    AppId_t m_nonsteamAppID;

    std::vector<ConnectedGuest> m_guests;
    uint32 m_rosterRevision;

    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayStart, RemoteClientStartStreamSession_t, m_remoteStartCb);
    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayStop, RemoteClientStopStreamSession_t, m_remoteStopCb);
    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayInviteResult, RemotePlayInviteResult_t, m_remoteInviteResultCb);
};

#endif // REMOTEPLAYINVITEHANDLER_H
