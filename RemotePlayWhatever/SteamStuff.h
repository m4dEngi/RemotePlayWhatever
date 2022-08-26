#ifndef STEAMSTUFF_H
#define STEAMSTUFF_H

#include <Steamworks.h>

class ClientContext
{
public:
    ClientContext();
    ~ClientContext();

    ISteamUser019* SteamUser();
    ISteamFriends015* SteamFriends();
    ISteamUtils009* SteamUtils();

    IClientRemoteClientManager* RemoteClientManager();

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
    IClientRemoteClientManager* m_pClientRemoteManager;
    
    bool m_ShuttingDown;
    bool m_Initialized;
};

ClientContext* GClientContext();
CGameID GetRunningGameID();

#endif // !STEAMSTUFF_H
