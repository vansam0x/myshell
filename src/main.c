#include <windows.h>
#include <stdio.h> 

#include "utils.h"
#include "builtins.h"

#define MAX_INPUT_LENGTH 1024 
#define SHELL_NAME "vansam's shell"
int main() {
    printf("Welcome to %s! Type 'help' for a list of commands.\n", SHELL_NAME);

    while(1) {
        char input[MAX_INPUT_LENGTH];
        printf("%s> ", SHELL_NAME); 
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("\n");
            break; // EOF (Ctrl+D)
        }
        // Remove trailing newline
        input[strcspn(input, "\r\n")] = 0;
        
    }
}