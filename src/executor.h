#pragma once

#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include "parser.h"
#include "process_manager.h"
#include "set_color.h"

extern std::vector<std::string> paths;

static std::string resolveFromCustomPaths(const std::string &command) {
    if (command.find('\\') != std::string::npos ||
        command.find('/') != std::string::npos) {
        return "";
    }

    const char* extensions[] = { ".exe", ".bat", ".cmd", ".com", "" };

    for (const auto &dir : paths) {
        for (const auto &ext : extensions) {
            std::string fullPath = dir;
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
static bool isExeFile(const std::string &command, const std::string &resolvedPath) {
    std::string pathToCheck = resolvedPath.empty() ? command : resolvedPath;
    std::string lower = pathToCheck;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.size() >= 2 && lower.front() == '"' && lower.back() == '"') {
        lower = lower.substr(1, lower.size() - 2);
    }
    
    size_t len = lower.size();
    if (len >= 4 && lower.substr(len - 4) == ".exe") {
        return true;
    }
    size_t dotPos = lower.find_last_of('.');
    size_t slashPos = lower.find_last_of("\\/");
    if (dotPos == std::string::npos || (slashPos != std::string::npos && dotPos < slashPos)) {
        char buffer[MAX_PATH];
        LPSTR filePart;
        DWORD searchResult = SearchPathA(NULL, pathToCheck.c_str(), ".exe", MAX_PATH, buffer, &filePart);
        if (searchResult > 0) {
            std::string resolvedSearch = buffer;
            std::transform(resolvedSearch.begin(), resolvedSearch.end(), resolvedSearch.begin(), ::tolower);
            if (resolvedSearch.size() >= 4 && resolvedSearch.substr(resolvedSearch.size() - 4) == ".exe") {
                return true;
            }
        }
    }
    
    return false;
}

static bool isBatCmdFile(const std::string &filePath) {
    std::string lower = filePath;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower.size() >= 2 && lower.front() == '"' && lower.back() == '"') {
        lower = lower.substr(1, lower.size() - 2);
    }
    
    size_t len = lower.size();
    if (len >= 4) {
        std::string ext = lower.substr(len - 4);
        if (ext == ".bat" || ext == ".cmd") {
            return true;
        }
    }
    return false;
}

static std::string buildCommandLine(const ParsedCommand &cmd) {
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
    std::string resolvedPath = resolveFromCustomPaths(cmd.command);
    if (!resolvedPath.empty()) {
        if (isBatCmdFile(resolvedPath)) {
            commandLine = "cmd.exe /c \"" + resolvedPath + "\"";
        } else {
            commandLine = "\"" + resolvedPath + "\"";
        }
        for (size_t i = 1; i < cmd.args.size(); ++i) {
            commandLine += " " + cmd.args[i];
        }
    } else {
        commandLine = cmd.command;
        for (size_t i = 1; i < cmd.args.size(); ++i) {
            commandLine += " " + cmd.args[i];
        }
    }

    return commandLine;
}
void execute_command(const ParsedCommand &cmd) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    std::string commandLine = buildCommandLine(cmd);
    char cmdLine[1024];
    strncpy(cmdLine, commandLine.c_str(), sizeof(cmdLine) - 1);
    cmdLine[sizeof(cmdLine) - 1] = '\0';
    std::string resolvedPath = resolveFromCustomPaths(cmd.command);
    bool isExe = isExeFile(cmd.command, resolvedPath);
    DWORD creationFlags = 0;
    if (isExe) {
        creationFlags = CREATE_NEW_CONSOLE;
        if (cmd.isBackground) {
            creationFlags |= CREATE_NEW_PROCESS_GROUP;
        }
    } else {
        creationFlags = cmd.isBackground ? (CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS) : 0;
    }

    HANDLE hNul = INVALID_HANDLE_VALUE;
    BOOL inheritHandles = FALSE;

    if (isExe) {
        inheritHandles = FALSE;
    } else {
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
            si.dwFlags |= STARTF_USESTDHANDLES;
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
            inheritHandles = TRUE;
        }
    }

    BOOL success = CreateProcessA(
        NULL,           
        cmdLine,        
        NULL,           
        NULL,           
        inheritHandles, 
        creationFlags,  
        NULL,           
        NULL,           
        &si,            
        &pi             
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
        addProcess(pi, cmd.command);
        std::cout << GREEN << "[+] Background process started: "
                  << cmd.command << " [PID: " << pi.dwProcessId << "]\n"
                  << RESET;
    } else {
        hForegroundProcess = pi.hProcess;
        isRunningForeground = TRUE;
        WaitForSingleObject(pi.hProcess, INFINITE);
        isRunningForeground = FALSE;
        hForegroundProcess = NULL;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}
