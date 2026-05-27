// ============================================================
// PROCESS MANAGER MODULE
// ============================================================ 
// Module manages background processes created by the shell.
// It maintains a map of background processes and provides
// commands to list, kill, stop, and resume them.
//
// Main data:
//   - std::map<DWORD, BgProcess> bgProcesses
//   - DWORD fg_pid  (PID of current foreground process)
// ============================================================

#pragma once

#include <windows.h>
#include <map>
#include <string>
#include <iostream>

// ---- Status codes ----
#define STATUS_RUNNING    0
#define STATUS_STOPPED    1
#define STATUS_TERMINATED 2

// ---- Background process info ----
struct BgProcess {
    HANDLE      hProcess;       // Handle to the process (for Kill, Wait)
    HANDLE      hThread;        // Handle to the main thread (for Stop, Resume)
    DWORD       dwProcessId;    // Process ID
    DWORD       dwThreadId;     // Thread ID
    std::string cmdName;        // Original command name
    int         status;         // STATUS_RUNNING | STATUS_STOPPED | STATUS_TERMINATED
};

// ---- Global data ----
std::map<DWORD, BgProcess> bgProcesses;   // All background processes
DWORD fg_pid = 0;                         // PID of the foreground process (0 = none)

// ============================================================
// HELPER: Get status as string
// ============================================================
const char* getStatusString(int status) {
    switch (status) {
        case STATUS_RUNNING:    return "RUNNING";
        case STATUS_STOPPED:    return "STOPPED";
        case STATUS_TERMINATED: return "TERMINATED";
        default:                return "UNKNOWN";
    }
}

// ============================================================
// addProcess - Save a new background process to the map
// ============================================================
void addProcess(const PROCESS_INFORMATION &pi, const std::string &cmdName) {
    BgProcess bp;
    bp.hProcess    = pi.hProcess;
    bp.hThread     = pi.hThread;
    bp.dwProcessId = pi.dwProcessId;
    bp.dwThreadId  = pi.dwThreadId;
    bp.cmdName     = cmdName;
    bp.status      = STATUS_RUNNING;
    bgProcesses[pi.dwProcessId] = bp;
}

// ============================================================
// listProcesses - Display all background processes with status
// Uses WaitForSingleObject(h, 0) to poll without blocking
// ============================================================
void listProcesses() {
    if (bgProcesses.empty()) {
        std::cout << "No background processes.\n";
        return;
    }

    // First pass: poll and update status of running processes
    for (auto &[pid, bp] : bgProcesses) {
        if (bp.status == STATUS_RUNNING) {
            DWORD result = WaitForSingleObject(bp.hProcess, 0);
            if (result == WAIT_OBJECT_0) {
                // Process has exited naturally
                bp.status = STATUS_TERMINATED;
                CloseHandle(bp.hProcess);
                CloseHandle(bp.hThread);
            }
        }
    }

    // Second pass: print the list
    std::cout << "----------------------------------------------\n";
    std::cout << "PID\tSTATUS\t\tCOMMAND\n";
    std::cout << "----------------------------------------------\n";
    for (const auto &[pid, bp] : bgProcesses) {
        std::cout << bp.dwProcessId << "\t"
                  << getStatusString(bp.status) << "\t\t"
                  << bp.cmdName << "\n";
    }
    std::cout << "----------------------------------------------\n";
}

// ============================================================
// killProcess - Forcefully terminate a process by PID
// API: TerminateProcess(hProcess, 0)
// ============================================================
bool killProcess(DWORD pid) {
    auto it = bgProcesses.find(pid);
    if (it == bgProcesses.end()) {
        std::cout << "[-] Error: Process " << pid << " not found.\n";
        return false;
    }

    BgProcess &bp = it->second;
    if (bp.status == STATUS_TERMINATED) {
        std::cout << "[-] Process " << pid << " already terminated.\n";
        return false;
    }

    TerminateProcess(bp.hProcess, 0);
    WaitForSingleObject(bp.hProcess, 500);  // Wait briefly for cleanup
    CloseHandle(bp.hProcess);
    CloseHandle(bp.hThread);
    bp.status = STATUS_TERMINATED;

    std::cout << "[+] Process " << pid << " (" << bp.cmdName << ") killed.\n";
    return true;
}

// ============================================================
// stopProcess - Suspend a running process by PID
// API: SuspendThread(hThread)
// ============================================================
bool stopProcess(DWORD pid) {
    auto it = bgProcesses.find(pid);
    if (it == bgProcesses.end()) {
        std::cout << "[-] Error: Process " << pid << " not found.\n";
        return false;
    }

    BgProcess &bp = it->second;
    if (bp.status != STATUS_RUNNING) {
        std::cout << "[-] Process " << pid << " is not running (status: "
                  << getStatusString(bp.status) << ").\n";
        return false;
    }

    SuspendThread(bp.hThread);
    bp.status = STATUS_STOPPED;

    std::cout << "[+] Process " << pid << " (" << bp.cmdName << ") stopped.\n";
    return true;
}

// ============================================================
// resumeProcess - Resume a suspended process by PID
// API: ResumeThread(hThread)
// ============================================================
bool resumeProcess(DWORD pid) {
    auto it = bgProcesses.find(pid);
    if (it == bgProcesses.end()) {
        std::cout << "[-] Error: Process " << pid << " not found.\n";
        return false;
    }

    BgProcess &bp = it->second;
    if (bp.status != STATUS_STOPPED) {
        std::cout << "[-] Process " << pid << " is not stopped (status: "
                  << getStatusString(bp.status) << ").\n";
        return false;
    }

    ResumeThread(bp.hThread);
    bp.status = STATUS_RUNNING;

    std::cout << "[+] Process " << pid << " (" << bp.cmdName << ") resumed.\n";
    return true;
}

// ============================================================
// cleanupAllProcesses - Terminate and close all remaining
// processes. Called when the shell exits.
// ============================================================
void cleanupAllProcesses() {
    for (auto &[pid, bp] : bgProcesses) {
        if (bp.status != STATUS_TERMINATED) {
            TerminateProcess(bp.hProcess, 0);
            WaitForSingleObject(bp.hProcess, 300);
            CloseHandle(bp.hProcess);
            CloseHandle(bp.hThread);
            bp.status = STATUS_TERMINATED;
        }
    }
    bgProcesses.clear();
}
