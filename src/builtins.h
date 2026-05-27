// ============================================================
// BUILTINS MODULE
// ============================================================
// Built-in commands are executed directly inside the shell
// process, NOT in a child process.
//
// Reason: commands like 'exit' and 'addpath' must modify the
// shell's own state. A child process cannot do that.
//
// Supported: help, exit, cd, dir, date, time, path, addpath,
//            list, kill, stop, resume
// ============================================================

#pragma once

#include "set_color.h"
#include "process_manager.h"
#include <string>
#include <vector>
#include <windows.h>
#include <iostream>

// ============================================================
// help - Print usage guide
// ============================================================
void help() {
    std::cout << CYAN << BOLD
              << "============================================\n"
              << "           myShell - Command Help            \n"
              << "============================================\n" << RESET;

    std::cout << YELLOW << " General Commands:\n" << RESET;
    std::cout << "   help                 Show this help message\n";
    std::cout << "   exit                 Exit the shell\n";
    std::cout << "   cd <path>            Change current directory\n";
    std::cout << "   dir [path]           List files in directory\n";
    std::cout << "   date                 Show current date\n";
    std::cout << "   time                 Show current time\n";

    std::cout << YELLOW << "\n Path Management:\n" << RESET;
    std::cout << "   path                 Show custom search paths\n";
    std::cout << "   addpath <path>       Add a directory to search paths\n";

    std::cout << YELLOW << "\n Process Management:\n" << RESET;
    std::cout << "   list                 List all background processes\n";
    std::cout << "   kill <pid>           Kill a background process\n";
    std::cout << "   stop <pid>           Suspend a background process\n";
    std::cout << "   resume <pid>         Resume a suspended process\n";

    std::cout << YELLOW << "\n Usage Tips:\n" << RESET;
    std::cout << "   <command> &          Run command in background\n";
    std::cout << "   Ctrl+C               Stop foreground process\n";

    std::cout << CYAN
              << "============================================\n" << RESET;
}

// ============================================================
// path - Display custom search paths
// ============================================================
std::vector<std::string> paths;

void path() {
    if (paths.empty()) {
        std::cout << "No custom paths set. Use 'addpath <path>' to add.\n";
    } else {
        std::cout << "Custom search paths:\n";
        for (size_t i = 0; i < paths.size(); ++i) {
            std::cout << "  " << i + 1 << ": " << paths[i] << "\n";
        }
    }
}

// ============================================================
// addpath - Add a directory to custom search paths
// ============================================================
void addpath(const std::string &new_path) {
    paths.push_back(new_path);
    std::cout << "[+] Path added: " << new_path << "\n";
}

// ============================================================
// dir - List files in a directory
// ============================================================
void dir(std::string dirPath = "") {
    if (dirPath.empty()) {
        char buf[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buf);
        dirPath = buf;
    }

    WIN32_FIND_DATAA data;
    HANDLE hFind = FindFirstFileA((dirPath + "\\*").c_str(), &data);
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "[-] Error: Path not found: " << dirPath << "\n";
        return;
    }

    std::cout << " Directory of " << dirPath << "\n\n";
    int fileCount = 0, dirCount = 0;
    do {
        std::string name = data.cFileName;
        if (name == "." || name == "..") continue;

        if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            std::cout << "  <DIR>  " << name << "\n";
            dirCount++;
        } else {
            std::cout << "         " << name << "\n";
            fileCount++;
        }
    } while (FindNextFileA(hFind, &data));
    FindClose(hFind);

    std::cout << "\n  " << fileCount << " file(s), " << dirCount << " dir(s)\n";
}

// ============================================================
// get_time - Display current time
// ============================================================
void get_time() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("Time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
}

// ============================================================
// get_date - Display current date
// ============================================================
void get_date() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("Date: %02d/%02d/%04d\n", st.wDay, st.wMonth, st.wYear);
}

// ============================================================
// cd - Change current directory
// ============================================================
void cd(const std::string &path) {
    if (SetCurrentDirectoryA(path.c_str())) {
        char buf[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buf);
        std::cout << buf << "\n";
    } else {
        std::cout << "[-] Error: Cannot find path: " << path << "\n";
    }
}

// ============================================================
// handle_builtin - Dispatch built-in commands
// Returns true if the command was a built-in, false otherwise.
//
// Note: args[0] = command name, args[1..] = actual arguments
// ============================================================
bool handle_builtin(const std::string &cmd, size_t argc, const std::vector<std::string> &args) {

    // ---- General commands ----
    if (cmd == "help") {
        help();
        return true;
    }
    if (cmd == "exit") {
        std::cout << "Goodbye!\n";
        cleanupAllProcesses();
        exit(0);
    }
    if (cmd == "cd") {
        if (argc < 2) {
            // No argument: print current directory
            char buf[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, buf);
            std::cout << buf << "\n";
        } else {
            cd(args[1]);
        }
        return true;
    }
    if (cmd == "dir") {
        if (argc < 2) dir();       // No argument: current directory
        else           dir(args[1]);
        return true;
    }
    if (cmd == "date") {
        get_date();
        return true;
    }
    if (cmd == "time") {
        get_time();
        return true;
    }

    // ---- Path management ----
    if (cmd == "path") {
        path();
        return true;
    }
    if (cmd == "addpath") {
        if (argc < 2) {
            std::cout << "Usage: addpath <directory>\n";
        } else {
            for (size_t i = 1; i < argc; ++i) {
                addpath(args[i]);
            }
        }
        return true;
    }

    // ---- Process management ----
    if (cmd == "list") {
        listProcesses();
        return true;
    }
    if (cmd == "kill") {
        if (argc < 2) {
            std::cout << "Usage: kill <pid>\n";
        } else {
            DWORD pid = std::stoul(args[1]);
            killProcess(pid);
        }
        return true;
    }
    if (cmd == "stop") {
        if (argc < 2) {
            std::cout << "Usage: stop <pid>\n";
        } else {
            DWORD pid = std::stoul(args[1]);
            stopProcess(pid);
        }
        return true;
    }
    if (cmd == "resume") {
        if (argc < 2) {
            std::cout << "Usage: resume <pid>\n";
        } else {
            DWORD pid = std::stoul(args[1]);
            resumeProcess(pid);
        }
        return true;
    }

    return false;  // Not a built-in command
}
