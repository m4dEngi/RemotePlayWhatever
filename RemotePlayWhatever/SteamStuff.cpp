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
    m_pClientRemoteManager(nullptr),
    m_ShuttingDown(false)
{
}

ClientContext::~ClientContext()
{
}

bool ClientContext::Init()
{
    if(m_Initialized)
    {
        return true;
    }

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

    m_pSteamUtils = (ISteamUtils009*)m_pSteamClient->GetISteamUtils(m_hPipe, STEAMUTILS_INTERFACE_VERSION_009);
    if(!m_pSteamUtils)
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

    return m_Initialized = true;
}

void ClientContext::Shutdown()
{
    if(m_ShuttingDown)
    {
        return;
    }

    m_ShuttingDown = true;

    if (m_hPipe)
    {
        if (m_hUser)
        {
            m_pSteamClient->ReleaseUser(m_hPipe, m_hUser);
        }
        if(m_pSteamClient->BReleaseSteamPipe(m_hPipe))
        {
            m_pSteamClient->BShutdownIfAllPipesClosed();
        }
    }
}

ISteamUser019* ClientContext::SteamUser()
{
    return m_pSteamUser;
}

ISteamFriends015* ClientContext::SteamFriends()
{
    return m_pSteamFriends;
}

ISteamUtils009 *ClientContext::SteamUtils()
{
    return m_pSteamUtils;
}

void ClientContext::RunCallbacks()
{
    if(!m_ShuttingDown)
    {
        Steam_RunCallbacks(m_hPipe, false);
    }
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


