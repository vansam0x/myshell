#pragma once

#include <windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include "set_color.h"

enum ProcessStatus {
    PROC_RUNNING    = 0,
    PROC_STOPPED    = 1,
    PROC_TERMINATED = 2
};
struct BackgroundProcess {
    DWORD       pid;        
    HANDLE      hProcess;   
    HANDLE      hThread;    
    std::string cmdName;    
    int         status;     
};
std::vector<BackgroundProcess> bgProcesses;
volatile BOOL isRunningForeground = FALSE;
volatile HANDLE hForegroundProcess = NULL;
volatile BOOL stopBatchExecution = FALSE;

const char* getStatusString(int status) {
    switch (status) {
        case PROC_RUNNING:    return "RUNNING";
        case PROC_STOPPED:    return "STOPPED";
        case PROC_TERMINATED: return "TERMINATED";
        default:              return "UNKNOWN";
    }
}

void refreshProcessStatus() {
    for (auto &proc : bgProcesses) {
        if (proc.status == PROC_TERMINATED) continue;
        DWORD result = WaitForSingleObject(proc.hProcess, 0);
        if (result == WAIT_OBJECT_0) {
            proc.status = PROC_TERMINATED;
            CloseHandle(proc.hProcess);
            CloseHandle(proc.hThread);
            proc.hProcess = NULL;
            proc.hThread  = NULL;
        }
    }
}

void addProcess(PROCESS_INFORMATION &pi, const std::string &cmdName) {
    BackgroundProcess bp;
    bp.pid      = pi.dwProcessId;
    bp.hProcess = pi.hProcess;
    bp.hThread  = pi.hThread;
    bp.cmdName  = cmdName;
    bp.status   = PROC_RUNNING;
    bgProcesses.push_back(bp);
}

void listProcesses() {
    refreshProcessStatus();
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
    std::cout << CYAN << BOLD
              << std::left
              << std::setw(10) << "PID"
              << std::setw(30) << "COMMAND"
              << std::setw(15) << "STATUS"
              << RESET << "\n";
    std::cout << std::string(55, '-') << "\n";
    for (const auto &proc : bgProcesses) {
        if (proc.status == PROC_TERMINATED) continue;
        const char* color = (proc.status == PROC_RUNNING) ? GREEN :
                            (proc.status == PROC_STOPPED) ? YELLOW : RED;

        std::cout << std::left
                  << std::setw(10) << proc.pid
                  << std::setw(30) << proc.cmdName
                  << color << std::setw(15) << getStatusString(proc.status)
                  << RESET << "\n";
    }
}

BackgroundProcess* findProcess(DWORD pid) {
    for (auto &proc : bgProcesses) {
        if (proc.pid == pid && proc.status != PROC_TERMINATED) {
            return &proc;
        }
    }
    return nullptr;
}

void killProcess(DWORD pid) {
    BackgroundProcess* proc = findProcess(pid);
    if (!proc) {
        std::cout << RED << "[-] Process " << pid << " not found.\n" << RESET;
        return;
    }

    if (TerminateProcess(proc->hProcess, 0)) {
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
