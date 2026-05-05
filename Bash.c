#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define INPUT_SIZE 1024
#define ARG_LIMIT  64


/* ---------- UI FUNCTIONS ---------- */

void show_banner() {
    printf("\n=========================================\n");
    printf("              My Custom Shell\n");
    printf("=========================================\n");
}


void read_input(char *buffer) {
    printf("\n$ ");

    if (fgets(buffer, INPUT_SIZE, stdin) == NULL) {
        printf("\nExiting shell...\n");
        exit(0);
    }

    buffer[strcspn(buffer, "\n")] = '\0';  // remove newline
}


void display_directory() {
    char path[1024];

    if (getcwd(path, sizeof(path)) == NULL) {
        perror("Error fetching directory");
        return;
    }

    printf("\n[%s]", path);
}


int split_pipe(char *input, char **cmd1, char **cmd2) {
    char *pipe_ptr = strchr(input, '|');

    if (!pipe_ptr) return 0;

    *pipe_ptr = '\0';
    pipe_ptr++;

    while (*pipe_ptr == ' ') pipe_ptr++;

    *cmd1 = input;
    *cmd2 = pipe_ptr;

    return 1;
}


/* ---------- PARSING ---------- */

void tokenize_input(char *input, char **arguments) {
    int index = 0;

    arguments[index] = strtok(input, " ");

    while (arguments[index] != NULL && index < ARG_LIMIT - 1) {
        index++;
        arguments[index] = strtok(NULL, " ");
    }
}


/* ---------- BUILT-IN COMMANDS ---------- */

int execute_builtin(char **args) {

    if (args[0] == NULL) return 1;

    if (strcmp(args[0], "exit") == 0) {
        printf("Closing shell...\n");
        exit(0);
    }

    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            printf("cd: missing argument\n");
        } else if (chdir(args[1]) != 0) {
            perror("cd error");
        }
        return 1;
    }

    if (strcmp(args[0], "help") == 0) {
        printf("\nAvailable commands:\n");
        printf("  cd <dir>\n  exit\n  help\n");
        printf("Supports piping: cmd1 | cmd2\n");
        return 1;
    }

    return 0;
}


/* ---------- EXECUTION ---------- */

void run_command(char **args) {
    pid_t pid = fork();

    if (pid == 0) {
        if (execvp(args[0], args) < 0) {
            perror("Execution failed");
        }
        exit(1);
    } 
    else {
        wait(NULL);
    }
}

void run_piped_commands(char **left_cmd, char **right_cmd) {
    int fd[2];
    pipe(fd);

    pid_t first = fork();

    if (first == 0) {
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]);
        close(fd[1]);

        execvp(left_cmd[0], left_cmd);
        perror("Pipe execution failed (cmd1)");
        exit(1);
    }

    pid_t second = fork();

    if (second == 0) {
        dup2(fd[0], STDIN_FILENO);
        close(fd[1]);
        close(fd[0]);

        execvp(right_cmd[0], right_cmd);
        perror("Pipe execution failed (cmd2)");
        exit(1);
    }

    close(fd[0]);
    close(fd[1]);

    wait(NULL);
    wait(NULL);
}


/* ---------- MAIN LOOP ---------- */

int main() {

    char input[INPUT_SIZE];
    char *args1[ARG_LIMIT];
    char *args2[ARG_LIMIT];

    show_banner();

    while (1) {

        display_directory();
        read_input(input);

        if (strlen(input) == 0) continue;

        char *left, *right;

        if (split_pipe(input, &left, &right)) {

            tokenize_input(left, args1);
            tokenize_input(right, args2);

            run_piped_commands(args1, args2);

        } 
        else {

            tokenize_input(input, args1);

            if (execute_builtin(args1)) continue;

            run_command(args1);
        }
    }

    return 0;
}