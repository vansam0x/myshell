#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include "set_color.h"
#include "parser.h"
#include "process_manager.h"
#include "builtins.h"
#include "executor.h"

void printPrompt() {
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    std::cout << GREEN << "myShell " << RESET 
              << CYAN << cwd << RESET 
              << "> ";
}

BOOL WINAPI CtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT) {
        stopBatchExecution = TRUE;
        if (isRunningForeground && hForegroundProcess != NULL) {
            TerminateProcess(hForegroundProcess, 0);
            return TRUE;
        }
        std::cout << "\n";
        printPrompt();
        return TRUE;
    }
    return FALSE; 
}

static bool isBatFile(const std::string &cmd) {
    std::string lower = cmd;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    size_t len = lower.size();
    if (len >= 4 && (lower.substr(len - 4) == ".bat" || lower.substr(len - 4) == ".cmd")) {
        return true;
    }
    return false;
}
void execute_bat_file(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << RED << "[-] Error: Cannot open file '" << filename << "'\n" << RESET;
        return;
    }

    std::cout << CYAN << "[*] Executing batch file: " << filename << "\n" << RESET;

    stopBatchExecution = FALSE; 
    bool echoOn = true;  
    std::string line;


    while (std::getline(file, line)) {
        if (stopBatchExecution) {
            std::cout << RED << "[!] Batch file execution interrupted.\n" << RESET;
            break;
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) continue;
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
        bool suppressEcho = false;
        std::string processLine = line;
        if (!processLine.empty() && processLine[0] == '@') {
            suppressEcho = true;
            processLine = processLine.substr(1);
            size_t start = processLine.find_first_not_of(" \t");
            if (start != std::string::npos) {
                processLine = processLine.substr(start);
            } else {
                continue;  
            }
        }
        std::string lowerProcess = processLine;
        std::transform(lowerProcess.begin(), lowerProcess.end(), lowerProcess.begin(), ::tolower);

        if (lowerProcess == "echo off") {
            echoOn = false;
            continue;
        }
        if (lowerProcess == "echo on") {
            echoOn = true;
            continue;
        }
        if (lowerProcess.substr(0, 4) == "rem " || lowerProcess.substr(0, 3) == "rem" ||
            processLine.substr(0, 2) == "::") {
            continue;
        }
        if (echoOn && !suppressEcho) {
            std::cout << YELLOW << ">> " << processLine << RESET << "\n";
        }
        ParsedCommand cmd = parse_command(processLine);
        if (cmd.command.empty()) continue;
        if (cmd.command == "exit") {
            std::cout << CYAN << "[*] Batch file ended by 'exit' command.\n" << RESET;
            break;
        }
        if (!handle_builtin(cmd.command, cmd.argc, cmd.args)) {
            execute_command(cmd);
        }
    }

    file.close();
    std::cout << CYAN << "[*] Batch file '" << filename << "' finished.\n" << RESET;
}
int main(int argc, char* argv[]) {
    if (argc > 1) {
        std::string batchFile = argv[1];
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--paths" && i + 1 < argc) {
                std::string pathsStr = argv[i + 1];
                size_t pos = 0;
                while ((pos = pathsStr.find(';')) != std::string::npos) {
                    std::string p = pathsStr.substr(0, pos);
                    if (!p.empty()) {
                        addpath_internal(p, false);
                    }
                    pathsStr.erase(0, pos + 1);
                }
                if (!pathsStr.empty()) {
                    addpath_internal(pathsStr, false);
                }
                break;
            }
        }
        if (isBatFile(batchFile)) {
            execute_bat_file(batchFile);
            return 0;
        }
    }
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
    std::cout << CYAN << BOLD
              << "============================================\n"
              << "    Welcome to myShell! (v1.0)              \n"
              << "    Type 'help' for available commands.      \n"
              << "============================================\n"
              << RESET << "\n";
    while (true) {
        printPrompt();
        std::string input;
        if (!std::getline(std::cin, input)) {
            std::cout << "\n";
            break;
        }
        if (input.empty()) continue;
        ParsedCommand cmd = parse_command(input);
        if (cmd.command.empty()) continue;
        if (handle_builtin(cmd.command, cmd.argc, cmd.args)) {
            continue;   
        }
        if (isBatFile(cmd.command)) {
            if (cmd.isBackground) {
                char szPath[MAX_PATH];
                GetModuleFileNameA(NULL, szPath, MAX_PATH);
                ParsedCommand bgBatCmd;
                bgBatCmd.command = cmd.command;
                bgBatCmd.args.push_back(szPath);
                bgBatCmd.args.push_back(cmd.command);
                if (!paths.empty()) {
                    bgBatCmd.args.push_back("--paths");
                    std::string pathsStr;
                    for (size_t i = 0; i < paths.size(); ++i) {
                        if (i > 0) pathsStr += ";";
                        pathsStr += paths[i];
                    }
                    bgBatCmd.args.push_back(pathsStr);
                }

                bgBatCmd.argc = bgBatCmd.args.size();
                bgBatCmd.isBackground = true;
                bgBatCmd.fullCommandLine = "\"" + std::string(szPath) + "\"";
                for (size_t i = 1; i < bgBatCmd.args.size(); ++i) {
                    bgBatCmd.fullCommandLine += " " + bgBatCmd.args[i];
                }

                execute_command(bgBatCmd);
            } else {
                execute_bat_file(cmd.command);
            }
            continue;
        }
        execute_command(cmd);
    }
    cleanupAllProcesses();
    return 0;
}
