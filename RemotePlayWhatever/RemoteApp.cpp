#include <wx/cmdline.h>
#include "RemoteApp.h"
#include "wxSteamStuff.h"
#include "FriendsListFrame.h"

static const wxCmdLineEntryDesc cmdLineDesc[] =
{
    { wxCMD_LINE_SWITCH, "h", "help", "show this help message" },
    { wxCMD_LINE_SWITCH, "v", "verbose", "verbose output" },
    { wxCMD_LINE_OPTION, "a", "appid", "AppID for non-steam games", wxCMD_LINE_VAL_NUMBER },
    { wxCMD_LINE_OPTION, "i", "invitee", "SteamID64 invitee ( use 0 for guest link )", wxCMD_LINE_VAL_STRING },
    { wxCMD_LINE_NONE }
};

bool RemoteApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    if (!GClientContext()->Init())
    {
#ifndef _WIN32
        if(!m_oneShot)
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

    if (!GClientContext()->SteamUser()->BLoggedOn() || !GetRunningGameID().IsValid())
    {
#ifndef _WIN32
        if(!m_oneShot)
#endif
            wxMessageBox
            (
                "Could not detect game running. Start a game first!",
                "No game runnunig!",
                wxOK | wxICON_INFORMATION
            );

        std::cout << "Error: No game running!" << std::endl;

        GClientContext()->Shutdown();

        return false;
    }

    m_callbackRunner.Start(200);

    if(!m_oneShot)
    {
        m_friendsList = new FriendsListFrame(&m_inviteHandler);
        m_friendsList->Show(true);
    }
    else
    {
        m_oneShot->Send();
    }

    return true;
}

int RemoteApp::OnExit()
{
    if(m_oneShot)
    {
        delete m_oneShot;
    }

    if (m_callbackRunner.IsRunning())
    {
        m_callbackRunner.Stop();
    }

    GClientContext()->Shutdown();

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
        m_inviteHandler.SetNonSteamAppID(appID);
    }

    wxString inviteeStr64;
    if(parser.Found("i", &inviteeStr64))
    {
        uint64_t cliInvitee = std::strtoull(inviteeStr64.c_str(), NULL, 10);
        m_oneShot = new OneShotInvite(CSteamID((uint64)cliInvitee), &m_inviteHandler);
    }

    return wxApp::OnCmdLineParsed(parser);
}

void RemoteApp::OnInitCmdLine(wxCmdLineParser &parser)
{
    parser.SetDesc(cmdLineDesc);
}

void RemoteAppCallbackRunner::Notify()
{
    GClientContext()->RunCallbacks();
}

// tiny one shot invite handler
OneShotInvite::OneShotInvite(CSteamID invitee, RemotePlayInviteHandler *handler):
    m_invitee(invitee),
    m_handler(handler),
    m_remoteInviteResultCb(this, &OneShotInvite::OnRemotePlayInviteResult)
{
}

void OneShotInvite::OnRemotePlayInviteResult(RemotePlayInviteResult_t* inviteResultCb)
{
    if (inviteResultCb->m_eResult == k_ERemoteClientLaunchResultOK)
    {
        std::cout << inviteResultCb->m_szConnectURL << std::endl;
    }
    else
    {
        std::cout << "Could not create remote play session: " << inviteResultCb->m_eResult << std::endl;
    }
    wxTheApp->Exit();
}

void OneShotInvite::Send()
{
    m_handler->SendInvite(m_invitee);
}


