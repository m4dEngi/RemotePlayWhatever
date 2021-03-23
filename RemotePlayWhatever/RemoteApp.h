#ifndef REMOTEAPP_H
#define REMOTEAPP_H

#include <wx/wx.h>
#include <wx/taskbar.h>
#include <map>
#include "SteamStuff.h"
#include "RemotePlayInviteHandler.h"

enum
{
    TRAY_EXIT,
    TRAY_FRIEND,
    TRAY_COPYLINK
};

class wxSteamID : public wxObject
{
public:
    wxSteamID(CSteamID steamID);
    CSteamID GetSteamID();

private:
    CSteamID m_steamID;
};

class RemoteAppTaskBarIcon : public wxTaskBarIcon
{
public:
    RemoteAppTaskBarIcon() {}

    void OnMenuExit(wxCommandEvent&);
    void OnMenuSteamFriend(wxCommandEvent&);
    void OnMenuCopyRemotePlayLink(wxCommandEvent&);

    virtual wxMenu* CreatePopupMenu();

private:
    wxMenu* BuildFriendsMenu();

    RemotePlayInviteHandler m_remoteInvite;
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
    virtual int OnExit();

protected:
    RemoteAppTaskBarIcon* m_taskBarIcon;
    RemoteAppCallbackRunner m_callbackRunner;
};

#endif // !REMOTEAPP_H
