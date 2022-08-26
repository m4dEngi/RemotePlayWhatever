#ifndef REMOTEAPP_H
#define REMOTEAPP_H

#include <wx/wx.h>
#include <wx/taskbar.h>
#include <map>
#include "SteamStuff.h"
#include "RemotePlayInviteHandler.h"

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

class RemoteAppCallbackRunner : public wxTimer
{
public:
    virtual void Notify() override;
};

class RemoteApp : public wxApp
{
public:
    virtual bool OnInit();
    virtual int  OnExit();
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
    virtual void OnInitCmdLine(wxCmdLineParser& parser);

protected:
    wxFrame* m_friendsList;
    RemoteAppCallbackRunner m_callbackRunner;
    RemotePlayInviteHandler m_inviteHandler;
};

#endif // !REMOTEAPP_H
