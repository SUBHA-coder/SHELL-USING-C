#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARG_SIZE 100
#define HISTORY_SIZE 10

char history[HISTORY_SIZE][MAX_INPUT_SIZE];
int history_count = 0;

// ANSI color codes
#define ANSI_COLOR_RESET   "\x1b[0m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"

void parse_input(char* input, char** args) {
    for (int i = 0; i < MAX_ARG_SIZE; i++) {
        args[i] = strsep(&input, " ");
        if (args[i] == NULL) break;
    }
}

void parse_pipes(char* input, char** commands) {
    for (int i = 0; i < MAX_ARG_SIZE; i++) {
        commands[i] = strsep(&input, "|");
        if (commands[i] == NULL) break;
    }
}

void add_to_history(char* input) {
    strncpy(history[history_count % HISTORY_SIZE], input, MAX_INPUT_SIZE);
    history_count++;
}

void show_history() {
    int start = history_count >= HISTORY_SIZE ? history_count - HISTORY_SIZE : 0;
    for (int i = start; i < history_count; i++) {
        printf("%d: %s\n", i - start + 1, history[i % HISTORY_SIZE]);
    }
}

void execute_builtin(char **args) {
    if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, ANSI_COLOR_RED "myshell: expected argument to \"cd\"\n" ANSI_COLOR_RESET);
        } else {
            if (chdir(args[1]) != 0) {
                perror("myshell");
            }
        }
    } else if (strcmp(args[0], "mkdir") == 0) {
        if (args[1] == NULL) {
            fprintf(stderr, ANSI_COLOR_RED "myshell: expected argument to \"mkdir\"\n" ANSI_COLOR_RESET);
        } else {
            if (mkdir(args[1], 0755) != 0) {
                perror("myshell");
            }
        }
    } else if (strcmp(args[0], "history") == 0) {
        show_history();
    }
}

void execute_piped_commands(char** commands) {
    int fd[2];
    pid_t pid;
    int fd_in = 0;  // First process should read from standard input

    for (int i = 0; commands[i] != NULL; i++) {
        pipe(fd);
        if ((pid = fork()) == -1) {
            perror("myshell");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            dup2(fd_in, 0);  // Change the input according to the old one
            if (commands[i + 1] != NULL) {
                dup2(fd[1], 1);
            }
            close(fd[0]);
            char* args[MAX_ARG_SIZE];
            parse_input(commands[i], args);
            execvp(args[0], args);
            perror("myshell");
            exit(EXIT_FAILURE);
        } else {
            wait(NULL);
            close(fd[1]);
            fd_in = fd[0];  // Save the input for the next command
        }
    }
}

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARG_SIZE];

    while (1) {
        printf(ANSI_COLOR_GREEN "myshell> " ANSI_COLOR_RESET);  // Green prompt
        fgets(input, MAX_INPUT_SIZE, stdin);
        input[strcspn(input, "\n")] = '\0';  // Remove newline character

        if (strlen(input) == 0) continue;  // Ignore empty input

        add_to_history(input);

        char* commands[MAX_ARG_SIZE];
        parse_pipes(input, commands);

        if (commands[1] != NULL) {
            execute_piped_commands(commands);
        } else {
            parse_input(input, args);

            if (strcmp(args[0], "exit") == 0) {
                break;
            }

            if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "mkdir") == 0 || strcmp(args[0], "history") == 0) {
                execute_builtin(args);
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) {
                // Child process
                if (execvp(args[0], args) == -1) {
                    perror("myshell");
                }
                exit(EXIT_FAILURE);
            } else if (pid < 0) {
                perror("myshell");
            } else {
                // Parent process
                int status;
                waitpid(pid, &status, 0);
            }
        }
    }
    return 0;
}
