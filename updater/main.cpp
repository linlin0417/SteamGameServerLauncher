/// Updater.exe — Independent pure C++ update program for SteamGameServerLauncher.
///
/// Usage:
///   Updater.exe --zip <path> --target <dir> --exe <launcher.exe> --pid <pid>
///
/// 1. Wait for the launcher process (--pid) to exit.
/// 2. Extract the zip using miniz.
/// 3. Re-launch the launcher executable.
/// 4. Clean up and exit.

#include <windows.h>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include "miniz.h"

namespace fs = std::filesystem;

static std::ofstream g_logFile;

static void log(const std::string& msg)
{
    if (g_logFile.is_open()) {
        g_logFile << msg << std::endl;
    }
    OutputDebugStringA((msg + "\n").c_str());
}

static std::string utf16_to_utf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static bool waitForProcessExit(DWORD pid, DWORD timeoutMs = 30000)
{
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (hProcess == NULL) {
        return true;
    }
    DWORD result = WaitForSingleObject(hProcess, timeoutMs);
    CloseHandle(hProcess);
    return result == WAIT_OBJECT_0;
}

static bool extractZipMiniz(const std::string& zipPath, const std::string& targetDir)
{
    mz_zip_archive zip_archive;
    memset(&zip_archive, 0, sizeof(zip_archive));
    if (!mz_zip_reader_init_file(&zip_archive, zipPath.c_str(), 0)) {
        log("ERROR: Failed to open zip file.");
        return false;
    }

    int numFiles = (int)mz_zip_reader_get_num_files(&zip_archive);
    for (int i = 0; i < numFiles; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat)) continue;

        fs::path destPath = fs::path(targetDir) / file_stat.m_filename;

        if (mz_zip_reader_is_file_a_directory(&zip_archive, i)) {
            fs::create_directories(destPath);
        } else {
            fs::create_directories(destPath.parent_path());
            if (!mz_zip_reader_extract_to_file(&zip_archive, i, destPath.string().c_str(), 0)) {
                log("ERROR: Failed to extract file: " + std::string(file_stat.m_filename));
            }
        }
    }
    mz_zip_reader_end(&zip_archive);
    return true;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    int argc;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argvW) return 1;

    std::string zipPath, targetDir, exePath;
    DWORD pid = 0;

    for (int i = 1; i < argc; ++i) {
        std::wstring warg = argvW[i];
        std::string arg = utf16_to_utf8(warg);
        if (arg == "--zip" && i + 1 < argc) {
            zipPath = utf16_to_utf8(argvW[++i]);
        } else if (arg == "--target" && i + 1 < argc) {
            targetDir = utf16_to_utf8(argvW[++i]);
        } else if (arg == "--exe" && i + 1 < argc) {
            exePath = utf16_to_utf8(argvW[++i]);
        } else if (arg == "--pid" && i + 1 < argc) {
            pid = std::stoul(utf16_to_utf8(argvW[++i]));
        }
    }
    LocalFree(argvW);

    if (targetDir.empty()) {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        targetDir = fs::path(buffer).parent_path().string();
    }

    g_logFile.open((fs::path(targetDir) / "updater.log").string(), std::ios::app);

    if (zipPath.empty() || exePath.empty()) {
        log("ERROR: Missing required arguments.");
        return 1;
    }

    log("=== SteamGameServerLauncher Updater ===");
    log("  Zip:    " + zipPath);
    log("  Target: " + targetDir);
    log("  Exe:    " + exePath);
    log("  PID:    " + std::to_string(pid));

    if (pid > 0) {
        log("Waiting for launcher to exit...");
        if (!waitForProcessExit(pid)) {
            log("WARNING: Timed out waiting for launcher. Proceeding anyway.");
        }
        Sleep(1000);
    } else {
        Sleep(2000);
    }

    // Backup exe
    fs::path originalExe(exePath);
    fs::path backupExe = originalExe.string() + ".bak";
    if (fs::exists(originalExe)) {
        try {
            fs::copy_file(originalExe, backupExe, fs::copy_options::overwrite_existing);
            log("Backed up executable to: " + backupExe.string());
        } catch (const std::exception& e) {
            log(std::string("WARNING: Failed to backup executable: ") + e.what());
        }
    }

    log("Extracting update using miniz...");
    if (!extractZipMiniz(zipPath, targetDir)) {
        log("ERROR: Extraction failed.");
        // Rollback
        if (fs::exists(backupExe)) {
            try {
                fs::copy_file(backupExe, originalExe, fs::copy_options::overwrite_existing);
                log("Rolled back executable.");
            } catch (...) {
                log("ERROR: Rollback failed.");
            }
        }
        return 2;
    }
    log("Extraction complete.");

    // Clean up zip
    std::error_code ec;
    if (fs::remove(zipPath, ec)) {
        log("Cleaned up update package.");
    }

    // Restart launcher
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
