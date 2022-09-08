#ifndef QUEUEINVITER_H
#define QUEUEINVITER_H
#include <queue>
#include <wx/wx.h>
#include "SteamStuff.h"
#include "RemotePlayInviteHandler.h"

class QueueInviter
{
public:
    QueueInviter(RemotePlayInviteHandler* handler, std::queue<uint64>* que);
    ~QueueInviter();

    void SendInvites();
    bool Running();

private:
    bool m_running;

    std::queue<uint64>* m_inviteQueue;
    RemotePlayInviteHandler* m_handler;

    STEAM_CALLBACK(QueueInviter, OnRemotePlayInviteResult, RemotePlayInviteResult_t, m_remoteInviteResultCb);
};

#endif // QUEUEINVITER_H
