#include <wx/clipbrd.h>
#include "SteamStuff.h"

// steam client context

ClientContext::ClientContext():
    m_hPipe(0),
    m_hUser(0),
    m_pSteamUser(nullptr),
    m_pSteamFriends(nullptr),
    m_pSteamClient(nullptr),
    m_pClientEngine(nullptr),
    m_pClientRemoteManager(nullptr)
{
}

ClientContext::~ClientContext()
{
    if (m_hPipe)
    {
        if (m_hUser)
        {
            m_pSteamClient->ReleaseUser(m_hPipe, m_hUser);
        }
        m_pSteamClient->BReleaseSteamPipe(m_hPipe);
    }
    m_pSteamClient->BShutdownIfAllPipesClosed();
}

bool ClientContext::Init()
{
    if (!OpenAPI_LoadLibrary())
    {
        return false;
    }

    m_pSteamClient = (ISteamClient019*)SteamInternal_CreateInterface(STEAMCLIENT_INTERFACE_VERSION_019);
    if (!m_pSteamClient)
    {
        return false;
    }

    m_hPipe = m_pSteamClient->CreateSteamPipe();
    if (!m_hPipe)
    {
        return false;
    }

    m_hUser = m_pSteamClient->ConnectToGlobalUser(m_hPipe);
    if (!m_hUser)
    {
        return false;
    }

    m_pSteamUser = (ISteamUser019*)m_pSteamClient->GetISteamUser(m_hUser, m_hPipe, STEAMUSER_INTERFACE_VERSION_019);
    if (!m_pSteamUser)
    {
        return false;
    }

    m_pSteamFriends = (ISteamFriends015*)m_pSteamClient->GetISteamFriends(m_hUser, m_hPipe, STEAMFRIENDS_INTERFACE_VERSION_015);
    if (!m_pSteamFriends)
    {
        return false;
    }

    m_pClientEngine = (IClientEngine*)SteamInternal_CreateInterface(CLIENTENGINE_INTERFACE_VERSION);
    if (!m_pClientEngine)
    {
        return false;
    }

    m_pClientRemoteManager = (IClientRemoteClientManager*)m_pClientEngine->GetIClientRemoteClientManager(m_hPipe);
    if (!m_pClientRemoteManager)
    {
        return false;
    }

    return true;
}

ISteamUser019* ClientContext::SteamUser()
{
    return m_pSteamUser;
}

ISteamFriends015* ClientContext::SteamFriends()
{
    return m_pSteamFriends;
}

void ClientContext::RunCallbacks()
{
    Steam_RunCallbacks(m_hPipe, false);
}

IClientRemoteClientManager* ClientContext::RemoteClientManager()
{
    return m_pClientRemoteManager;
}

// helper functions

ClientContext* GClientContext()
{
    static ClientContext ctx;
    return &ctx;
}

CGameID GetRunningGameID()
{
    FriendGameInfo_t gameInfo;
    if(GClientContext()->SteamFriends()->GetFriendGamePlayed(GClientContext()->SteamUser()->GetSteamID(), &gameInfo))
    {
        return gameInfo.m_gameID;
    }

    return CGameID();
}
