#pragma once

// APP_VERSION is defined via CMake compile definitions.
// This fallback is for IDE intellisense only.
#ifndef APP_VERSION
#define APP_VERSION "2.0.9"
#endif

namespace AppConfig {
    // Application
    inline constexpr const char* AppName           = "SteamGameServerLauncher";
    inline constexpr const char* OrganizationName   = "SteamGameServerLauncher";

    // GitHub repository for self-update
    inline constexpr const char* GithubOwner        = "linlin0417";
    inline constexpr const char* GithubRepo         = "SteamGameServerLauncher";

    // Paths (relative to the launcher executable)
    inline constexpr const char* SteamCmdSubDir     = "steamcmd";
    inline constexpr const char* InstancesSubDir    = "instances";
    inline constexpr const char* ProfilesSubDir     = "profiles";
    inline constexpr const char* ConfigFileName     = "launcher_settings.json";
    inline constexpr const char* UpdaterExeName     = "Updater.exe";
    inline constexpr const char* LauncherExeName    = "SteamGameServerLauncher.exe";
}
