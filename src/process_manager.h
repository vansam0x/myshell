// ============================================================
// PROCESS MANAGER MODULE
// ============================================================
// Manages background processes: add, list, kill, stop, resume.
//
// Uses a simple vector of BackgroundProcess structs.
// Each struct stores the PID, process handle, thread handle,
// command name, and current status.
//
// Windows APIs used:
//   - WaitForSingleObject(h, 0)  -> poll if process is alive
//   - TerminateProcess(h, 0)     -> forcefully kill
//   - SuspendThread(hThread)     -> pause execution
//   - ResumeThread(hThread)      -> resume execution
//   - CloseHandle()              -> release kernel handles
// ============================================================

#pragma once

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include "set_color.h"

// ============================================================
// Background process status codes
// ============================================================
enum ProcessStatus {
    PROC_RUNNING    = 0,
    PROC_STOPPED    = 1,
    PROC_TERMINATED = 2
};

// ============================================================
// BackgroundProcess - stores info about one background process
// ============================================================
struct BackgroundProcess {
    DWORD       pid;        // Process ID
    HANDLE      hProcess;   // Handle to the process (for Kill, Wait)
    HANDLE      hThread;    // Handle to the main thread (for Stop, Resume)
    std::string cmdName;    // Name of the command that was executed
    int         status;     // PROC_RUNNING, PROC_STOPPED, PROC_TERMINATED
};

// ============================================================
// Global list of background processes
// ============================================================
std::vector<BackgroundProcess> bgProcesses;

// ============================================================
// Global flag for CTRL+C handling
// ============================================================
// When true, the shell is waiting for a foreground process.
// The CtrlHandler uses this to decide behavior.
volatile BOOL isRunningForeground = FALSE;
volatile HANDLE hForegroundProcess = NULL;
volatile BOOL stopBatchExecution = FALSE;

// ============================================================
// Helper: convert status code to string
// ============================================================
const char* getStatusString(int status) {
    switch (status) {
        case PROC_RUNNING:    return "RUNNING";
        case PROC_STOPPED:    return "STOPPED";
        case PROC_TERMINATED: return "TERMINATED";
        default:              return "UNKNOWN";
    }
}

// ============================================================
// refreshProcessStatus
// ============================================================
// Poll each background process to check if it has terminated
// naturally. If so, update its status and close handles.
// This is called before listing or when the shell needs
// up-to-date information.
// ============================================================
void refreshProcessStatus() {
    for (auto &proc : bgProcesses) {
        if (proc.status == PROC_TERMINATED) continue;

        // Poll with zero timeout: returns immediately
        DWORD result = WaitForSingleObject(proc.hProcess, 0);
        if (result == WAIT_OBJECT_0) {
            // Process has terminated naturally
            proc.status = PROC_TERMINATED;
            CloseHandle(proc.hProcess);
            CloseHandle(proc.hThread);
            proc.hProcess = NULL;
            proc.hThread  = NULL;
        }
    }
}

// ============================================================
// addProcess - Register a new background process
// ============================================================
void addProcess(PROCESS_INFORMATION &pi, const std::string &cmdName) {
    BackgroundProcess bp;
    bp.pid      = pi.dwProcessId;
    bp.hProcess = pi.hProcess;
    bp.hThread  = pi.hThread;
    bp.cmdName  = cmdName;
    bp.status   = PROC_RUNNING;
    bgProcesses.push_back(bp);
}

// ============================================================
// listProcesses - Print all background processes
// ============================================================
// Refreshes status first, then prints a formatted table.
// ============================================================
void listProcesses() {
    refreshProcessStatus();

    // Check if there are any processes to show
    bool hasAny = false;
    for (const auto &proc : bgProcesses) {
        if (proc.status != PROC_TERMINATED) {
            hasAny = true;
            break;
        }
    }

    if (!hasAny) {
        std::cout << "No background processes.\n";
        return;
    }

    // Print header
    std::cout << CYAN << BOLD
              << std::left
              << std::setw(10) << "PID"
              << std::setw(30) << "COMMAND"
              << std::setw(15) << "STATUS"
              << RESET << "\n";
    std::cout << std::string(55, '-') << "\n";

    // Print each process
    for (const auto &proc : bgProcesses) {
        if (proc.status == PROC_TERMINATED) continue;

        // Color based on status
        const char* color = (proc.status == PROC_RUNNING) ? GREEN :
                            (proc.status == PROC_STOPPED) ? YELLOW : RED;

        std::cout << std::left
                  << std::setw(10) << proc.pid
                  << std::setw(30) << proc.cmdName
                  << color << std::setw(15) << getStatusString(proc.status)
                  << RESET << "\n";
    }
}

// ============================================================
// findProcess - Find a background process by PID
// ============================================================
// Returns a pointer to the BackgroundProcess, or nullptr.
// ============================================================
BackgroundProcess* findProcess(DWORD pid) {
    for (auto &proc : bgProcesses) {
        if (proc.pid == pid && proc.status != PROC_TERMINATED) {
            return &proc;
        }
    }
    return nullptr;
}

// ============================================================
// killProcess - Forcefully terminate a background process
// ============================================================
void killProcess(DWORD pid) {
    BackgroundProcess* proc = findProcess(pid);
    if (!proc) {
        std::cout << RED << "[-] Process " << pid << " not found.\n" << RESET;
        return;
    }

    if (TerminateProcess(proc->hProcess, 0)) {
        // Wait briefly for the process to actually terminate
        WaitForSingleObject(proc->hProcess, 1000);
        proc->status = PROC_TERMINATED;
        CloseHandle(proc->hProcess);
        CloseHandle(proc->hThread);
        proc->hProcess = NULL;
        proc->hThread  = NULL;
        std::cout << GREEN << "[+] Process " << pid << " killed.\n" << RESET;
    } else {
        std::cout << RED << "[-] Failed to kill process " << pid
                  << ". Error: " << GetLastError() << "\n" << RESET;
    }
}

// ============================================================
// stopProcess - Suspend a background process
// ============================================================
void stopProcess(DWORD pid) {
    BackgroundProcess* proc = findProcess(pid);
    if (!proc) {
        std::cout << RED << "[-] Process " << pid << " not found.\n" << RESET;
        return;
    }

    if (proc->status == PROC_STOPPED) {
        std::cout << YELLOW << "[!] Process " << pid << " is already stopped.\n" << RESET;
        return;
    }

    // Try suspending the entire process using NtSuspendProcess
    bool suspended = false;
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll) {
        typedef LONG(NTAPI* pfnNtSuspendProcess)(HANDLE ProcessHandle);
        pfnNtSuspendProcess NtSuspendProcess = (pfnNtSuspendProcess)GetProcAddress(hNtDll, "NtSuspendProcess");
        if (NtSuspendProcess) {
            LONG status = NtSuspendProcess(proc->hProcess);
            if (status >= 0) { // NT_SUCCESS
                suspended = true;
            }
        }
    }

    // Fallback: suspend main thread
    if (!suspended) {
        DWORD result = SuspendThread(proc->hThread);
        if (result != (DWORD)-1) {
            suspended = true;
        }
    }

    if (suspended) {
        proc->status = PROC_STOPPED;
        std::cout << GREEN << "[+] Process " << pid << " stopped.\n" << RESET;
    } else {
        std::cout << RED << "[-] Failed to stop process " << pid
                  << ". Error: " << GetLastError() << "\n" << RESET;
    }
}

// ============================================================
// resumeProcess - Resume a suspended background process
// ============================================================
void resumeProcess(DWORD pid) {
    BackgroundProcess* proc = findProcess(pid);
    if (!proc) {
        std::cout << RED << "[-] Process " << pid << " not found.\n" << RESET;
        return;
    }

    if (proc->status != PROC_STOPPED) {
        std::cout << YELLOW << "[!] Process " << pid << " is not stopped.\n" << RESET;
        return;
    }

    // Try resuming the entire process using NtResumeProcess
    bool resumed = false;
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll) {
        typedef LONG(NTAPI* pfnNtResumeProcess)(HANDLE ProcessHandle);
        pfnNtResumeProcess NtResumeProcess = (pfnNtResumeProcess)GetProcAddress(hNtDll, "NtResumeProcess");
        if (NtResumeProcess) {
            LONG status = NtResumeProcess(proc->hProcess);
            if (status >= 0) { // NT_SUCCESS
                resumed = true;
            }
        }
    }

    // Fallback: resume main thread
    if (!resumed) {
        DWORD result = ResumeThread(proc->hThread);
        if (result != (DWORD)-1) {
            resumed = true;
        }
    }

    if (resumed) {
        proc->status = PROC_RUNNING;
        std::cout << GREEN << "[+] Process " << pid << " resumed.\n" << RESET;
    } else {
        std::cout << RED << "[-] Failed to resume process " << pid
                  << ". Error: " << GetLastError() << "\n" << RESET;
    }
}

// ============================================================
// cleanupAllProcesses - Terminate and close all bg processes
// ============================================================
// Called when the shell exits (via 'exit' command or EOF).
// ============================================================
void cleanupAllProcesses() {
    for (auto &proc : bgProcesses) {
        if (proc.status != PROC_TERMINATED) {
            TerminateProcess(proc.hProcess, 0);
            WaitForSingleObject(proc.hProcess, 1000);
            CloseHandle(proc.hProcess);
            CloseHandle(proc.hThread);
            proc.status   = PROC_TERMINATED;
            proc.hProcess = NULL;
            proc.hThread  = NULL;
        }
    }
    bgProcesses.clear();
}
