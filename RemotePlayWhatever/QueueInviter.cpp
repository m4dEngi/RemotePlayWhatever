#include "QueueInviter.h"

QueueInviter::QueueInviter(RemotePlayInviteHandler* handler, std::queue<uint64>* que):
    m_handler(handler),
    m_running(false),
    m_inviteQueue(que),
    m_remoteInviteResultCb(this, &QueueInviter::OnRemotePlayInviteResult)
{
}

QueueInviter::~QueueInviter()
{
}

void QueueInviter::SendInvites()
{
    if(m_running || m_inviteQueue->empty())
    {
       return;
    }
    m_running = true;

    m_handler->SendInvite(m_inviteQueue->front());
}

bool QueueInviter::Running()
{
    return m_running;
}

void QueueInviter::OnRemotePlayInviteResult(RemotePlayInviteResult_t* inviteResultCb)
{
    if( m_inviteQueue->empty() ||
        inviteResultCb->m_player.m_playerID != m_inviteQueue->front()
    )
    {
        return;
    }

    if(inviteResultCb->m_eResult == k_ERemoteClientLaunchResultOK)
    {
        std::cout << " - guestid    : " << inviteResultCb->m_player.m_guestID <<std::endl;
        std::cout << " - connectURL : " << inviteResultCb->m_szConnectURL <<std::endl;
        std::cout << std::endl;
    }


    m_inviteQueue->pop();

    if(m_inviteQueue->empty())
    {
        m_running = false;
        return;
    }

    m_handler->SendInvite(m_inviteQueue->front());
}
