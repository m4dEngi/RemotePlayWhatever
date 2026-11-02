#ifndef STEAMSTUFF_H
#define STEAMSTUFF_H

#include <string>
#include <Steamworks.h>
#include "ClientVTableByName.h"

// IClientRemoteClientManager drifts between Steam builds, so instead of calling
// it through the compiler-fixed vtable order we resolve each method by name at
// runtime (see ClientVTableByName.h) and call through the resolved index. The
// method set below is exactly what this app uses; the call sites are unchanged.
class CRemoteClientManagerByName : public clientvt::NamedVTable
{
public:
    bool BCreateRemotePlayInviteAndSession(RemotePlayPlayer_t player, AppId_t appID)
    {
        return Invoke<bool>("BCreateRemotePlayInviteAndSession", player, appID);
    }
    void CancelRemotePlayInviteAndSession(RemotePlayPlayer_t player)
    {
        // This method's stub doesn't embed its name on current builds, so it
        // can't be looked up directly. It sits immediately between the two named
        // methods BCreateRemotePlayInviteAndSession and JoinRemotePlaySession
        // (…Create, Cancel, Join…). If those anchors bracket exactly one slot,
        // that slot is Cancel; otherwise the layout changed and we skip rather
        // than risk calling the wrong method.
        int bc = IndexOf("BCreateRemotePlayInviteAndSession");
        int jn = IndexOf("JoinRemotePlaySession");
        if (bc >= 0 && jn == bc + 2)
        {
            InvokeAt<void>(bc + 1, player);
        }
    }
    bool BIsStreamingSessionActive()
    {
        return Invoke<bool>("BIsStreamingSessionActive");
    }
    void SetStreamingDesktopToRemotePlayTogetherEnabled(bool enabled)
    {
        Invoke<void>("SetStreamingDesktopToRemotePlayTogetherEnabled", enabled);
    }
    void ShowRemotePlayTogetherUI(AppId_t appID)
    {
        Invoke<void>("ShowRemotePlayTogetherUI", appID);
    }
    // --- Remote Play Together group + per-user input control ---------------
    // On drifted builds the guest never appears in the Steam overlay player
    // list, so granting control through the overlay is impossible; the only
    // way to give a guest input is to enable it per-user programmatically.
    uint32 GetLocalRemotePlayTogetherGroupID()
    {
        return Invoke<uint32>("GetLocalRemotePlayTogetherGroupID", (int32)0);
    }
    bool BCreateRemotePlayGroup()
    {
        return Invoke<bool>("BCreateRemotePlayGroup", (void*)0);
    }
    uint32 GetAvailableRemotePlayTogetherGuestID(uint32 group)
    {
        return Invoke<uint32>("GetAvailableRemotePlayTogetherGuestID", group);
    }
    // These take the 40-byte RemotePlayPlayer_t by value (passed by hidden
    // pointer per the x64 ABI) — NOT (group,guest) ints, which crashes.
    void SetPerUserKeyboardInputEnabled(RemotePlayPlayer_t player, bool enabled)
    {
        Invoke<void>("SetPerUserKeyboardInputEnabled", player, enabled);
    }
    void SetPerUserMouseInputEnabled(RemotePlayPlayer_t player, bool enabled)
    {
        Invoke<void>("SetPerUserMouseInputEnabled", player, enabled);
    }
    void SetPerUserControllerInputEnabled(RemotePlayPlayer_t player, bool enabled)
    {
        Invoke<void>("SetPerUserControllerInputEnabled", player, enabled);
    }
    void DisbandRemotePlayTogetherGroup(uint32 group)
    {
        Invoke<void>("DisbandRemotePlayTogetherGroup", group);
    }
};

class ClientContext
{
public:
    ClientContext();
    ~ClientContext();

    ISteamUser019* SteamUser();
    ISteamFriends015* SteamFriends();
    ISteamUtils009* SteamUtils();

    CRemoteClientManagerByName* RemoteClientManager();

    bool Init();
    void Shutdown();
    void RunCallbacks();

private:
    HSteamPipe m_hPipe;
    HSteamUser m_hUser;

    ISteamClient019* m_pSteamClient;
    ISteamUser019* m_pSteamUser;
    ISteamFriends015* m_pSteamFriends;
    ISteamUtils009* m_pSteamUtils;

    IClientEngine* m_pClientEngine;
    CRemoteClientManagerByName m_remoteManager;

    bool m_ShuttingDown;
    bool m_Initialized;
};

ClientContext* GClientContext();
CGameID GetRunningGameID();

// Steam hands out Remote Play links without a scheme ("s.team/p/..."), which chat
// apps don't turn into clickable links. Adds https:// when no scheme is present.
std::string ClickableURL(const char* url);

#endif // !STEAMSTUFF_H
