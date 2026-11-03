#ifndef FRIENDSLISTFRAME_H
#define FRIENDSLISTFRAME_H
#include <map>
#include <wx/wx.h>
#include "FriendPanel.h"
#include "RemotePlayInviteHandler.h"

class wxSearchCtrl;
class RpwTrayIcon;

enum ERPWEvtID
{
    rpwID_INVITE_GUEST,
    rpwID_INVITE_FRIEND,
};

class FriendsListFrame : public wxFrame
{
public:
    FriendsListFrame(RemotePlayInviteHandler* handler);

    // Tray icon support: minimizing hides the window to the tray.
    void SetTray(RpwTrayIcon* tray);
    void ShowFromTray();
    void CopyGuestLink();
    void ConfirmEndSession();

#ifdef __WXMSW__
    // A second copy of the app asks this one to show itself with a registered
    // window message (it may be hidden in the tray).
    virtual WXLRESULT MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) override;
#endif

private:
    void AddFreiendPanel(CSteamID friendID);
    void RemoveFriendPanel(CSteamID friendID);

    bool ShowNonSteamWarning();
    bool CanSendInvite();

    RemotePlayInviteHandler* m_pRemoteInvite;
    wxScrolledWindow* m_friendsListContainer;
    wxBoxSizer* m_friendsListSizer;

    std::map<uint64, FriendPanel*> m_friendPanels;

    void OnFriendPanelClick(wxCommandEvent& event);
    void OnGuestPanelClick(wxCommandEvent& event);
    void OnTick(wxTimerEvent& event);
    void OnClose(wxCloseEvent& event);
    void OnIconize(wxIconizeEvent& event);
    void OnEndSession(wxCommandEvent& event);
    void OnCopyLink(wxCommandEvent& event);
    void UpdateStatus();
    void ApplyFilter();

    RpwTrayIcon* m_tray;
    wxSearchCtrl* m_search;
    wxStaticText* m_status;
    wxButton* m_copyLinkButton;
    wxButton* m_endButton;
    wxString m_statusShown;
    wxString m_notice;
    wxLongLong m_noticeUntil;
    wxLongLong m_copiedUntil;
    wxTimer m_tick;
    bool m_askingGroupFallback;

    STEAM_CALLBACK(FriendsListFrame, OnRemotePlayInviteResult, RemotePlayInviteResult_t, m_remoteInviteResultCb);
    STEAM_CALLBACK(FriendsListFrame, OnPersonaStateChange, PersonaStateChange_t, m_personaStateCb);
};

#endif // FRIENDSLISTFRAME_H
