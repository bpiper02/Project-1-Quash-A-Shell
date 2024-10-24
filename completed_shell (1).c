#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_COMMAND_LINE_LEN 1024
#define MAX_COMMAND_LINE_ARGS 128

char prompt[] = "> ";
char delimiters[] = " \t\r\n";
extern char **environ;

void sigint_handler(int sig) {
    printf("\nCaught SIGINT. Use 'exit' to quit the shell.\n");
}

void sigalarm_handler(int sig) {
    // Placeholder for handling long-running processes
    printf("Process terminated due to timeout.\n");
}

int main() {
    // Stores the string typed into the command line.
    char command_line[MAX_COMMAND_LINE_LEN];
    char cmd_bak[MAX_COMMAND_LINE_LEN];

    // Stores the tokenized command line input.
    char *arguments[MAX_COMMAND_LINE_ARGS];
    
    // Handle Ctrl+C
    signal(SIGINT, sigint_handler);
    signal(SIGALRM, sigalarm_handler);
    
    while (true) {
        // Print the shell prompt with current working directory
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s> ", cwd);
        } else {
            perror("getcwd() error");
        }
        fflush(stdout);

        // Read input from stdin and store it in command_line. Exit on error.
        if ((fgets(command_line, MAX_COMMAND_LINE_LEN, stdin) == NULL) && ferror(stdin)) {
            fprintf(stderr, "fgets error");
            exit(0);
        }
        
        // Handle empty inputs
        if (command_line[0] == 0x0A) {
            continue;
        }
        command_line[strlen(command_line) - 1] = '\0'; // Null-terminate input

        // Handle EOF (Ctrl+D)
        if (feof(stdin)) {
            printf("\n");
            fflush(stdout);
            fflush(stderr);
            return 0;
        }

        // Tokenize the command line input
        int argc = 0;
        arguments[argc] = strtok(command_line, delimiters);
        while (arguments[argc] != NULL) {
            argc++;
            arguments[argc] = strtok(NULL, delimiters);
        }

        // Implement built-in commands
        if (strcmp(arguments[0], "cd") == 0) {
            if (chdir(arguments[1]) != 0) {
                perror("cd failed");
            }
            continue;
        } else if (strcmp(arguments[0], "pwd") == 0) {
            printf("%s\n", cwd);
            continue;
        } else if (strcmp(arguments[0], "echo") == 0) {
            int i;
            for (i = 1; i < argc; i++) {
                printf("%s ", arguments[i]);
            }
            printf("\n");
            continue;
        } else if (strcmp(arguments[0], "exit") == 0) {
            printf("Exiting shell...\n");
            exit(0);
        } else if (strcmp(arguments[0], "env") == 0) {
            char **env;
            for (env = environ; *env != 0; env++) {
                printf("%s\n", *env);
            }
            continue;
        } else if (strcmp(arguments[0], "setenv") == 0) {
            if (setenv(arguments[1], arguments[2], 1) != 0) {
                perror("setenv failed");
            }
            continue;
        }

        // Implementing process forking
        int background = 0;
        if (strcmp(arguments[argc-1], "&") == 0) {
            background = 1;
            arguments[argc-1] = NULL; // Remove the '&'
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            if (execvp(arguments[0], arguments) == -1) {
                perror("execvp failed");
            }
            exit(EXIT_FAILURE);
        } else if (pid < 0) {
            // Forking failed
            perror("fork failed");
        } else {
            // Parent process waits for the child unless it's a background process
            if (!background) {
                wait(NULL);
            }
        }
    }
}