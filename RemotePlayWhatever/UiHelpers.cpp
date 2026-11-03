#include <wx/defs.h>
// Windows headers come first: Steamworks.h includes windows.h itself, and
// wx's copy must come before it so the winsock headers don't clash.
#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#include <mmsystem.h>
#endif
#include <wx/bitmap.h>
#include <wx/config.h>
#include <wx/icon.h>
#include <wx/image.h>
#include <wx/utils.h>
#include "UiHelpers.h"

#ifndef __WXMSW__
#include "appicon.xpm"
#endif

wxIconBundle AppIcons()
{
#ifdef __WXMSW__
    return wxIconBundle("ID_APPICON", (WXHINSTANCE)::GetModuleHandleW(NULL));
#else
    wxIconBundle icons;
    const int sizes[] = { 16, 24, 32, 48, 64 };
    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i)
    {
        wxImage image(appicon_xpm);
        image.Rescale(sizes[i], sizes[i], wxIMAGE_QUALITY_HIGH);
        wxIcon icon;
        icon.CopyFromBitmap(wxBitmap(image));
        icons.AddIcon(icon);
    }
    return icons;
#endif
}

bool SoundsEnabled()
{
    wxConfigBase* config = wxConfigBase::Get();
    return !config || config->ReadBool("/Options/Sounds", true);
}

void SetSoundsEnabled(bool enabled)
{
    wxConfigBase* config = wxConfigBase::Get();
    if (config)
    {
        config->Write("/Options/Sounds", enabled);
        config->Flush();
    }
}

void PlayGuestSound(bool joined)
{
    if (!SoundsEnabled())
    {
        return;
    }
#ifdef __WXMSW__
    // The same sounds Windows plays when a device is plugged in or removed.
    ::PlaySoundW(joined ? L"DeviceConnect" : L"DeviceDisconnect", NULL, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
#else
    if (joined)
    {
        wxBell();
    }
#endif
}

static wxString GuestKeyPath(uint64 steamID)
{
    return wxString::Format("/Guests/%llu", (unsigned long long)steamID);
}

bool LoadGuestControls(uint64 steamID, bool& kbd, bool& mouse, bool& ctrl)
{
    wxConfigBase* config = wxConfigBase::Get();
    wxString path = GuestKeyPath(steamID);
    if (!steamID || !config || !config->HasGroup(path))
    {
        return false;
    }
    kbd = config->ReadBool(path + "/Keyboard", true);
    mouse = config->ReadBool(path + "/Mouse", true);
    ctrl = config->ReadBool(path + "/Controller", true);
    return true;
}

void SaveGuestControls(uint64 steamID, bool kbd, bool mouse, bool ctrl)
{
    wxConfigBase* config = wxConfigBase::Get();
    if (!steamID || !config)
    {
        return;
    }
    wxString path = GuestKeyPath(steamID);
    config->Write(path + "/Keyboard", kbd);
    config->Write(path + "/Mouse", mouse);
    config->Write(path + "/Controller", ctrl);
    config->Flush();
}
