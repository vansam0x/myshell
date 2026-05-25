// ============================================================
// BUILTINS MODULE
// ============================================================
// Module execute built-in commands — commands that are implemented directly in the shell, 
// not by running an external program.
//
// Why do we need built-ins?
// - "exit" must run within the shell process itself
// - "addpath" modifies the shell process's environment
//   (if run in a child process, the changes would be lost when the child process exits)
// ============================================================

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// check the built-in command and execute it, return 1 if it's a built-in command, 
// otherwise return 0

#include "set_color.h"

#define MAX_PATHS 10000

char* path_list[MAX_PATHS]; // array to store paths
int path_count = 0; // number of paths in the list

void help() {
    Print(GREEN, "Available built-in commands:\n");
    Print(YELLOW, "help: Show this help message\n");
    Print(YELLOW, "exit: Exit the shell\n");
    Print(YELLOW, "time: Display the current time\n");
    Print(YELLOW, "date: Display the current date\n");
    Print(YELLOW, "dir: List files in the current directory\n");
    Print(YELLOW, "path: Show the list of paths\n");
    Print(YELLOW, "addpath <new_path>: Add a new path to the list\n");
}

void get_time() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    Print(GREEN, "Current time: %02d:%02d:%02d\n", st.wHour, st.wMinute, st.wSecond);
}

void get_date() {
    SYSTEMTIME st; 
    GetLocalTime(&st); 
    Print(GREEN, "Current date: %02d/%02d/%04d\n", st.wDay, st.wMonth, st.wYear);
}

void dir(char * path) {
    if(!path) {
        path = "."; // default to current directory
    }
    WIN32_FIND_DATA findFileData; 
    HANDLE hFind = FindFirstFile(path, &findFileData);
    if (hFind == INVALID_HANDLE_VALUE) {
        Print(RED, "Failed to list directory : %s\n", path);
        return;
    }
    Print(GREEN, "Files in directory : %s\n", path);
    Print(YELLOW, "Name\tCreation Time\tSize\n");
    Print(YELLOW, "-----------------------------------\n");
    do {
        Print(BLUE, findFileData.cFileName, " ", findFileData.ftCreationTime.dwLowDateTime, findFileData.nFileSizeLow, "\n");
    } while (FindNextFile(hFind, &findFileData) != 0);

    FindClose(hFind);
}

void path() {
    if(path_count == 0) {
        Print(YELLOW, "No paths in the list. \n");
        return; 
    }
    Print(GREEN, "Current paths in the list : \n");
    for (int i = 0; i < path_count; ++i) {
        Print(BLUE, path_list[i], "\n");
    }
}

void addpath(const char * new_path) {
    if(path_count >= MAX_PATHS) {
        Print(RED, "Path list is full. Cannot add more paths. \n");
        return;
    }
    path_list[path_count++] = _strdup(new_path); 
}


int handle_builtin(const char* cmd, int argc,char * args[]) {
    if (strcmp(cmd, "help") == 0) {
        help();
        return 1; 
    }
    if(strcmp(cmd, "exit") == 0) {
        exit(0);
        return 1; 
    }
    if(strcmp(cmd, "time") == 0) {
        get_time();
        return 1; 
    }
    if(strcmp(cmd, "date") == 0) {
        get_date();
        return 1; 
    }
    if(strcmp(cmd, "dir") == 0) {
        dir(argc > 0 ? args[0] : NULL);
        return 1;   
    }

    if(strcmp(cmd, "path") == 0) {
        path();
        return 1;
    }

    if(strcmp(cmd, "addpath") == 0) {
        for (int i = 0; i < argc; ++i) {
            addpath(args[i]);
        }
        return 1;
    }
    return 0;
}
