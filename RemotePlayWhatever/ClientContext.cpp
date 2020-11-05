#include "ClientContext.h"

ClientContext::ClientContext():
    m_hPipe(0),
    m_hUser(0),
    m_pClientEngine(nullptr),
    m_pClientUser(nullptr),
    m_pClientRemoteManager(nullptr),
    m_pClientFriends(nullptr)
{
}

ClientContext::~ClientContext()
{
    if (m_hPipe)
    {
        if (m_hUser)
        {
            m_pClientEngine->ReleaseUser(m_hPipe, m_hUser);
        }
        m_pClientEngine->BReleaseSteamPipe(m_hPipe);
    }
    m_pClientEngine->BShutdownIfAllPipesClosed();
}

bool ClientContext::Init()
{
    if (!OpenAPI_LoadLibrary())
    {
        return false;
    }

    m_pClientEngine = (IClientEngine*)SteamInternal_CreateInterface(CLIENTENGINE_INTERFACE_VERSION);
    if (!m_pClientEngine)
    {
        return false;
    }

    m_hPipe = m_pClientEngine->CreateSteamPipe();
    if (!m_hPipe)
    {
        return false;
    }

    m_hUser = m_pClientEngine->ConnectToGlobalUser(m_hPipe);
    if (!m_hUser)
    {
        return false;
    }

    m_pClientUser = (IClientUser*)m_pClientEngine->GetIClientUser(m_hUser, m_hPipe);
    if (!m_pClientUser)
    {
        return false;
    }

    m_pClientRemoteManager = (IClientRemoteClientManager*)m_pClientEngine->GetIClientRemoteClientManager(m_hPipe);
    if (!m_pClientRemoteManager)
    {
        return false;
    }

    m_pClientFriends = (IClientFriends*)m_pClientEngine->GetIClientFriends(m_hUser, m_hPipe);
    if (!m_pClientFriends)
    {
        return false;
    }

    return true;
}

void ClientContext::RunCallbacks()
{
    Steam_RunCallbacks(m_hPipe, false);
}

IClientUser* ClientContext::ClientUser()
{
    return m_pClientUser;
}

IClientFriends* ClientContext::ClientFriends()
{
    return m_pClientFriends;
}

IClientRemoteClientManager* ClientContext::RemoteClientManager()
{
    return m_pClientRemoteManager;
}

ClientContext* GClientContext()
{
    static ClientContext ctx;
    return &ctx;
}
