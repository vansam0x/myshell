#pragma once

#include "set_color.h"
#include "process_manager.h"
#include <string>
#include <vector>
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cctype>

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
    std::cout << "   delpath <path>   Remove a directory from search paths\n";

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

void addpath(const std::string &new_path) {
    std::string trimmed = new_path;
    size_t start = trimmed.find_first_not_of(" \t");
    size_t end = trimmed.find_last_not_of(" \t");
    if (start != std::string::npos) {
        trimmed = trimmed.substr(start, end - start + 1);
    }

    if (trimmed.empty()) {
        std::cout << "[-] Error: Path cannot be empty\n";
        return;
    }
    for (const auto &p : paths) {
        std::string p_lower = p;
        std::string trimmed_lower = trimmed;
        std::transform(p_lower.begin(), p_lower.end(), p_lower.begin(), ::tolower);
        std::transform(trimmed_lower.begin(), trimmed_lower.end(), trimmed_lower.begin(), ::tolower);
        if (p_lower == trimmed_lower) {
            std::cout << "[-] Error: Path already exists in search paths\n";
            return;
        }
    }
    DWORD attrs = GetFileAttributesA(trimmed.c_str());
    if (attrs == INVALID_FILE_ATTRIBUTES || !(attrs & FILE_ATTRIBUTE_DIRECTORY)) {
        std::cout << "[-] Error: Directory does not exist or is not accessible: " << trimmed << "\n";
        return;
    }
    paths.push_back(trimmed);
    std::cout << "[+] Path added: " << trimmed << "\n";
}

void delete_path(const std::string &target) {
    if (target.empty()) {
        std::cout << "Usage: delpath <index | path_string>\n";
        return;
    }
    bool isIndex = true;
    for (char c : target) {
        if (!isdigit(c)) {
            isIndex = false;
            break;
        }
    }

    if (isIndex && !target.empty()) {
        int idx = std::stoi(target) - 1;
        if (idx >= 0 && idx < static_cast<int>(paths.size())) {
            std::string removed = paths[idx];
            paths.erase(paths.begin() + idx);
            std::cout << "[+] Path removed: " << removed << "\n";
            return;
        } else {
            std::cout << "[-] Error: Invalid path index: " << target << "\n";
            return;
        }
    }
    auto it = std::find(paths.begin(), paths.end(), target);
    if (it != paths.end()) {
        paths.erase(it);
        std::cout << "[+] Path removed: " << target << "\n";
    } else {
        std::cout << "[-] Error: Path not found in search paths: " << target << "\n";
    }
}

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
void get_time() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    // printf("Time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
    std::cout << "Time : " << std::setfill('0') << std::setw(2) << st.wHour << ":"
              << std::setfill('0') << std::setw(2) << st.wMinute << ":"
              << std::setfill('0') << std::setw(2) << st.wSecond << "\n";
}

void get_date() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    // printf("Date: %02d/%02d/%04d\n", st.wDay, st.wMonth, st.wYear);
    std::cout << "Date : " << std::setfill('0') << std::setw(2) << st.wDay << "/"
              << std::setfill('0') << std::setw(2) << st.wMonth << "/"
              << st.wYear << "\n";
}

void cd(const std::string &path) {
    if (SetCurrentDirectoryA(path.c_str())) {
        char buf[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, buf);
        std::cout << buf << "\n";
    } else {
        std::cout << "[-] Error: Cannot find path: " << path << "\n";
    }
}

bool handle_builtin(const std::string &cmd, size_t argc, const std::vector<std::string> &args) {
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
            char buf[MAX_PATH];
            GetCurrentDirectoryA(MAX_PATH, buf);
            std::cout << buf << "\n";
        } else {
            cd(args[1]);
        }
        return true;
    }
    if (cmd == "dir") {
        if (argc < 2) dir();
        else dir(args[1]);
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
    if (cmd == "echo") {
        if (argc < 2) {
            std::cout << "\n";
        } else {
            std::string arg = args[1];
            std::string lower_arg = arg;
            std::transform(lower_arg.begin(), lower_arg.end(), lower_arg.begin(), ::tolower);

            if (lower_arg == "on") {
                std::cout << "[+] Echo mode: ON\n";
            } else if (lower_arg == "off") {
                std::cout << "[+] Echo mode: OFF\n";
            } else {
                for (size_t i = 1; i < argc; ++i) {
                    if (i > 1) std::cout << " ";
                    std::cout << args[i];
                }
                std::cout << "\n";
            }
        }
        return true;
    }

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
    if (cmd == "delpath" || cmd == "removepath") {
        if (argc < 2) {
            std::cout << "Usage: delpath <index | path_string>\n";
        } else {
            for (size_t i = 1; i < argc; ++i) {
                delete_path(args[i]);
            }
        }
        return true;
    }

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

    return false;  
}
