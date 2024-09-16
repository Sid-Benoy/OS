#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <errno.h>
#include <wait.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

#include <readline/readline.h>
#include <readline/history.h>

#define MAXINPUTS 100
#define MAXLABELS 10

void execute_command(char * command, char * arguments[], int token_count);
void file_redirection_piping(char * command, char * command2, char * arguments[], int token_count);
void handle_signal(int signumber);
void clear_args(char * arguments[]);

int main()
{
    pid_t pid;
    char * commands[MAXINPUTS];
    int command_index, token_count;
    char * line;
    char * token;

    command_index = token_count = 0;

    while ((line = readline("# ")))
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

        clear_args(commands);
        
//        if (signal(SIGHUP, handle_signal) == SIG_ERR)
//        {
//            printf("caught signal");
//        }
        command_index = 0;
        token_count = 0;
        free(line);
    }

}



void execute_command(char * command, char * arguments[], int token_count)
{
    int cpid, error, redirection_flag, file_descriptor, sub_args_index;

    sub_args_index = 0;

    char * sub_args[MAXINPUTS];

    for (int i = 0; i < token_count; i++)
    {
        if (strcmp(arguments[i], "|") == 0)
        {
            char * command2 = arguments[i+1];
            file_redirection_piping(command, command2, arguments, token_count);
            //check for bg/fg
            return;
        }
        else if (strcmp(arguments[i], "<") == 0 && i+1 == token_count-1)
        {
            file_descriptor = open(arguments[i+1], O_RDONLY);
            cpid = fork();
            if (cpid == 0)
            {
                dup2(file_descriptor, STDIN_FILENO);
                execvp(command, sub_args);
            }
            else
            {
                wait(NULL);
            }
            close(file_descriptor);
            return;
        }
        else if (strcmp(arguments[i], ">") == 0 && i+1 == token_count-1)
        {
            file_descriptor = creat(arguments[i+1], O_WRONLY);
            cpid = fork();
            if (cpid == 0)
            {
                dup2(file_descriptor, STDOUT_FILENO);
                execvp(command, sub_args);
            }
            else
            {
                wait(NULL);
            }
            close(file_descriptor);
            clear_args(sub_args);
            return;
        }
        else if (strcmp(arguments[i], "2>") == 0 && i+1 == token_count-1)
        {
            file_descriptor = creat(arguments[i+1], O_WRONLY);
            cpid = fork();
            if (cpid == 0)
            {
                dup2(file_descriptor, STDERR_FILENO);
                execvp(command, sub_args);
            }
            else
            {
                wait(NULL);
            }
            close(file_descriptor);
            return;
        }
        else
        {
            sub_args[sub_args_index] = arguments[i];
            sub_args[sub_args_index + 1] = NULL;
            sub_args_index++;
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

void file_redirection_piping(char * command, char * command2, char * arguments[], int token_count)
{
    int pdf[2];
    int cpid, lpid, argument_index;
    argument_index = 0;
    char * left_arguments[MAXINPUTS];
    char * right_arguments[MAXINPUTS];

    for (int i = 0; i < token_count; i++)           //gets command options into two arrays for both commands
    {
        while ((strcmp(arguments[i], ">") == 0) || (strcmp(arguments[i], "<") == 0) || (strcmp(arguments[i], "2>") ==0))
        {
            left_arguments[argument_index] = arguments[i];
            argument_index++;
            left_arguments[argument_index] = NULL;
            i++;
        }
        argument_index = 0;
        if (strcmp(arguments[i], command2) == 0)
        {
            while ((strcmp(arguments[i], ">") == 0) || (strcmp(arguments[i], "<") == 0) || (strcmp(arguments[i], "2>") ==0))
            {
                right_arguments[argument_index] = arguments[i];
                argument_index++;
                right_arguments[argument_index] = NULL;
                i++;
            }
        }
    }



    if (pipe(pdf) == -1)
    {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < token_count; i++)
    {

    }
}

void clear_args(char * arguments[])
{
    for (int i = 0; i < MAXINPUTS; i++)
    {
        arguments[i] = '\0';
    }
}

//void handle_signal(int signumber)
//{
//    switch(signumber)
//    {
//        case SIGHUP:
//            printf("caught sighup aka the ending\n");
//            exit(0);
//        case SIGINT:
//            printf("caught sigint\n");
//            exit(0);
//        case SIGTSTP:
//            printf("caught sigtstp\n");
//            break;
//        default:
//            printf("hi");
//            break;
//    }
//}

