#include <wx/clipbrd.h>
#include "SteamStuff.h"

// steam client context

ClientContext::ClientContext():
    m_hPipe(0),
    m_hUser(0),
    m_pSteamClient(nullptr),
    m_pSteamUser(nullptr),
    m_pSteamFriends(nullptr),
    m_pSteamUtils(nullptr),
    m_pClientEngine(nullptr),
    m_ShuttingDown(false),
    m_Initialized(false),
    m_initError(InitOK)
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

    // Until something more specific is known, failing means Steam isn't there.
    m_initError = InitSteamNotRunning;

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
        m_initError = InitNotSignedIn;
        return false;
    }

    m_initError = InitMethodsMissing;

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

    IClientRemoteClientManager* pRemoteManager =
        (IClientRemoteClientManager*)m_pClientEngine->GetIClientRemoteClientManager(m_hPipe);
    if (!pRemoteManager)
    {
        return false;
    }

    // IClientRemoteClientManager is the interface that actually drifts between
    // builds (Valve keeps adding Remote Play methods), so instead of calling it
    // through the header's fixed vtable order we resolve each method by name and
    // call through the resolved index. Confirm the methods we use exist on this
    // build before anything calls through the proxy.
    m_remoteManager.Bind(pRemoteManager);
    m_remoteManager.DebugDump(); // no-op unless RPW_VTDEBUG is set
    if (!m_remoteManager.Ok()
        || !m_remoteManager.Has("BCreateRemotePlayInviteAndSession")
        || !m_remoteManager.Has("ShowRemotePlayTogetherUI"))
    {
        clientvt::Log("[vt] Init: RCM method resolution FAILED "
                      "(ok=%d count=%zu) — refusing to call into wrong slots\n",
                      (int)m_remoteManager.Ok(), m_remoteManager.Count());
        return false;
    }

    m_initError = InitOK;
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

CRemoteClientManagerByName* ClientContext::RemoteClientManager()
{
    return &m_remoteManager;
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

std::string ClickableURL(const char* url)
{
    std::string link = url ? url : "";
    if (!link.empty() && link.find("://") == std::string::npos)
    {
        link = "https://" + link;
    }
    return link;
}


