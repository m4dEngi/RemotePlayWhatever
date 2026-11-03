#ifndef REMOTEAPP_H
#define REMOTEAPP_H
#include <queue>
#include <wx/wx.h>
#include "SteamStuff.h"
#include "RemotePlayInviteHandler.h"
#include "QueueInviter.h"

class OneShotInvite
{
public:
    OneShotInvite(CSteamID invitee, RemotePlayInviteHandler* handler);
    void Send();

private:
    CSteamID m_invitee;
    RemotePlayInviteHandler* m_handler;

    STEAM_CALLBACK(OneShotInvite, OnRemotePlayInviteResult, RemotePlayInviteResult_t, m_remoteInviteResultCb);
};

class ControlFrame;
class FriendsListFrame;
class wxSingleInstanceChecker;

class RemoteAppCallbackRunner : public wxTimer
{
public:
    virtual void Notify() override;
};

class RemoteApp : public wxApp
{
public:
    RemoteApp();

    virtual bool OnInit() override;
    virtual int  OnRun() override;
    virtual int  OnExit() override;
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser) override;
    virtual void OnInitCmdLine(wxCmdLineParser& parser) override;

    RemotePlayInviteHandler* GetInviteHandler() const { return m_inviteHandler; }

private:
    bool m_oneshot;


    FriendsListFrame* m_friendsList;
    ControlFrame* m_controlFrame;
    wxSingleInstanceChecker* m_singleInstance;
    RemoteAppCallbackRunner m_callbackRunner;

    std::queue<uint64> m_inviteQue;
    AppId_t m_nonSteamID;
    uint64 m_guestID;
    uint64 m_inviteToCancel;

    RemotePlayInviteHandler* m_inviteHandler;
};

#endif // !REMOTEAPP_H
