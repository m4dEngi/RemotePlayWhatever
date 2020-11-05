#ifndef CLIENTTASKS_H
#define CLIENTTASKS_H

#include <queue>
#include <Steamworks.h>
#include "IClientRemoteClientManager.h"

class ClientTaskBase
{
public:
    ClientTaskBase() {}
    virtual ~ClientTaskBase() {}

    virtual void Start() {}
    virtual void Update() {}

    bool BFinished() { return m_finished; }
    bool BStarted() { return m_started; }

protected:
    bool m_finished = false;
    bool m_started = false;
};

class ClientTaskRunner
{
public:
    ClientTaskRunner() {}
    ~ClientTaskRunner();

    void QueueTask(ClientTaskBase* newTask);
    void RunTasks();

private:

    std::queue<ClientTaskBase*> m_activeTasks;
};

ClientTaskRunner* GClientTaskRunner();

class RemotePlayInviteTask : public ClientTaskBase
{
public:
    RemotePlayInviteTask(CSteamID invitee);
    ~RemotePlayInviteTask() {}

    void Start() override;
    void Update() override;

private:
    CSteamID m_invitee;
    AppId_t m_runningAppId;

    STEAM_CALLBACK(RemotePlayInviteTask, OnRemotePlayStart, RemoteClientStartStreamSession_t, m_remoteStartCb);
};


#endif // CLIENTTASKS_H