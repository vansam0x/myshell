#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define MAX_INPUT_LENGTH   1024   // max length of user input command line
#define MAX_ARGS           64     // max number of arguments for a command
#define MAX_BG_PROCESSES   100    // max number of background processes
#define SHELL_NAME         "myShell"

// background process status codes
#define STATUS_RUNNING     0
#define STATUS_STOPPED     1
#define STATUS_TERMINATED  2

// ============================================================
// DATA STRUCTURES
// ============================================================

#pragma once

// struct storing parsed command information
typedef struct {
    char *command;              // Command name (first token)
    char *args[MAX_ARGS];       // Array of arguments (including command at args[0])
    int   argCount;             // Number of arguments
    int   isBackground;         // 1 if '&' is at the end, 0 otherwise
    char  fullCommandLine[MAX_INPUT_LENGTH]; // Full command line for CreateProcess
} LegacyParsedCommand;

// Struct storing information about a background process
typedef struct {
    DWORD  pid;                 // Process ID
    HANDLE hProcess;            // Handle to the process (used for Kill, Wait)
    HANDLE hThread;             // Handle to the main thread (used for Stop, Resume)
    char   cmdName[MAX_INPUT_LENGTH]; // Name of the command that was executed
    int    status;              // STATUS_RUNNING, STATUS_STOPPED, STATUS_TERMINATED
} BackgroundProcess;

// ============================================================
// UTILITY FUNCTIONS
// ============================================================

/**
 * Converts a background process status code to a descriptive string.
 * Example: STATUS_RUNNING → "RUNNING"
 *
 * @param status  Status code (STATUS_RUNNING, STATUS_STOPPED, STATUS_TERMINATED)
 * @return        Descriptive string for the status
 */
const char* getStatusString(int status);
// TODO: Implement trong utils.c hoặc ngay tại đây (inline)
// Gợi ý: dùng switch-case

/**
 * Removes the newline character '\n' and '\r' from the end of a string.
 *
 * @param str  String to process (will be modified directly)
 */
void trimNewline(char *str);
// TODO: Implement
// Hint: find '\n' or '\r' at the end of the string, replace with '\0'

/**
 * Checks if a string ends with a specific suffix.
 * Used to detect .bat files.
 *
 * @param str     String to check  
 * @param suffix  Suffix to compare against (e.g., ".bat")
 * @return        1 if the string ends with the suffix, 0 otherwise
 */
int endsWith(const char *str, const char *suffix);
// TODO: Implement
// Hint: Get the lengths of str and suffix, then compare the end of str with suffix using strcmp.