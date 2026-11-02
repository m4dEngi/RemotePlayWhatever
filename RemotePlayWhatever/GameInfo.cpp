#include <map>
#include <regex>
#include <string>
#include <vector>
#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/utils.h>
#ifdef __WXMSW__
#include <wx/msw/registry.h>
#endif
#include "GameInfo.h"

static std::string ReadTextFile(const wxString& path)
{
    if (!wxFileExists(path))
    {
        return std::string();
    }

    wxFFile file(path, "rb");
    wxString text;
    if (!file.IsOpened() || !file.ReadAll(&text, wxConvUTF8))
    {
        return std::string();
    }
    return std::string(text.utf8_str());
}

static std::vector<wxString> SteamRoots()
{
    std::vector<wxString> roots;
#ifdef __WXMSW__
    wxRegKey key(wxRegKey::HKCU, "Software\\Valve\\Steam");
    wxString path;
    if (key.Exists() && key.QueryValue("SteamPath", path) && !path.empty())
    {
        roots.push_back(path);
    }
#else
    wxString home = wxGetHomeDir();
    roots.push_back(home + "/.steam/steam");
    roots.push_back(home + "/.local/share/Steam");
    roots.push_back(home + "/.var/app/com.valvesoftware.Steam/.local/share/Steam");
#endif
    return roots;
}

// Every Steam install plus the extra library folders listed in its
// steamapps/libraryfolders.vdf.
static std::vector<wxString> LibraryFolders()
{
    static const std::regex pathRx("\"path\"\\s+\"([^\"]+)\"");

    std::vector<wxString> folders;
    for (const wxString& root : SteamRoots())
    {
        if (!wxDirExists(root))
        {
            continue;
        }
        folders.push_back(root);

        std::string vdf = ReadTextFile(root + "/steamapps/libraryfolders.vdf");
        for (std::sregex_iterator it(vdf.begin(), vdf.end(), pathRx), end; it != end; ++it)
        {
            // VDF escapes backslashes in Windows paths.
            std::string escaped = (*it)[1].str();
            std::string path;
            for (size_t i = 0; i < escaped.size(); ++i)
            {
                if (escaped[i] == '\\' && i + 1 < escaped.size() && escaped[i + 1] == '\\')
                {
                    ++i;
                }
                path += escaped[i];
            }
            folders.push_back(wxString::FromUTF8(path.c_str()));
        }
    }
    return folders;
}

wxString GetSteamAppName(AppId_t appID)
{
    static std::map<AppId_t, wxString> cache;
    auto cached = cache.find(appID);
    if (cached != cache.end())
    {
        return cached->second;
    }

    static const std::regex nameRx("\"name\"\\s+\"([^\"]*)\"");

    wxString name;
    for (const wxString& folder : LibraryFolders())
    {
        std::string acf = ReadTextFile(wxString::Format("%s/steamapps/appmanifest_%u.acf", folder, (unsigned)appID));
        std::smatch match;
        if (std::regex_search(acf, match, nameRx) && match[1].length() > 0)
        {
            name = wxString::FromUTF8(match[1].str().c_str());
            break;
        }
    }

    if (name.empty())
    {
        name = wxString::Format("App %u", (unsigned)appID);
    }
    cache[appID] = name;
    return name;
}
