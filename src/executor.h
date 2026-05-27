// ============================================================
// EXECUTOR MODULE
// ============================================================
// Module responsible for creating child processes to execute
// external commands using the Windows CreateProcess() API.
//
// Handles:
//   - Foreground execution (WaitForSingleObject)
//   - Background execution (save to process list)
//   - .bat file detection (prepend "cmd.exe /c")
// ============================================================

#pragma once

#include "parser.h"
#include "process_manager.h"
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>

// ============================================================
// HELPER: Check if a command ends with ".bat" (case-insensitive)
// ============================================================
bool isBatFile(const std::string &cmd) {
    if (cmd.size() < 4) return false;
    std::string ext = cmd.substr(cmd.size() - 4);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".bat");
}

// ============================================================
// execute_command - Main entry point for external commands
// ============================================================
void execute_command(const ParsedCommand &cmd) {
    if (cmd.command.empty()) return;

    // ---- Step 1: Build the command line string from args ----
    // (args already has '&' removed by the parser)
    std::string cmdLine;
    for (size_t i = 0; i < cmd.args.size(); ++i) {
        if (i > 0) cmdLine += " ";
        cmdLine += cmd.args[i];
    }

    // ---- Step 2: Handle .bat files ----
    // .bat files are scripts, not executables.
    // They must be run via: cmd.exe /c script.bat [args]
    if (isBatFile(cmd.command)) {
        cmdLine = "cmd.exe /c " + cmdLine;
    }

    // ---- Step 3: Prepare structures for CreateProcess ----
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // CreateProcess() requires a mutable char buffer for lpCommandLine
    char cmdLineBuf[1024];
    strncpy(cmdLineBuf, cmdLine.c_str(), sizeof(cmdLineBuf) - 1);
    cmdLineBuf[sizeof(cmdLineBuf) - 1] = '\0';

    // ---- Step 4: Create the child process ----
    BOOL success = CreateProcessA(
        NULL,           // lpApplicationName: NULL = let Windows resolve
        cmdLineBuf,     // lpCommandLine: full command line string
        NULL,           // lpProcessAttributes: default security
        NULL,           // lpThreadAttributes: default security
        FALSE,          // bInheritHandles: don't inherit parent handles
        0,              // dwCreationFlags: normal creation
        NULL,           // lpEnvironment: use parent's environment
        NULL,           // lpCurrentDirectory: use parent's working dir
        &si,            // lpStartupInfo: startup configuration
        &pi             // lpProcessInformation: [OUT] PID, handles
    );

    if (!success) {
        std::cerr << "[-] Error: '" << cmd.command 
                  << "' is not recognized as an internal or external command.\n";
        return;
    }

    // ---- Step 5: Foreground vs Background ----
    if (cmd.isBackground) {
        // === BACKGROUND MODE ===
        // Do NOT wait. Save process info for later management.
        addProcess(pi, cmd.command);
        std::cout << "[" << pi.dwProcessId << "] " << cmd.command << "\n";
        // Do NOT close handles here --- we need them for kill/stop/resume
    } else {
        // === FOREGROUND MODE ===
        // Block the shell until the child process terminates.
        fg_pid = pi.dwProcessId;
        WaitForSingleObject(pi.hProcess, INFINITE);
        fg_pid = 0;

        // Child is done --- clean up handles immediately.
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}
