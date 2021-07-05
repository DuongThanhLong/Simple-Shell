#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#define pipeDirect "!<>"
#define MAX_LENGTH 80
char historyBuffer[MAX_LENGTH];

//String parse with space
void stringParse(char **args, char command[80])
{
    args[0] = strtok(command, " ");
    int i = 0;
    while (args[i] != NULL)
    {
        args[++i] = strtok(NULL, " ");
    }
}

// function count the number of character in parse
int numOfParse(char **parse)
{
    int count = 1;
    while (parse[count] != NULL)
    {
        ++count;
    }
    return count;
}
// function delete white space in head and tail of str
// delete the last character in str
void deleteWhiteSpace_Character(char *str, char c)
{
    int i;
    int begin = 0, end = strlen(str) - 1;
    //char c = '\0';
    if (c == '\0')
    {
        while ((str[begin] == ' ' || str[begin] == '\n' || str[begin] == '\t' || str[begin] == '\a' || str[begin] == '\r') && (begin < end))
        {
            begin++;
        }
        while ((end >= begin) && (str[end] == ' ' || str[end] == '\n' || str[end] == '\t' || str[end] == '\a' || str[end] == '\r'))
        {
            end--;
        }
    }
    else
    {
        end--; // if you want to erase the letter c of ending string
    }
    for (i = begin; i <= end; i++)
    {
        str[i - begin] = str[i];
    }
    str[i - begin] = '\0';
}

// function split str into 2 side: left and right
int parsePipe(char *args[], char *args_left[], char *args_right[])
{
    int idx = 0;
    int count = numOfParse(args);
    while (args[idx] != NULL)
    {
        if (strcmp(args[idx], "|") == 0)
        {
            int i = 0;
            for (i; i < idx; i++)
            {
                args_left[i] = strdup(args[i]);
            }
            args_left[i] = NULL;
            int z = 0;
            int j = idx + 1;
            for (j; j < count; j++)
            {
                args_right[z] = strdup(args[j]);
                z++;
            }
            args_right[j] = NULL;
            return 1;
        }
        idx++;
    }
    return 0;
}

//Parse redirect
void redirectParse(char *args[], char *args_redir[])
{
    args_redir[0] = NULL;
    args_redir[1] = NULL;
    int idx = 0;
    while (args[idx] != NULL)
    {
        if (strcmp(args[idx], ">") == 0 || strcmp(args[idx], "<") == 0)
        {
            args_redir[0] = strdup(args[idx]);
            args_redir[1] = strdup(args[idx + 1]);
            args[idx] = NULL;
            args[idx + 1] = NULL;
            return;
        }
        idx++;
    }
    return;
}

// function check whether sign & is in ending str or not
bool isAmpersand(char **parse)
{
    bool ans = false;
    int n = numOfParse(parse); // the number character of parse
    if (strcmp(parse[n - 1], "&") == 0)
    {
        ans = true;
        parse[n - 1] = NULL; // delete & becasue shell does not understand this
    }

    else
    {
        // prevent user enter the sticky command
        int len = strlen(parse[n - 1]);
        char tmp = parse[n - 1][len - 1];
        if (tmp == '&')
        {
            ans = true;
            deleteWhiteSpace_Character(parse[n - 1], tmp);
        }
    }
    return ans;
}

//Check type of command ("exit","history", "real command")
int checkcommand(char command[80])
{
    // if user's input is exit
    if (strcmp(command, "quit") == 0 || strcmp(command, "exit") == 0)
    {
        return -1;
    }

    // if user's input is exec again("History")
    if (strcmp(command, "!!") == 0)
    {
        return 1;
    }

    //Real command
    return 0;
}

//Proccess command
void proc(char **args)
{
    bool isAmp = isAmpersand(args);                         //Flag check if exists "&"
    char *args_redir[2];                                    //buffer for redirect
    char *args_left[MAX_LENGTH / 2 + 1];                    // buffer for pipe write
    char *args_right[MAX_LENGTH / 2 + 1];                   // buffer for pipe read
    int checkPipe = parsePipe(args, args_left, args_right); //flag check if pipe exists

    //if command is empty
    if (args[0] == NULL)
    {
        printf("Hay nhap vao mot lenh\n");
        return;
    }

    //if pipe exists
    if (checkPipe)
    {
        // 0 is read only and 1 is write only
        int pipefd[2];
        pid_t p1, p2;

        if (pipe(pipefd) < 0)
        {
            printf("\nPipe could not be initialized");
            return;
        }
        p1 = fork();
        if (p1 < 0)
        {
            printf("\nFork error");
            return;
        }

        if (p1 == 0)
        {
            // Child 1
            // Write only
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);

            if (execvp(args_left[0], args_left) < 0)
            {
                printf("\nCommand is not existed");
                exit(0);
            }
        }
        else
        {
            // Parent
            p2 = fork();

            if (p2 < 0)
            {
                printf("\nFork error");
                return;
            }

            // Child 2
            // Read only
            if (p2 == 0)
            {
                close(pipefd[1]);
                dup2(pipefd[0], STDIN_FILENO);
                close(pipefd[0]);
                if (execvp(args_right[0], args_right) < 0)
                {
                    printf("\nCommand is not existed");
                    exit(0);
                }
            }
            else
            {
                // Parent close all discriptors and wait for children
                close(pipefd[0]);
                close(pipefd[1]);
                wait(NULL);
                wait(NULL);
            }
        }
    }

    // If pipe is not existed
    else
    {

        redirectParse(args, args_redir);
        pid_t id = fork();
        if (id == 0)
        {
            // Check if Redirect is existed
            if (args_redir[0] != NULL)
            {
                // If redirect ouput
                if (strcmp(args_redir[0], ">") == 0)
                {
                    int fd = open(args_redir[1], O_WRONLY);
                    if (fd == -1)
                        fd = creat(args_redir[1], S_IRWXU);
                    close(STDOUT_FILENO);
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }

                // If redirect input
                if (strcmp(args_redir[0], "<") == 0)
                {
                    int fd = open(args_redir[1], O_RDONLY);
                    if (fd == -1)
                    {
                        printf("%s not found\n", args_redir[1]);
                        return;
                    }
                    close(STDIN_FILENO);
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
            }

            // Run command for either normal command or redirection
            if (execvp(args[0], args))
            {
                printf("This command is not existed\n");
                exit(0);
            }
            exit(0);
        }
        else
        {
            //proccess parent
            //Check if flag "&" is true
            if (!isAmp)
                wait(NULL);
        }
        return;
    }
}

int main(void)
{
    int should_run = 1;
    char *args[MAX_LENGTH / 2 + 1]; // Buffer for command
    char command[80];               // buffer for user input
    while (should_run)
    {
        printf("\033[0;32m");
        printf("osh>");
        printf("\033[0m");
        fflush(stdout);
        fgets(command, MAX_LENGTH, stdin);
        command[strlen(command) - 1] = '\0';
        int check = checkcommand(command);
        // User's input is exit
        if (check == -1)
        {
            exit(0);
        }

        // User's input is "history"
        if (check == 1)
        {
            if (strlen(historyBuffer) == 0)
            {
                printf("No command in history\n");
            }
            else
            {
                strcpy(command, historyBuffer);
                printf("Echo: %s\n", historyBuffer);
                stringParse(args, command);
                proc(args);
            }
            continue;
        }

        //User's input is "real command"
        if (check == 0)
        {
            strcpy(historyBuffer, command);
            stringParse(args, command);
            proc(args);
            continue;
        }
    }
}