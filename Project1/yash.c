#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAXINPUTS 100

void execute_command(char * command, char * arguments[], int command_count);
void file_redirection_piping(char * command, char * arguments[], int token_count);
void handle_signal(int signumber);

int main()
{
    pid_t pid;
    char * commands[MAXINPUTS];
    int command_index, token_count;
    char * line;
    char * token;

    command_index = token_count = 0;

    while (line = readline("# "))
    {
        token = strtok(line," ");
        commands[command_index] = token;

        while (token != NULL)
        {
            command_index++;
            token_count++;
            token = strtok(NULL, " ");
            commands[command_index] = token;
        }

        execute_command(commands[0], commands, token_count);

        for (int i = 0; i < MAXINPUTS; i++)
        {
            commands[i] = '\0';
        }
        
        if (signal(SIGHUP, handle_signal) == SIG_ERR)
        {
            printf("caught signal");
        }
        command_index = 0;
    }

}



void execute_command(char * command, char * arguments[], int token_count)
{
    int cpid, error, redirection_flag, file_descriptor;

    printf("%s", arguments[0]);
    printf("hi");

    for (int i = 0; i < token_count; i++)
    {
        if (strcmp(arguments[i], "|") == 0)
        {
            file_redirection_piping(command, arguments, token_count);
            return;
        }
        else if (strcmp(arguments[i], "<") == 0 && i+1 == token_count-1)
        {
            file_descriptor = open(arguments[i+1], O_RDONLY);
            cpid = fork();
            if (cpid == 0)
            {
                dup2(file_descriptor, STDOUT_FILENO);
                execlp(command, command, NULL);
            }
            else
            {
                wait(NULL);
            }
            return;
        }
        else if (strcmp(arguments[i], ">") == 0 && i+1 == token_count-1)
        {
            file_descriptor = creat(arguments[i+1], O_WRONLY);
            cpid = fork();
            if (cpid == 0)
            {
                dup2(file_descriptor, STDIN_FILENO);
                execlp(command, command, NULL);
            }
            else
            {
                wait(NULL);
            }
            return;
        }
        else if (strcmp(arguments[i], "2>") == 0 && i+1 == token_count-1)
        {
            file_descriptor = creat(arguments[i+1], O_WRONLY);
            cpid = fork();
            if (cpid == 0)
            {
                dup2(file_descriptor, STDERR_FILENO);
                execlp(command, command, NULL);
            }
            else
            {
                wait(NULL);
            }
            return;
        }
    }

    
    
    cpid = fork();
    if (cpid == 0)
    {
        execvp(command, arguments);
    }
    else
    {
        wait((int*) NULL);
    }



}

void file_redirection_piping(char * command, char * arguments[], int token_count)
{

}

void handle_signal(int signumber)
{
    switch(signumber)
    {
        case SIGHUP:
            printf("caught sighup aka the ending\n");
            exit(0);
            break;
        case SIGINT:
            printf("caught sigint\n");
            exit(0);
            break;
        case SIGTSTP:
            printf("caught sigtstp\n");
            break;
    }
}