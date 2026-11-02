#include <wx/cmdline.h>
#include <wx/snglinst.h>
#include <wx/utils.h>
#include "RemoteApp.h"
#include "wxSteamStuff.h"
#include "FriendsListFrame.h"
#include "ControlFrame.h"

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_SWITCH, "h", "help",    "show this help message" },
    { wxCMD_LINE_SWITCH, "v", "verbose", "verbose output" },
    { wxCMD_LINE_OPTION, "a", "appid",   "AppID for non-steam games", wxCMD_LINE_VAL_NUMBER },
    { wxCMD_LINE_OPTION, "i", "invite",  "Send remote play invites to provided SteamID64s ( use 0 for guest link )", wxCMD_LINE_VAL_STRING },
    { wxCMD_LINE_OPTION, "g", "guestid", "Guest client id", wxCMD_LINE_VAL_NUMBER },
    { wxCMD_LINE_OPTION, "c", "cancel",  "Cancel invite for provided SteamID64 and GuestID", wxCMD_LINE_VAL_STRING },
    { wxCMD_LINE_NONE }
};

RemoteApp::RemoteApp():
    m_inviteHandler(nullptr),
    m_controlFrame(nullptr),
    m_singleInstance(nullptr),
    m_oneshot(false),
    m_guestID(1),
    m_nonSteamID(0),
    m_inviteToCancel(-1)
{
}

bool RemoteApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    // Only one window at a time; a second launch brings the open one to the
    // front. Command-line invites (one-shot mode) can still run alongside it.
    if (!m_oneshot)
    {
        m_singleInstance = new wxSingleInstanceChecker();
        if (m_singleInstance->Create("RemotePlayWhatever-" + wxGetUserId()) &&
            m_singleInstance->IsAnotherRunning())
        {
            bool activated = false;
#ifdef __WXMSW__
            HWND other = ::FindWindowW(nullptr, L"Remote Play Whatever");
            if (other)
            {
                ::ShowWindow(other, SW_RESTORE);
                activated = ::SetForegroundWindow(other) != FALSE;
            }
#endif
            if (!activated)
            {
                wxMessageBox("Remote Play Whatever is already open.", "Remote Play Whatever", wxOK | wxICON_INFORMATION);
            }
            delete m_singleInstance;
            m_singleInstance = nullptr;
            return false;
        }
    }

    if (!GClientContext()->Init())
    {
#ifndef _WIN32
        if(!m_oneshot)
#endif
            wxMessageBox
            (
                "Could not initialize steam client library!",
                "Error",
                wxOK | wxICON_ERROR
            );

        std::cout << "Error: Could not initialize steam client library!" << std::endl;

        return false;
    }

    return true;
}

int RemoteApp::OnRun()
{
    m_inviteHandler = new RemotePlayInviteHandler();
    m_inviteHandler->SetGuestID(m_guestID);
    m_inviteHandler->SetNonSteamAppID(m_nonSteamID);

    if(!m_oneshot)
    {
        m_callbackRunner.Start(200);

        m_friendsList = new FriendsListFrame(m_inviteHandler);
        m_friendsList->Show(true);

        // Hidden until a guest connects, then it pops itself up.
        m_controlFrame = new ControlFrame(m_inviteHandler);

        return wxApp::OnRun();
    }

    if(!m_inviteQue.empty() &&
        GetRunningGameID().IsValid() &&
        GClientContext()->SteamUser()->BLoggedOn()
    )
    {
        // No one to ask on the command line: if Steam won't create a group,
        // send anyway like stock RPW did.
        m_inviteHandler->SetAutoFallback(true);
        QueueInviter qinviter(m_inviteHandler, &m_inviteQue);
        qinviter.SendInvites();
        while(qinviter.Running())
        {
            // look at me... i'm the main event loop now
            GClientContext()->RunCallbacks();
            m_inviteHandler->GroupTick();   // fire the async (group-gated) invite

            wxMilliSleep(200);
        }
    }

    if(m_inviteToCancel != -1)
    {
        m_inviteHandler->CancelInvite(m_inviteToCancel, m_guestID);
    }

    return 0;
}

int RemoteApp::OnExit()
{
    if (m_callbackRunner.IsRunning())
    {
        m_callbackRunner.Stop();
    }

    if(m_inviteHandler)
    {
        delete m_inviteHandler;
    }

    GClientContext()->Shutdown();

    delete m_singleInstance;
    m_singleInstance = nullptr;

    return 0;
}

bool RemoteApp::OnCmdLineParsed(wxCmdLineParser &parser)
{
    if(parser.FoundSwitch("h"))
    {
        parser.Usage();

        wxTheApp->Exit();
    }

    long appID;
    if(parser.Found("a", &appID))
    {
        m_nonSteamID = appID;
    }

    long guestID;
    if(parser.Found("g", &guestID))
    {
        m_guestID = guestID;
    }

    wxString inviteeStr64;
    if(parser.Found("i", &inviteeStr64))
    {
        m_oneshot = true;
        size_t first = 0, last = 0;
        while(last != wxString::npos)
        {
            last = inviteeStr64.find(",", first);
            uint64 invitee = 0;
            if(inviteeStr64.substr(first, last - first).ToULongLong(&invitee))
            {
                m_inviteQue.push(invitee);
            }
            first = last + 1;
        }
    }
    else if (parser.Found("c", &inviteeStr64))
    {
        m_oneshot = true;
        inviteeStr64.ToULongLong(&m_inviteToCancel);
    }

    return wxApp::OnCmdLineParsed(parser);
}

void RemoteApp::OnInitCmdLine(wxCmdLineParser &parser)
{
    parser.SetDesc(cmdLineDesc);
}

// --

void RemoteAppCallbackRunner::Notify()
{
    GClientContext()->RunCallbacks();

    RemoteApp* app = static_cast<RemoteApp*>(wxTheApp);
    if (app && app->GetInviteHandler())
    {
        // Advance async group creation / flush queued invites every tick.
        app->GetInviteHandler()->GroupTick();

        // Re-assert guest control roughly every 2s (10 ticks @ 200ms) so Steam
        // can't silently reset the per-user input we granted.
        static int s_tick = 0;
        if (++s_tick >= 10)
        {
            s_tick = 0;
            app->GetInviteHandler()->GrantControlTick();
        }
    }
}


