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
#include <string>

#include "set_color.h"
#include "parser.h"
#include "process_manager.h"
#include "builtins.h"
#include "executor.h"

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
        if (fg_pid != 0) {
            // A foreground process is running.
            // The OS will also deliver CTRL+C to the child process,
            // which will cause it to terminate. We just need to
            // survive by returning TRUE.
            return TRUE;
        }
        // No foreground process --- just reprint the prompt.
        std::cout << "\nmyShell> ";
        return TRUE;
    }
    return FALSE;  // Let the OS handle other control signals
}

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
// MAIN - The REPL (Read-Eval-Print Loop)
// ============================================================
int main() {
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

        // Step 6: Not a built-in --- execute as an external command
        execute_command(cmd);
    }

    // ---- Cleanup ----
    cleanupAllProcesses();
    return 0;
}
