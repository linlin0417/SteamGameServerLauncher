#pragma once

// APP_VERSION is defined via CMake compile definitions.
// This fallback is for IDE intellisense only.
#ifndef APP_VERSION
#define APP_VERSION "1.0.0"
#endif

namespace AppConfig {
    // Application
    inline constexpr const char* AppName           = "SteamGameServerLauncher";
    inline constexpr const char* OrganizationName   = "SteamGameServerLauncher";

    // GitHub repository for self-update
    inline constexpr const char* GithubOwner        = "linlin0417"; // TODO: Update this
    inline constexpr const char* GithubRepo         = "SteamGameServerLauncher";

    // Icarus Dedicated Server defaults
    inline constexpr const char* IcarusAppId        = "2089300";
    inline constexpr const char* DefaultServerName  = "My Icarus Server";
    inline constexpr int         DefaultPort        = 17777;
    inline constexpr int         DefaultQueryPort   = 27015;
    inline constexpr int         DefaultMaxPlayers  = 8;

    // Paths (relative to the launcher executable)
    inline constexpr const char* SteamCmdSubDir     = "steamcmd";
    inline constexpr const char* ServersSubDir      = "servers";
    inline constexpr const char* ConfigFileName     = "launcher_settings.json";
    inline constexpr const char* UpdaterExeName     = "Updater.exe";
    inline constexpr const char* LauncherExeName    = "SteamGameServerLauncher.exe";
}
