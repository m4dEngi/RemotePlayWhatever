#ifndef CLIENTCONTEXT_H
#define CLIENTCONTEXT_H

#include <Steamworks.h>

class ClientContext
{
public:
    ClientContext();
    ~ClientContext();

    IClientUser* ClientUser();
    IClientFriends* ClientFriends();
    IClientRemoteClientManager* RemoteClientManager();

    bool Init();
    void RunCallbacks();

private:
    HSteamPipe m_hPipe;
    HSteamUser m_hUser;
    
    IClientEngine* m_pClientEngine;
    IClientUser* m_pClientUser;
    IClientRemoteClientManager* m_pClientRemoteManager;
    IClientFriends* m_pClientFriends;
};

ClientContext* GClientContext();


#endif // !CLIENTCONTEXT_H
