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
#define MAXCHARS 2000
#define MAXLABELS 10

typedef enum {Stopped, Running, Done} status;

mode_t mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH;
int jid = 0;
int child_id = 0;

void execute_command(char * command, char * arguments[], int token_count);
void file_redirection_piping(char * command, char * command2, char * arguments[], int token_count);
void sig_handler(int signumber);
void clear_args(char * arguments[]);
void printjobs();
void add_job(pid_t pid, char * command, status stat, bool bg);
struct Job
{
    int job_id;
    int pgid;
    status state;
    char * jstr;
    bool bg;
    struct job * next;
};

struct Job * jobs[MAXINPUTS];
int job_index = 0;

void add_job(pid_t pid, char * command, status t, bool bg)
{

}
int main()
{

    signal(SIGCHLD, sig_handler);
    signal(SIGINT, sig_handler);
    signal(SIGTSTP, sig_handler);

    pid_t pid;
    char * commands[MAXINPUTS];
    int command_index, token_count;
    char * line;
    char * token;

    command_index = token_count = 0;

    while ((line = readline("# ")))
    {
        if(strcmp(line, "") == 0)
            continue;

        add_history(line);
        token = strtok(line," ");
        commands[command_index] = token;

        if (strcmp(token, "fg") == 0)
        {

        }
        else if (strcmp(token, "bg") == 0)
        {

        }
        else if (strcmp(token, "jobs") == 0)
        {
            printjobs();
        }


        while (token != NULL)
        {
            command_index++;
            token_count++;
            token = strtok(NULL, " ");
            commands[command_index] = token;
        }

        execute_command(commands[0], commands, token_count);

        clear_args(commands);
        command_index = 0;
        token_count = 0;
        free(line);
    }

}



void execute_command(char * command, char * arguments[], int token_count)
{
    int cpid, file_descriptor_in, file_descriptor_out, file_descriptor_err, sub_args_index;

    file_descriptor_in = -1;
    file_descriptor_out = -1;
    file_descriptor_err = -1;

    bool redirection = false;
    struct Job new_job;

    bool is_bg = false;
    sub_args_index = 1;

    char * sub_args[MAXINPUTS];
    sub_args[0] = command;              //adding command to subargs to help exec
    sub_args[1] = NULL;

    if (strcmp(arguments[token_count-1], "&") == 0)
    {
        is_bg = true;
        arguments[token_count-1] = NULL;
        token_count--;
    }

    for (int i = 1; i < token_count; i++)                                       //first run through of command, and arguments...
    {
        if (strcmp(arguments[i], "|") == 0)
        {
            char * command2 = arguments[i+1];
            file_redirection_piping(command, command2, arguments, token_count);
            //check for bg/fg
            return;
            }
        else if (strcmp(arguments[i], "<") == 0 && arguments[i+1] != NULL)              //single command redirection, up to >, <, 2>
        {
            file_descriptor_in = open(arguments[i+1], O_RDONLY, mode);
            redirection = true;
            i++;
        }
        else if (strcmp(arguments[i], ">") == 0 && arguments[i+1] != NULL)
        {
            file_descriptor_out = open(arguments[i+1], O_WRONLY | O_CREAT, mode);
            redirection = true;
            i++;
        }
        else if (strcmp(arguments[i], "2>") == 0 && arguments[i+1] != NULL)
        {
            file_descriptor_err = open(arguments[i+1], O_WRONLY | O_CREAT, mode);
            redirection = true;
            i++;
        }
        else
        {
            sub_args[sub_args_index] = arguments[i];
            sub_args[sub_args_index + 1] = NULL;
            sub_args_index++;
        }
    }



    if (redirection)
    {
        cpid = fork();
        if (cpid == 0)
        {
            if (file_descriptor_out)
            {
                dup2(file_descriptor_in, STDIN_FILENO);             //exec single command redirection
                close(file_descriptor_in);
            }
            if (file_descriptor_in)
            {
                dup2(file_descriptor_out, STDOUT_FILENO);
                close(file_descriptor_in);
            }
            if (file_descriptor_err)
            {
                dup2(file_descriptor_err, STDERR_FILENO);
                close(file_descriptor_err);
            }
            setpgid(0, 0);

            execvp(command, sub_args);
        }
        else
        {
            child_id = cpid;
            setpgid(cpid, cpid);
            //wait(NULL);
            pause();
        }

        clear_args(sub_args);
        return;
    }

    char new_jstr[MAXCHARS];
    for (int i = 0; arguments[i] != NULL; i++)
    {
        strcat(new_jstr, arguments[i]);
    }

    int status, x;

    cpid = fork();
    if (cpid == 0)
    {
        setpgid(0,0);
        execvp(command, arguments);
    }
    else if (is_bg)
    {
        add_job(cpid, new_jstr, Running, is_bg);
    }
    else
    {
        setpgid(cpid, cpid);
        child_id = cpid;
        //pause();
        x = waitpid(cpid, &status, WUNTRACED | WNOHANG );

        printf("%d\n", x);
        printf("%d\n", status);

        new_job.state = Done;
    }

    clear_args(sub_args);
    is_bg = false;
}

void file_redirection_piping(char * command, char * command2, char * arguments[], int token_count) {
    int pdf[2];
    int rpid, lpid, argument_index;
    bool is_bg;

    int file_descriptor_in = -1;
    int file_descriptor_out = -1;
    int file_descriptor_err = -1;

    argument_index = 0;
    char * left_arguments[MAXINPUTS];
    char * right_arguments[MAXINPUTS];

    int index = 0;
    for (index;
         strcmp(arguments[index], "|") != 0; index++)           //gets command options into two arrays for both commands
    {
        if (strcmp(arguments[index], "<") == 0 || strcmp(arguments[index], ">") == 0 || strcmp(arguments[index], "2>") == 0) {
            continue;
        }
        else
        {
            left_arguments[argument_index] = arguments[index];
            argument_index++;
            left_arguments[argument_index] = NULL;
        }
    }
    index++; //where pipe is, increment index
    argument_index = 0;

    while ((strcmp(arguments[index], ">") != 0) && (strcmp(arguments[index], "<") != 0) &&
    (strcmp(arguments[index], "2>") != 0))
    {
        right_arguments[argument_index] = arguments[index];
        argument_index++;
        right_arguments[argument_index] = NULL;
        index++;
        if (index == token_count)
            break;
    }


    if (pipe(pdf) == -1) {
        exit(EXIT_FAILURE);
    }

    int i = 0;
    for (i; strcmp(arguments[i], "|") != 0; i++)
    {
        if (strcmp(arguments[i], ">") == 0 && arguments[i+1] != NULL)
        {
            file_descriptor_out = open(arguments[i+1], O_WRONLY | O_CREAT, mode);
//            if (file_descriptor_out < 0)
//            {
//                perror("open output");
//                exit(EXIT_FAILURE);
//            }
        }
        else if (strcmp(arguments[i], "<") == 0 && arguments[i+1] != NULL)
        {
            file_descriptor_in = open(arguments[i+1], O_RDONLY | O_CREAT, mode);
            dup2(file_descriptor_in, STDIN_FILENO);
        }
        else if (strcmp(arguments[i], "2>") == 0 && arguments[i+1] != NULL)
        {
            file_descriptor_err = open(arguments[i+1], O_WRONLY | O_CREAT, mode);
            dup2(file_descriptor_err, STDERR_FILENO);
        }
    }

    if (strcmp(arguments[token_count-1], "&") == 0)
    {
        is_bg = true;
    }

    lpid = fork();
    if (lpid == 0)
    {
        if (file_descriptor_out > 0)
        {
            dup2(file_descriptor_out, STDOUT_FILENO);
            close(file_descriptor_out);
        }
        if (file_descriptor_in > 0)
        {
            dup2(file_descriptor_in, STDIN_FILENO);
            close(file_descriptor_out);
        }
        if (file_descriptor_err > 0) {
            dup2(file_descriptor_err, STDERR_FILENO);
            close(file_descriptor_err);
        }

        dup2(pdf[1], STDOUT_FILENO);
        close(pdf[0]);
        close(pdf[1]);
        setpgid(0, 0);
        execvp(command, left_arguments);
    }

    file_descriptor_err = -1;
    file_descriptor_out = -1;
    file_descriptor_in = -1;

    for (i; arguments[i] != NULL; i++)
    {
        if (strcmp(arguments[i], ">") == 0 && arguments[i+1] != NULL)
        {
            file_descriptor_out = creat(arguments[i+1], O_WRONLY);
//            if (file_descriptor_out < 0)
//            {
//                perror("open output");
//                exit(EXIT_FAILURE);
//            }
        }
        else if (strcmp(arguments[i], "<") == 0 && arguments[i+1] != NULL)
        {
            file_descriptor_in = open(arguments[i+1], O_RDONLY);
            dup2(file_descriptor_in, STDIN_FILENO);
        }
        else if (strcmp(arguments[i], "2>") == 0 && arguments[i+1] != NULL)
        {
            file_descriptor_err = creat(arguments[i+1], O_WRONLY);
            dup2(file_descriptor_err, STDERR_FILENO);
        }
    }

    rpid = fork();
    if (rpid == 0)
    {
        if (file_descriptor_out > 0)
        {
            dup2(file_descriptor_out, STDOUT_FILENO);
            close(file_descriptor_out);
        }
        if (file_descriptor_in > 0)
        {
            dup2(file_descriptor_in, STDIN_FILENO);
            close(file_descriptor_out);
        }
        if (file_descriptor_err > 0) {
            dup2(file_descriptor_err, STDERR_FILENO);
            close(file_descriptor_err);
        }

        dup2(pdf[0], STDIN_FILENO);
        close(pdf[1]);
        close(pdf[1]);
        setpgid(0, lpid);
        execvp(command2, right_arguments);
    }

    close(pdf[0]);
    close(pdf[1]);
    wait((int *)NULL);
    wait((int *)NULL);
    clear_args(left_arguments);
    clear_args(right_arguments);

}

void clear_args(char * arguments[])
{
    for (int i = 0; i < MAXINPUTS; i++)
    {
        arguments[i] = '\0';
    }
}

void printjobs()
{
    if (job_index == 0)
        return;
    for (int i = 0; jobs[i]->jstr != NULL; i++)
    {
        if (strcmp(jobs[i]->jstr, "Running") == 0)
        {
            printf("[%d] Running", jobs[i]->job_id);
        }
        else if (strcmp(jobs[i]->jstr, "Started") == 0)
        {
            printf("[%d] Started", jobs[i]->job_id);
        }
        else if (strcmp(jobs[i]->jstr, "Done") == 0)
        {
            printf("[%d] Done", jobs[i]->job_id);
            jobs[i] = NULL;
        }
    }
}

void sig_handler(int signumber)
{
    pid_t pid = tcgetpgrp(STDIN_FILENO);
    int status;
    switch(signumber)
    {
        case SIGINT:
            if (killpg(child_id, SIGINT) == 0)
            {

            }
            child_id = 0;
            break;
        case SIGTSTP:
            if (killpg(child_id, SIGTSTP) == 0)
            {

            }
            break;
        case SIGCHLD:
            while (waitpid(-1, NULL, WNOHANG | WUNTRACED) > 0);
            break;

    }
}

