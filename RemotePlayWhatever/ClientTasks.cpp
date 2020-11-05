#include "ClientTasks.h"
#include "ClientContext.h"

ClientTaskRunner::~ClientTaskRunner()
{
    while (!m_activeTasks.empty())
    {
        delete m_activeTasks.front();
        m_activeTasks.pop();
    }
}

void ClientTaskRunner::QueueTask(ClientTaskBase* newTask)
{
    m_activeTasks.push(newTask);
}

void ClientTaskRunner::RunTasks()
{
    if (m_activeTasks.empty())
    {
        return;
    }

    if (!m_activeTasks.front()->BFinished())
    {
        if (!m_activeTasks.front()->BStarted())
        {
            m_activeTasks.front()->Start();
        }
        else
        {
            m_activeTasks.front()->Update();
        }
    }
    else
    {
        delete m_activeTasks.front();
        m_activeTasks.pop();
    }
}

ClientTaskRunner* GClientTaskRunner()
{
    static ClientTaskRunner runner;
    return &runner;
}

RemotePlayInviteTask::RemotePlayInviteTask(CSteamID invitee):
    m_remoteStartCb(this, &RemotePlayInviteTask::OnRemotePlayStart),
    m_invitee(invitee),
    m_runningAppId(0)
{
}

void RemotePlayInviteTask::Start()
{
    m_started = true;

    if (GClientContext()->ClientUser()->BIsAnyGameRunning())
    {
        CGameID gameID = GClientContext()->ClientUser()->GetRunningGameID(0);
        if (gameID.IsSteamApp())
        {
            GClientContext()->RemoteClientManager()->CreateRemotePlayInviteAndSession(m_invitee, gameID.AppID());
            GClientContext()->RemoteClientManager()->SetStreamingDriversInstalled(true);

            m_runningAppId = gameID.AppID();
        }
        else
        {
            GClientContext()->RemoteClientManager()->SetStreamingDesktopToRemotePlayTogetherEnabled(true);

            GClientContext()->RemoteClientManager()->CreateRemotePlayInviteAndSession(m_invitee, 480);
            GClientContext()->RemoteClientManager()->SetStreamingDriversInstalled(true);
        }
    }
}

void RemotePlayInviteTask::Update()
{
}

void RemotePlayInviteTask::OnRemotePlayStart(RemoteClientStartStreamSession_t* startCB)
{
    if (m_runningAppId == 0)
    {
        char buf[512] = { '\0' };
        std::string authKey;
        char hexBuf[3] = { '\0' };
        for (int i = 0; i < startCB->m_cubAuthToken; ++i)
        {
            sprintf(hexBuf, "%02x", startCB->m_rgubAuthToken[i]);
            authKey += hexBuf;
        }

        sprintf(buf, 
            "Please open this link in your default browser to join: steam://remoteplay/connect/%llu?appid=%d&auth=%s&transport=k_EStreamTransportSDR&relay=%s&restricted_countries=CN,XC", 
            GClientContext()->ClientUser()->GetSteamID().ConvertToUint64(), 
            480, 
            authKey.c_str(), 
            startCB->m_szRelayServer
        );

        GClientContext()->ClientFriends()->SendMsgToFriend(m_invitee, k_EChatEntryTypeChatMsg, buf);
    }

    m_finished = true;
}

