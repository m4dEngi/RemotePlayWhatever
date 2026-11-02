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
    uint64 key;         // unique per guest, see RemotePlayInviteHandler::GuestKey
    uint64 steamID;     // 0 for Steam Link guests who joined without a Steam account
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

    // Called every app-timer tick (~200ms). Advances async group creation and
    // fires any queued invites once the group is ready (so SendInvite never
    // blocks the UI). Cheap no-op when there's nothing pending.
    void GroupTick();

    // If Steam won't create a Remote Play Together group, the invite can only
    // fall back to stock RPW's default group, where guests can usually watch
    // but not play. The UI asks the host what to do; command-line (one-shot)
    // mode sends anyway, like stock RPW did.
    bool HasGroupFailure() const { return m_groupState == GroupFailed && !m_pending.empty(); }
    void ResolveGroupFailure(bool sendAnyway);
    void SetAutoFallback(bool sendAnyway) { m_autoFallback = sendAnyway; }

    // Called periodically (from the app callback timer) to re-assert each
    // guest's desired input state so Steam can't silently reset it.
    void GrantControlTick();

    // Roster access + per-guest control, driven by the control window UI.
    const std::vector<ConnectedGuest>& Guests() const { return m_guests; }
    void SetGuestInput(uint64 key, bool kbd, bool mouse, bool ctrl);

    // Guests with a Steam account are identified by their SteamID. Steam Link
    // guests without an account all report SteamID 0, so they're identified by
    // their guest slot instead (a value that can never be a real SteamID).
    static uint64 GuestKey(const RemotePlayPlayer_t& player);
    // Bumped whenever the roster changes (guest joins/leaves) so the UI can
    // tell when to rebuild itself.
    uint32 RosterRevision() const { return m_rosterRevision; }

private:
    void ApplyGuest(const ConnectedGuest& g);

    // Async group creation. Stock RPW hardcodes groupID=1 without ever creating
    // a group; we create/settle one first, then fire the queued invite(s).
    enum EGroupState { GroupNone, GroupWaiting, GroupReady, GroupFailed };
    struct PendingInvite { CSteamID invitee; AppId_t appID; };

    bool m_enabledDesktopStreaming;
    uint32 m_remoteGuestID;
    uint32 m_groupID;
    AppId_t m_nonsteamAppID;

    EGroupState m_groupState;
    AppId_t m_groupAppID;       // the game the current group was set up for
    int m_groupTicks;
    bool m_autoFallback;
    std::vector<PendingInvite> m_pending;

    std::vector<ConnectedGuest> m_guests;
    uint32 m_rosterRevision;

    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayStart, RemoteClientStartStreamSession_t, m_remoteStartCb);
    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayStop, RemoteClientStopStreamSession_t, m_remoteStopCb);
    STEAM_CALLBACK(RemotePlayInviteHandler, OnRemotePlayInviteResult, RemotePlayInviteResult_t, m_remoteInviteResultCb);
};

#endif // REMOTEPLAYINVITEHANDLER_H
