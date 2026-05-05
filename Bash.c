#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64

// Function to print banner
void printBanner() {
    printf("  _____MY CUSTOM SHELL______ \n\n");
    
}

// Function to build the prompt string with current directory
void buildPrompt(char *prompt, size_t size) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        snprintf(prompt, size, "\nDir: ???\n>>> ");
    } else {
        snprintf(prompt, size, "\nDir: %s\n>>> ", cwd);
    }
}

// Function to take input using readline (supports up/down history navigation)
char *takeInput() {
    char prompt[1100];
    buildPrompt(prompt, sizeof(prompt));

    char *input = readline(prompt);

    if (input == NULL) {
        printf("\nExiting...\n");
        exit(0);
    }

    if (strlen(input) > 0) {
        add_history(input);
    }

    return input;
}

// Function to split input into command + arguments
void parseInput(char *input, char **args) {
    int i = 0;
    args[i] = strtok(input, " ");
    while (args[i] != NULL && i < MAX_ARGS - 1) {
        i++;
        args[i] = strtok(NULL, " ");
    }
}

// Function to handle built-in shell commands
int handleBuiltIn(char **args) {
    if (args[0] == NULL) return 1;

    if (strcmp(args[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            printf("Expected argument to \"cd\"\n");
        } else {
            if (chdir(args[1]) != 0) {
                perror("cd failed");
            }
        }
        return 1;
    }

    if (strcmp(args[0], "history") == 0) {
        HIST_ENTRY **hist = history_list();
        if (hist == NULL) {
            printf("No history yet.\n");
        } else {
            for (int i = 0; hist[i] != NULL; i++) {
                printf("  %d  %s\n", i + 1, hist[i]->line);
            }
        }
        return 1;
    }

    if (strcmp(args[0], "help") == 0) {
        printf("\nSimple Shell Help\n");
        printf("Built-in commands:\n");
        printf("  cd <dir>\n  exit\n  help\n  history\n");
        printf("Supports pipes: ls | grep txt\n");
        printf("Use up/down arrow keys to navigate command history.\n");
        return 1;
    }

    return 0;
}

// Function to execute a normal (non-piped) command
void executeCommand(char **args) {
    pid_t pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) < 0) {
            perror("Command failed");
        }
        exit(1);
    } else {
        wait(NULL);
    }
}

// Function to execute two commands connected by a pipe
void executePiped(char **args1, char **args2) {
    int pipefd[2];
    pipe(pipefd);

    pid_t p1 = fork();
    if (p1 == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(args1[0], args1);
        perror("Pipe cmd1 failed");
        exit(1);
    }

    pid_t p2 = fork();
    if (p2 == 0) {
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);
        execvp(args2[0], args2);
        perror("Pipe cmd2 failed");
        exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    wait(NULL);
    wait(NULL);
}

// Function to detect and split input around a pipe '|'
int parsePipe(char *input, char **left, char **right) {
    char *pipePos = strchr(input, '|');
    if (!pipePos) return 0;

    *pipePos = '\0';
    pipePos++;
    while (*pipePos == ' ') pipePos++;

    *left = input;
    *right = pipePos;
    return 1;
}

int main() {
    char *args1[MAX_ARGS], *args2[MAX_ARGS];

    using_history();
    printBanner();

    while (1) {
        char *input = takeInput();

        if (strlen(input) == 0) {
            free(input);
            continue;
        }

        char inputCopy[MAX_INPUT];
        strncpy(inputCopy, input, MAX_INPUT - 1);
        inputCopy[MAX_INPUT - 1] = '\0';
        free(input);

        char *left, *right;

        if (parsePipe(inputCopy, &left, &right)) {
            parseInput(left, args1);
            parseInput(right, args2);
            executePiped(args1, args2);
        } else {
            parseInput(inputCopy, args1);
            if (handleBuiltIn(args1)) continue;
            executeCommand(args1);
        }
    }

    return 0;
}
