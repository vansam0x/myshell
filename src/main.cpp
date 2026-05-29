// ============================================================
// MYSHELL - MAIN ENTRY POINT
// ============================================================
// A tiny shell for Windows, built as an Operating Systems
// course project at HUST.
//
// Architecture:
//   main.cpp          -> REPL loop + CTRL+C handler
//   parser.h          -> Parse input into ParsedCommand
//   builtins.h        -> Execute built-in commands
//   executor.h        -> CreateProcess for external commands
//   process_manager.h -> Track and manage background processes
// ============================================================

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>

#include "set_color.h"
#include "parser.h"
#include "process_manager.h"
#include "builtins.h"
#include "executor.h"

// ============================================================
// printPrompt - Display the shell prompt with current directory
// ============================================================
void printPrompt() {
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, cwd);
    std::cout << GREEN << "myShell " << RESET 
              << CYAN << cwd << RESET 
              << "> ";
}

// ============================================================
// CTRL+C HANDLER
// ============================================================
// Without this handler, CTRL+C would kill BOTH the shell and
// any running child process. We want to:
//   - Keep the shell alive (return TRUE)
//   - Let the foreground child process die naturally
//
// See: SetConsoleCtrlHandler() documentation
// ============================================================
BOOL WINAPI CtrlHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_C_EVENT) {
        stopBatchExecution = TRUE;
        if (isRunningForeground && hForegroundProcess != NULL) {
            // Forcefully terminate the foreground process to ensure it terminates
            TerminateProcess(hForegroundProcess, 0);
            return TRUE;
        }
        // No foreground process --- just reprint the prompt.
        std::cout << "\n";
        printPrompt();
        return TRUE;
    }
    return FALSE;  // Let the OS handle other signals
}

// ============================================================
// Helper: check if a command is a .bat/.cmd file
// ============================================================
static bool isBatFile(const std::string &cmd) {
    std::string lower = cmd;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    size_t len = lower.size();
    if (len >= 4 && (lower.substr(len - 4) == ".bat" || lower.substr(len - 4) == ".cmd")) {
        return true;
    }
    return false;
}

// ============================================================
// execute_bat_file - Interpret a .bat file using myShell
// ============================================================
// Reads the .bat file line by line and processes each line
// through myShell's own pipeline:
//   parse -> handle_builtin -> execute_command
//
// Supports:
//   - @echo off (suppress echo)
//   - REM / :: comments (skip)
//   - All myShell built-in commands (help, date, list, etc.)
//   - External commands via CreateProcess
// ============================================================
void execute_bat_file(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << RED << "[-] Error: Cannot open file '" << filename << "'\n" << RESET;
        return;
    }

    std::cout << CYAN << "[*] Executing batch file: " << filename << "\n" << RESET;

    stopBatchExecution = FALSE; // Reset the interruption flag
    bool echoOn = true;  // @echo off can disable command echoing
    std::string line;


    while (std::getline(file, line)) {
        if (stopBatchExecution) {
            std::cout << RED << "[!] Batch file execution interrupted.\n" << RESET;
            break;
        }

        // Remove trailing \r if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // Skip empty lines
        if (line.empty()) continue;

        // Handle @echo off (case-insensitive)
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

        // Remove leading '@' (suppresses echo for this line)
        bool suppressEcho = false;
        std::string processLine = line;
        if (!processLine.empty() && processLine[0] == '@') {
            suppressEcho = true;
            processLine = processLine.substr(1);
            // Trim leading spaces after @
            size_t start = processLine.find_first_not_of(" \t");
            if (start != std::string::npos) {
                processLine = processLine.substr(start);
            } else {
                continue;  // Line was just "@"
            }
        }

        // Check for "echo off" / "echo on"
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

        // Skip comments: REM or ::
        if (lowerProcess.substr(0, 4) == "rem " || lowerProcess.substr(0, 3) == "rem" ||
            processLine.substr(0, 2) == "::") {
            continue;
        }

        // Echo the command if echo is on and not suppressed
        if (echoOn && !suppressEcho) {
            std::cout << YELLOW << ">> " << processLine << RESET << "\n";
        }

        // Parse and execute through myShell's pipeline
        ParsedCommand cmd = parse_command(processLine);
        if (cmd.command.empty()) continue;

        // Handle 'exit' specially in batch files --- just stop the script
        if (cmd.command == "exit") {
            std::cout << CYAN << "[*] Batch file ended by 'exit' command.\n" << RESET;
            break;
        }

        // Try built-in first, then external
        if (!handle_builtin(cmd.command, cmd.argc, cmd.args)) {
            execute_command(cmd);
        }
    }

    file.close();
    std::cout << CYAN << "[*] Batch file '" << filename << "' finished.\n" << RESET;
}

// ============================================================
// MAIN - The REPL (Read-Eval-Print Loop)
// ============================================================
int main(int argc, char* argv[]) {
    // ---- Command-line Argument Parsing for Batch Execution & Path Inheritance ----
    if (argc > 1) {
        std::string batchFile = argv[1];
        // Parse custom search paths if provided: --paths "dir1;dir2;..."
        for (int i = 2; i < argc; ++i) {
            if (std::string(argv[i]) == "--paths" && i + 1 < argc) {
                std::string pathsStr = argv[i + 1];
                size_t pos = 0;
                while ((pos = pathsStr.find(';')) != std::string::npos) {
                    std::string p = pathsStr.substr(0, pos);
                    if (!p.empty()) {
                        paths.push_back(p);
                    }
                    pathsStr.erase(0, pos + 1);
                }
                if (!pathsStr.empty()) {
                    paths.push_back(pathsStr);
                }
                break;
            }
        }
        if (isBatFile(batchFile)) {
            // Execute the batch file silently or print as configured inside the file
            execute_bat_file(batchFile);
            return 0;
        }
    }

    // ---- Initialization ----
    // Enable ANSI escape codes for colored output on Windows 10+
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // Register CTRL+C handler so the shell doesn't die
    SetConsoleCtrlHandler(CtrlHandler, TRUE);

    // ---- Welcome message ----
    std::cout << CYAN << BOLD
              << "============================================\n"
              << "    Welcome to myShell! (v1.0)              \n"
              << "    Type 'help' for available commands.      \n"
              << "============================================\n"
              << RESET << "\n";

    // ---- Main REPL loop ----
    while (true) {
        // Step 1: Display prompt
        printPrompt();

        // Step 2: Read user input
        std::string input;
        if (!std::getline(std::cin, input)) {
            // EOF (e.g., Ctrl+Z on Windows) --- exit gracefully
            std::cout << "\n";
            break;
        }

        // Step 3: Skip empty input
        if (input.empty()) continue;

        // Step 4: Parse the input into a structured command
        ParsedCommand cmd = parse_command(input);
        if (cmd.command.empty()) continue;

        // Step 5: Try to execute as a built-in command
        if (handle_builtin(cmd.command, cmd.argc, cmd.args)) {
            continue;   // Built-in handled it, back to prompt
        }

        // Step 6: Check if it's a .bat/.cmd file --- interpret with myShell
        if (isBatFile(cmd.command)) {
            if (cmd.isBackground) {
                // Background mode: spawn a new myShell process to run the batch file
                char szPath[MAX_PATH];
                GetModuleFileNameA(NULL, szPath, MAX_PATH);

                ParsedCommand bgBatCmd;
                bgBatCmd.command = cmd.command;
                bgBatCmd.args.push_back(szPath);
                bgBatCmd.args.push_back(cmd.command);

                // Pass custom search paths
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

                // Build fullCommandLine for logging/execution
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

        // Step 7: Not a built-in and not a bat --- execute as external command
        execute_command(cmd);
    }

    // ---- Cleanup ----
    cleanupAllProcesses();
    return 0;
}
