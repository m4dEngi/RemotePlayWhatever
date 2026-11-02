#ifndef STEAMSTUFF_H
#define STEAMSTUFF_H

#include <string>
#include <Steamworks.h>
#include "ClientVTableByName.h"

// IClientRemoteClientManager drifts between Steam builds, so instead of calling
// it through the compiler-fixed vtable order we resolve each method by name at
// runtime (see ClientVTableByName.h) and call through the resolved index. The
// method set below is exactly what this app uses; the call sites are unchanged.
//
// Every wrapper checks Has() first: calling a name Steam no longer has would
// otherwise jump to vtable slot 0 with the wrong arguments. A missing method
// becomes a no-op (or false/0) instead of a crash.
class CRemoteClientManagerByName : public clientvt::NamedVTable
{
public:
    bool BCreateRemotePlayInviteAndSession(RemotePlayPlayer_t player, AppId_t appID)
    {
        return Has("BCreateRemotePlayInviteAndSession") &&
               Invoke<bool>("BCreateRemotePlayInviteAndSession", player, appID);
    }
    void CancelRemotePlayInviteAndSession(RemotePlayPlayer_t player)
    {
        // Current builds embed this method's name. Older builds didn't, so fall
        // back to the slot between its two named neighbours
        // BCreateRemotePlayInviteAndSession and JoinRemotePlaySession
        // (…Create, Cancel, Join…). If those anchors don't bracket exactly one
        // slot, the layout changed and we skip rather than call the wrong method.
        if (Has("CancelRemotePlayInviteAndSession"))
        {
            Invoke<void>("CancelRemotePlayInviteAndSession", player);
            return;
        }
        int bc = IndexOf("BCreateRemotePlayInviteAndSession");
        int jn = IndexOf("JoinRemotePlaySession");
        if (bc >= 0 && jn == bc + 2)
        {
            InvokeAt<void>(bc + 1, player);
        }
    }
    void StopStreamingSession(uint32 sessionID)
    {
        if (Has("StopStreamingSession"))
        {
            Invoke<void>("StopStreamingSession", sessionID);
        }
    }
    bool BIsStreamingSessionActive()
    {
        return Has("BIsStreamingSessionActive") && Invoke<bool>("BIsStreamingSessionActive");
    }
    bool BStreamingDesktopToRemotePlayTogetherEnabled()
    {
        return Has("BStreamingDesktopToRemotePlayTogetherEnabled") &&
               Invoke<bool>("BStreamingDesktopToRemotePlayTogetherEnabled");
    }
    void SetStreamingDesktopToRemotePlayTogetherEnabled(bool enabled)
    {
        if (Has("SetStreamingDesktopToRemotePlayTogetherEnabled"))
        {
            Invoke<void>("SetStreamingDesktopToRemotePlayTogetherEnabled", enabled);
        }
    }
    void ShowRemotePlayTogetherUI(AppId_t appID)
    {
        if (Has("ShowRemotePlayTogetherUI"))
        {
            Invoke<void>("ShowRemotePlayTogetherUI", appID);
        }
    }
    // --- Remote Play Together group + per-user input control ---------------
    // On drifted builds the guest never appears in the Steam overlay player
    // list, so granting control through the overlay is impossible; the only
    // way to give a guest input is to enable it per-user programmatically.
    uint32 GetLocalRemotePlayTogetherGroupID()
    {
        return Has("GetLocalRemotePlayTogetherGroupID") ?
               Invoke<uint32>("GetLocalRemotePlayTogetherGroupID", (int32)0) : 0;
    }
    bool BCreateRemotePlayGroup()
    {
        return Has("BCreateRemotePlayGroup") && Invoke<bool>("BCreateRemotePlayGroup", (void*)0);
    }
    uint32 GetAvailableRemotePlayTogetherGuestID(uint32 group)
    {
        return Has("GetAvailableRemotePlayTogetherGuestID") ?
               Invoke<uint32>("GetAvailableRemotePlayTogetherGuestID", group) : 0;
    }
    // These take the 40-byte RemotePlayPlayer_t by value (passed by hidden
    // pointer per the x64 ABI) — NOT (group,guest) ints, which crashes.
    void SetPerUserKeyboardInputEnabled(RemotePlayPlayer_t player, bool enabled)
    {
        if (Has("SetPerUserKeyboardInputEnabled"))
        {
            Invoke<void>("SetPerUserKeyboardInputEnabled", player, enabled);
        }
    }
    void SetPerUserMouseInputEnabled(RemotePlayPlayer_t player, bool enabled)
    {
        if (Has("SetPerUserMouseInputEnabled"))
        {
            Invoke<void>("SetPerUserMouseInputEnabled", player, enabled);
        }
    }
    void SetPerUserControllerInputEnabled(RemotePlayPlayer_t player, bool enabled)
    {
        if (Has("SetPerUserControllerInputEnabled"))
        {
            Invoke<void>("SetPerUserControllerInputEnabled", player, enabled);
        }
    }
    void DisbandRemotePlayTogetherGroup(uint32 group)
    {
        if (Has("DisbandRemotePlayTogetherGroup"))
        {
            Invoke<void>("DisbandRemotePlayTogetherGroup", group);
        }
    }
};

class ClientContext
{
public:
    // Why Init() failed, so the app can say something more useful than
    // "could not initialize".
    enum EInitError
    {
        InitOK,
        InitSteamNotRunning,    // Steam isn't installed or isn't running
        InitNotSignedIn,        // Steam is running but nobody is signed in
        InitMethodsMissing,     // Steam changed the functions this app needs
    };

    ClientContext();
    ~ClientContext();

    ISteamUser019* SteamUser();
    ISteamFriends015* SteamFriends();
    ISteamUtils009* SteamUtils();

    CRemoteClientManagerByName* RemoteClientManager();

    bool Init();
    EInitError InitError() const { return m_initError; }
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
    EInitError m_initError;
};

ClientContext* GClientContext();
CGameID GetRunningGameID();

// Steam hands out Remote Play links without a scheme ("s.team/p/..."), which chat
// apps don't turn into clickable links. Adds https:// when no scheme is present.
std::string ClickableURL(const char* url);

#endif // !STEAMSTUFF_H
