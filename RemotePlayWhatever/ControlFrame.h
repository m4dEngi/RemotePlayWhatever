#ifndef CONTROLFRAME_H
#define CONTROLFRAME_H
#include <wx/wx.h>
#include "RemotePlayInviteHandler.h"

// A window that appears when guests connect, showing one row per guest
// (avatar + name + keyboard/mouse/controller toggle buttons). Clicking a
// toggle grants or revokes that peripheral for that specific guest. Driven
// entirely by the handler's roster; rebuilds itself whenever it changes.
class ControlFrame : public wxFrame
{
public:
    ControlFrame(RemotePlayInviteHandler* handler);

private:
    void Rebuild();
    void OnPoll(wxTimerEvent& evt);
    void AddToggle(wxWindow* parent, wxSizer* sizer, const wxString& label,
                   bool on, uint64 guestKey, int which);
    // Name shown for a guest; sets placeholder when Steam doesn't know it yet.
    wxString GuestName(const ConnectedGuest& g, bool& placeholder);

    RemotePlayInviteHandler* m_handler;
    wxScrolledWindow* m_container;
    wxBoxSizer* m_sizer;
    wxTimer m_timer;
    uint32 m_lastRevision;
    bool m_waitingForNames;
};

#endif // CONTROLFRAME_H
