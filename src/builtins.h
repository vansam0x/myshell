#pragma once

#include "set_color.h"
#include "process_manager.h"
#include <string>
#include <vector>
#include <windows.h>
#include <iostream>
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
    std::cout << "   delpath <path>       Remove a directory from search paths\n";

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

inline std::string getAbsolutePath(const std::string &path) {
    char absPath[MAX_PATH];
    DWORD len = GetFullPathNameA(path.c_str(), MAX_PATH, absPath, NULL);
    if (len > 0 && len < MAX_PATH) {
        return std::string(absPath, len);
    }
    return path;
}

inline std::string getEnvPath() {
    char buffer[32767];
    DWORD len = GetEnvironmentVariableA("PATH", buffer, 32767);
    if (len > 0 && len < 32767) {
        return std::string(buffer, len);
    }
    return "";
}

inline void addpath_internal(const std::string &new_path, bool print = true) {
    std::string abs_path = getAbsolutePath(new_path);

    auto it = std::find(paths.begin(), paths.end(), abs_path);
    if (it == paths.end()) {
        paths.push_back(abs_path);
    }

    std::string current_path = getEnvPath();
    bool exists = false;
    size_t start = 0;
    size_t pos = 0;

    std::string target_norm = abs_path;
    std::transform(target_norm.begin(), target_norm.end(), target_norm.begin(), ::tolower);
    std::replace(target_norm.begin(), target_norm.end(), '/', '\\');

    while ((pos = current_path.find(';', start)) != std::string::npos) {
        std::string part = current_path.substr(start, pos - start);
        std::string part_norm = part;
        std::transform(part_norm.begin(), part_norm.end(), part_norm.begin(), ::tolower);
        std::replace(part_norm.begin(), part_norm.end(), '/', '\\');
        if (part_norm == target_norm) {
            exists = true;
        }
        start = pos + 1;
    }
    if (start < current_path.size()) {
        std::string part = current_path.substr(start);
        std::string part_norm = part;
        std::transform(part_norm.begin(), part_norm.end(), part_norm.begin(), ::tolower);
        std::replace(part_norm.begin(), part_norm.end(), '/', '\\');
        if (part_norm == target_norm) {
            exists = true;
        }
    }

    if (!exists) {
        if (!current_path.empty() && current_path.back() != ';') {
            current_path += ";";
        }
        current_path += abs_path;
        SetEnvironmentVariableA("PATH", current_path.c_str());
    }

    if (print) {
        std::cout << "[+] Path added: " << abs_path << "\n";
    }
}

inline void addpath(const std::string &new_path) {
    addpath_internal(new_path, true);
}

inline void delete_path(const std::string &target) {
    if (target.empty()) {
        std::cout << "Usage: delpath <path>\n";
        return;
    }

    std::string abs_path = getAbsolutePath(target);
    bool found = false;

    std::string target_norm = abs_path;
    std::transform(target_norm.begin(), target_norm.end(), target_norm.begin(), ::tolower);
    std::replace(target_norm.begin(), target_norm.end(), '/', '\\');

    for (auto it = paths.begin(); it != paths.end(); ) {
        std::string p_norm = *it;
        std::transform(p_norm.begin(), p_norm.end(), p_norm.begin(), ::tolower);
        std::replace(p_norm.begin(), p_norm.end(), '/', '\\');
        if (p_norm == target_norm) {
            it = paths.erase(it);
            found = true;
        } else {
            ++it;
        }
    }

    if (!found) {
        std::cout << "[-] Error: Path not found in search paths: " << target << "\n";
        return;
    }

    std::cout << "[+] Path removed: " << abs_path << "\n";

    std::string current_path = getEnvPath();
    std::vector<std::string> parts;
    size_t start = 0;
    size_t pos = 0;
    while ((pos = current_path.find(';', start)) != std::string::npos) {
        std::string part = current_path.substr(start, pos - start);
        std::string part_norm = part;
        std::transform(part_norm.begin(), part_norm.end(), part_norm.begin(), ::tolower);
        std::replace(part_norm.begin(), part_norm.end(), '/', '\\');
        if (part_norm != target_norm && !part.empty()) {
            parts.push_back(part);
        }
        start = pos + 1;
    }
    if (start < current_path.size()) {
        std::string part = current_path.substr(start);
        std::string part_norm = part;
        std::transform(part_norm.begin(), part_norm.end(), part_norm.begin(), ::tolower);
        std::replace(part_norm.begin(), part_norm.end(), '/', '\\');
        if (part_norm != target_norm && !part.empty()) {
            parts.push_back(part);
        }
    }

    std::string new_path_env = "";
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) new_path_env += ";";
        new_path_env += parts[i];
    }
    SetEnvironmentVariableA("PATH", new_path_env.c_str());
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
    printf("Time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
}

void get_date() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    printf("Date: %02d/%02d/%04d\n", st.wDay, st.wMonth, st.wYear);
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

    if (cmd == "path") {
        path();
        return true;
    }
    if (cmd == "addpath") {
        if (argc < 2) {
            std::cout << "Usage: addpath <directory>\n";
        } else {
            std::string full_path = "";
            for (size_t i = 1; i < argc; ++i) {
                if (i > 1) full_path += " ";
                full_path += args[i];
            }
            addpath(full_path);
        }
        return true;
    }
    if (cmd == "delpath" || cmd == "removepath") {
        if (argc < 2) {
            std::cout << "Usage: delpath <path>\n";
        } else {
            std::string full_path = "";
            for (size_t i = 1; i < argc; ++i) {
                if (i > 1) full_path += " ";
                full_path += args[i];
            }
            delete_path(full_path);
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
