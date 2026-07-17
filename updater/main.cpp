/// Updater.exe — Independent pure C++ update program for SteamGameServerLauncher.
///
/// Usage:
///   Updater.exe --zip <path> --target <dir> --exe <launcher.exe> --pid <pid>
///
/// 1. Wait for the launcher process (--pid) to exit.
/// 2. Extract the zip using built-in Windows 10 tar.exe.
/// 3. Re-launch the launcher executable.
/// 4. Clean up and exit.

#include <windows.h>
#include <string>
#include <iostream>
#include <vector>

static void log(const std::string& msg)
{
    std::cout << msg << "\n";
}

/// Wait for a process to exit.
static bool waitForProcessExit(DWORD pid, DWORD timeoutMs = 30000)
{
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (hProcess == NULL) {
        return true; // Process likely already exited
    }
    DWORD result = WaitForSingleObject(hProcess, timeoutMs);
    CloseHandle(hProcess);
    return result == WAIT_OBJECT_0;
}

/// Run a command and wait for it to finish.
static bool runCommand(const std::string& cmdLine)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // CreateProcess requires a mutable string buffer
    std::vector<char> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back('\0');

    if (!CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return false;
    }

    WaitForSingleObject(pi.hProcess, 120000); // 2 minute timeout
    
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return exitCode == 0;
}

/// Extract zip using Windows 10 built-in tar
static bool extractZip(const std::string& zipPath, const std::string& targetDir)
{
    // tar -xf "zipPath" -C "targetDir"
    std::string cmd = "tar -xf \"" + zipPath + "\" -C \"" + targetDir + "\"";
    return runCommand(cmd);
}

int main(int argc, char *argv[])
{
    std::string zipPath;
    std::string targetDir;
    std::string exePath;
    DWORD pid = 0;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--zip" && i + 1 < argc) {
            zipPath = argv[++i];
        } else if (arg == "--target" && i + 1 < argc) {
            targetDir = argv[++i];
        } else if (arg == "--exe" && i + 1 < argc) {
            exePath = argv[++i];
        } else if (arg == "--pid" && i + 1 < argc) {
            pid = std::stoul(argv[++i]);
        }
    }

    if (zipPath.empty() || targetDir.empty() || exePath.empty()) {
        log("ERROR: Missing required arguments.");
        return 1;
    }

    log("=== SteamGameServerLauncher Updater ===");
    log("  Zip:    " + zipPath);
    log("  Target: " + targetDir);
    log("  Exe:    " + exePath);
    log("  PID:    " + std::to_string(pid));

    // Step 1: Wait for launcher to exit
    if (pid > 0) {
        log("Waiting for launcher to exit...");
        if (!waitForProcessExit(pid)) {
            log("WARNING: Timed out waiting for launcher. Proceeding anyway.");
        }
        Sleep(1000); // Extra safety delay
    } else {
        Sleep(2000);
    }

    // Step 2: Extract update
    log("Extracting update...");
    if (!extractZip(zipPath, targetDir)) {
        log("ERROR: Failed to extract update package using tar.exe.");
        return 2;
    }
    log("Extraction complete.");

    // Step 3: Clean up zip
    DeleteFileA(zipPath.c_str());
    log("Cleaned up update package.");

    // Step 4: Restart launcher
    log("Restarting: " + exePath);
    std::string launchCmd = "\"" + exePath + "\"";
    
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::vector<char> cmdBuffer(launchCmd.begin(), launchCmd.end());
    cmdBuffer.push_back('\0');

    if (!CreateProcessA(NULL, cmdBuffer.data(), NULL, NULL, FALSE, 0, NULL, targetDir.c_str(), &si, &pi)) {
        log("ERROR: Failed to restart launcher.");
        return 3;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    log("Update complete. Exiting updater.");
    return 0;
}
