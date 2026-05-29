// ============================================================
// EXECUTOR MODULE
// ============================================================
// Handles execution of external commands via CreateProcess().
//
// Responsibilities:
//   1. Build the command line string
//   2. Search custom paths if the command is not found
//   3. Search custom paths if the command is not found
//   4. Foreground mode: WaitForSingleObject + CloseHandle
//   5. Background mode: addProcess to the process list
//
// Windows APIs used:
//   - CreateProcessA()        -> spawn child process
//   - WaitForSingleObject()   -> block until child exits (fg)
//   - CloseHandle()           -> release handles (fg)
//   - GetFileAttributesA()    -> check if file exists
// ============================================================

#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "parser.h"
#include "process_manager.h"
#include "set_color.h"

// Forward-declare the paths vector from builtins.h
extern std::vector<std::string> paths;

// ============================================================
// Helper: try to find the executable in custom paths
// ============================================================
// If the command doesn't contain a path separator, try each
// custom path directory to find a matching executable.
// Returns the full path if found, or empty string if not found.
// ============================================================
static std::string resolveFromCustomPaths(const std::string &command) {
    // If the command already has a path separator, don't search
    if (command.find('\\') != std::string::npos ||
        command.find('/') != std::string::npos) {
        return "";
    }

    // Extensions to try when searching
    const char* extensions[] = { "", ".exe", ".com", ".bat", ".cmd" };

    for (const auto &dir : paths) {
        for (const auto &ext : extensions) {
            std::string fullPath = dir;
            // Ensure trailing backslash
            if (!fullPath.empty() && fullPath.back() != '\\' && fullPath.back() != '/') {
                fullPath += '\\';
            }
            fullPath += command + ext;

            DWORD attrs = GetFileAttributesA(fullPath.c_str());
            if (attrs != INVALID_FILE_ATTRIBUTES &&
                !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
                return fullPath;
            }
        }
    }
    return "";
}

// ============================================================
// buildCommandLine - construct the full command line string
// ============================================================
// Tries custom paths if direct execution might fail.
// ============================================================
static std::string buildCommandLine(const ParsedCommand &cmd) {
    // If the first argument is an executable, use it directly (e.g. background batch files running via myShell.exe)
    if (!cmd.args.empty() && cmd.args[0].size() >= 4) {
        std::string firstArg = cmd.args[0];
        std::transform(firstArg.begin(), firstArg.end(), firstArg.begin(), ::tolower);
        if (firstArg.substr(firstArg.size() - 4) == ".exe") {
            std::string commandLine = "\"" + cmd.args[0] + "\"";
            for (size_t i = 1; i < cmd.args.size(); ++i) {
                commandLine += " " + cmd.args[i];
            }
            return commandLine;
        }
    }

    std::string commandLine;

    // Check if we should try custom paths
    std::string resolvedPath = resolveFromCustomPaths(cmd.command);

    // Build the base command line
    if (!resolvedPath.empty()) {
        // Use the resolved full path as the command
        commandLine = "\"" + resolvedPath + "\"";
        // Append remaining arguments
        for (size_t i = 1; i < cmd.args.size(); ++i) {
            commandLine += " " + cmd.args[i];
        }
    } else {
        // Use the original full command line (minus the '&' if it was bg)
        commandLine = cmd.command;
        for (size_t i = 1; i < cmd.args.size(); ++i) {
            commandLine += " " + cmd.args[i];
        }
    }

    return commandLine;
}

// ============================================================
// execute_command - Main entry point for external commands
// ============================================================
// Called from main.cpp when the command is NOT a built-in.
// Creates a child process in either foreground or background mode.
// ============================================================
void execute_command(const ParsedCommand &cmd) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Build the command line string
    std::string commandLine = buildCommandLine(cmd);

    // CreateProcess needs a mutable char array for lpCommandLine
    char cmdLine[1024];
    strncpy(cmdLine, commandLine.c_str(), sizeof(cmdLine) - 1);
    cmdLine[sizeof(cmdLine) - 1] = '\0';

    // Attempt to create the process
    // For background processes, use CREATE_NEW_PROCESS_GROUP and DETACHED_PROCESS
    // so that:
    //   1. Ctrl+C signals are NOT forwarded to them from the parent console.
    //   2. They are detached from the parent console session, preventing the console
    //      subsystem from locking up when the background process is suspended (stopped).
    DWORD creationFlags = cmd.isBackground ? (CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS) : 0;

    HANDLE hNul = INVALID_HANDLE_VALUE;
    BOOL inheritHandles = FALSE;

    if (cmd.isBackground) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;

        hNul = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (hNul != INVALID_HANDLE_VALUE) {
            si.dwFlags |= STARTF_USESTDHANDLES;
            si.hStdInput = hNul;
            si.hStdOutput = hNul;
            si.hStdError = hNul;
            inheritHandles = TRUE;
        }
    } else {
        // Foreground command: inherit the parent's standard handles
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
        inheritHandles = TRUE;
    }

    BOOL success = CreateProcessA(
        NULL,           // lpApplicationName: NULL -> use cmdLine
        cmdLine,        // lpCommandLine: full command string
        NULL,           // lpProcessAttributes
        NULL,           // lpThreadAttributes
        inheritHandles, // bInheritHandles
        creationFlags,  // dwCreationFlags
        NULL,           // lpEnvironment: inherit parent's
        NULL,           // lpCurrentDirectory: inherit parent's
        &si,            // lpStartupInfo
        &pi             // lpProcessInformation [OUT]
    );

    if (hNul != INVALID_HANDLE_VALUE) {
        CloseHandle(hNul);
    }

    if (!success) {
        std::cout << RED << "[-] Error: Cannot execute '" << cmd.command
                  << "'. Error code: " << GetLastError() << "\n" << RESET;
        return;
    }

    if (cmd.isBackground) {
        // ---- BACKGROUND MODE ----
        // Don't wait. Save process info for later management.
        addProcess(pi, cmd.command);
        std::cout << GREEN << "[+] Background process started: "
                  << cmd.command << " [PID: " << pi.dwProcessId << "]\n"
                  << RESET;
        // Do NOT close handles --- we need them for kill/stop/resume.

    } else {
        // ---- FOREGROUND MODE ----
        // Set the global flag so CtrlHandler knows we're waiting
        hForegroundProcess = pi.hProcess;
        isRunningForeground = TRUE;

        // Block until the child process terminates
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Clear the flag
        isRunningForeground = FALSE;
        hForegroundProcess = NULL;

        // Child is done --- clean up handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}
