/**
 * @author Aryan Agrawal
 * JC Shell is a program that imitates a bash shell
 * It takes upto five commands using pipe '|'
 * After executing each command it would print out the stats
 * of running the process. 
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

int maxChar = 1024;
int maxString = 30;
int maxCommands = 5;
int isSkip = 0;
pid_t currPid;

int start_process();

// handle wrong pipe cases (e.g. '||' or '| at the start or end')
void wrongpipe_handler(char *line)
{
    // handle extreme pipe cases (e.g. "| at the start or end")
    if (line[0] == '|' || line[strlen(line) - 2] == '|')
    {
        printf("| cannot be at the start or at the end\n");
        start_process();
    }

    // handle extreme pipe cases (e.g. "||")
    for (int i = 0; i < strlen(line) - 1; i++)
    {
        if (line[i] == '|' && line[i + 1] == '|')
        {
            printf("|| is not allowed\n");
            start_process();
        }
    }

    return;
}

// parse user input to get commands
void parse_commands(const char *line, int *numberOfCommands, int *numberOfPipes, char commands[][100])
{
    int start = 0; // start of command
    *numberOfCommands = 0;
    *numberOfPipes = 0;

    for (int i = 0; i < strlen(line); i++)
    {
        if (line[i] == '|')
        {
            (*numberOfCommands)++;
            strncpy(commands[*numberOfCommands - 1], line + start, i - start); // Copy the command to the array
            commands[*numberOfCommands - 1][i - start] = '\0';                 // Null-terminate the command
            (*numberOfPipes)++;
            start = i + 1; // start of next command
        }
    }

    // Copy the last command
    strncpy(commands[*numberOfCommands], line + start, strlen(line) - start);
    commands[*numberOfCommands][strlen(line) - start] = '\0';
    (*numberOfCommands)++;
}

void sigint_Handler(int sigint)
{
    // child process handles in default way, otherwise this
    //  JCshell process handling SIGINT
    printf("\n## JCshell [%d] ## ", currPid);
    fflush(stdout);
}

void sigusr_Handler(int sigusr1)
{
    sleep(0.5);
}

void getProcessStatistics(pid_t childPID, char *command, int status)
{

    int foo_int, pid, ppid, excode, vctx, nvctx;
    unsigned long user, sys, foo_long;
    char foo_char, state, statStr[50], statusStr[50], cmd[50], line[maxChar];
    FILE *statFile, *statusFile;

    siginfo_t processInfo;
    struct rusage usage;

    int ret = waitid(P_ALL, 0, &processInfo, WNOWAIT | WEXITED);
    if (!ret)
    {
        // opening /proc/{pid}/stat file
        sprintf(statStr, "/proc/%d/stat", processInfo.si_pid);
        statFile = fopen(statStr, "r");
        if (statFile == NULL)
        {
            printf("ERROR in opening /proc/%d/stat file\n", childPID);
            return;
        }

        // scanning /proc/{pid}/stat file
        fscanf(statFile, "%d %s %c %d %d %d %d %d %u %lu %lu %lu %lu %lu %lu",
               &pid, &foo_char, &state, &ppid, &foo_int, &foo_int, &foo_int, &foo_int,
               (unsigned *)&foo_int, &foo_long, &foo_long, &foo_long, &foo_long,
               &user, &sys);
        fclose(statFile);

        // opening /proc/{pid}/status file
        sprintf(statusStr, "/proc/%d/status", processInfo.si_pid);
        statusFile = fopen(statusStr, "r");
        if (statusFile == NULL)
        {
            printf("ERROR in opening /proc/%d/status file\n", childPID);
            return;
        }

        // scanning /proc{pid}/status file
        while (fgets(line, sizeof(line), statusFile) != NULL)
        {
            // read second last line
            sscanf(line, "voluntary_ctxt_switches: %d", &vctx);
            // read last line
            sscanf(line, "nonvoluntary_ctxt_switches: %d", &nvctx);
        }

        waitpid(processInfo.si_pid, &status, 0);
        // if normal exit
        if (WIFEXITED(status))
        {
            printf("\n(PID)%d (CMD)%s (STATE)%c (EXCODE)%d (PPID)%d (USER)%.2ld (SYS)%.2ld (VCTX)%d (NVCTX)%d\n",
                   pid, command, state, WEXITSTATUS(status), ppid, user, sys, vctx, nvctx);
            // if signal exit
        }
        else if (WIFSIGNALED(status))
        {
            int signum = WTERMSIG(status);
            printf("\n(PID)%d (CMD)%s (STATE)%c (EXSIG)%s (PPID)%d (USER)%.2ld (SYS)%.2ld (VCTX)%d (NVCTX)%d\n",
                   pid, command, state, sys_siglist[signum], ppid, user, sys, vctx, nvctx);
        }
    }
    else
    {
        perror("waitid");
    }
    return;
}

int start_process()
{
    signal(SIGINT, sigint_Handler);
    signal(SIGUSR1, sigusr_Handler);

    char line[maxChar];
    char *arguments[maxString];
    char *arguments2[maxString];
    char *arguments3[maxString];
    char *arguments4[maxString];
    char *arguments5[maxString];
    char commands[maxCommands][100]; // max 5 commands
    int numberOfCommands = 0, numberOfPipes = 0;
    int isWrongPipe = 0;

    // get user input
    currPid = getpid();
    printf("## JCshell [%d] ## ", getpid()); // print shell prompt
    fgets(line, 1025, stdin);

    wrongpipe_handler(line);                                           // handle wrong pipe cases
    parse_commands(line, &numberOfCommands, &numberOfPipes, commands); // parse user input to get commands

    // Tokenize the first command into arguments
    int argumentCount = 0, commandLength = strlen(commands[0]);
    char temp[100];
    for (int i = 0; i < commandLength; i++) // remove the trailing end character
        temp[i] = commands[0][i];
    if (temp[commandLength - 1] == '\n')
    {
        temp[commandLength - 1] = '\0'; // Replace newline character with null character
    }
    else
    {
        temp[commandLength] == '\0';
    }
    char *token = strtok(temp, " ");
    while (token != NULL && argumentCount < 30)
    {
        arguments[argumentCount++] = token;
        token = strtok(NULL, " ");
    }
    arguments[argumentCount] = NULL;

    // Tokenize the second command into arguments2
    if (numberOfCommands >= 2)
    {
        int argumentCount2 = 0, commandLength2 = strlen(commands[1]);
        char temp2[100];
        for (int i = 0; i < commandLength2; i++)
        {
            temp2[i] = commands[1][i];
        }
        if (temp2[commandLength2 - 1] == '\n')
        {
            temp2[commandLength2 - 1] = '\0'; // Replace newline character with null character
        }
        else
        {
            temp2[commandLength2] = '\0';
        }

        char *token2 = strtok(temp2, " ");
        while (token2 != NULL && argumentCount2 < 30)
        {
            arguments2[argumentCount2++] = token2;
            token2 = strtok(NULL, " ");
        }
        arguments2[argumentCount2] = NULL;
    }

    // Tokenize the third command into arguments3
    if (numberOfCommands >= 3)
    {

        int argumentCount3 = 0, commandLength3 = strlen(commands[2]);
        char temp3[100];
        for (int i = 0; i < commandLength3; i++)
        {
            temp3[i] = commands[2][i];
        }
        if (temp3[commandLength3 - 1] == '\n')
        {
            temp3[commandLength3 - 1] = '\0'; // Replace newline character with null character
        }
        else
        {
            temp3[commandLength3] = '\0';
        }

        char *token3 = strtok(temp3, " ");
        while (token3 != NULL && argumentCount3 < 30)
        {
            arguments3[argumentCount3++] = token3;
            token3 = strtok(NULL, " ");
        }
        arguments3[argumentCount3] = NULL;
    }

    // Tokenize the fourth command into arguments4
    if (numberOfCommands >= 4)
    {
        int argumentCount4 = 0, commandLength4 = strlen(commands[3]);
        char temp4[100];
        for (int i = 0; i < commandLength4; i++)
        {
            temp4[i] = commands[3][i];
        }
        if (temp4[commandLength4 - 1] == '\n')
        {
            temp4[commandLength4 - 1] = '\0'; // Replace newline character with null character
        }
        else
        {
            temp4[commandLength4] = '\0';
        }

        char *token4 = strtok(temp4, " ");
        while (token4 != NULL && argumentCount4 < 30)
        {
            arguments4[argumentCount4++] = token4;
            token4 = strtok(NULL, " ");
        }
        arguments4[argumentCount4] = NULL;
    }

    // Tokenize the fifth command into arguments5
    if (numberOfCommands == 5)
    {
        int argumentCount5 = 0, commandLength5 = strlen(commands[4]);
        char temp5[100];
        for (int i = 0; i < commandLength5; i++)
        {
            temp5[i] = commands[4][i];
        }
        if (temp5[commandLength5 - 1] == '\n')
        {
            temp5[commandLength5 - 1] = '\0'; // Replace newline character with null character
        }
        else
        {
            temp5[commandLength5] = '\0';
        }

        char *token5 = strtok(temp5, " ");
        while (token5 != NULL && argumentCount5 < 30)
        {
            arguments5[argumentCount5++] = token5;
            token5 = strtok(NULL, " ");
        }
        arguments5[argumentCount5] = NULL;
    }
    // exit handling
    if ((numberOfCommands > 1 || argumentCount > 1) && strcmp(arguments[0], "exit") == 0)
    {
        isSkip = 1;
        printf("exit with extra arguments!!!\n");
    }
    else if (strcmp(arguments[0], "exit") == 0)
    {
        isSkip = 1;
        kill(0, SIGTERM);
        exit(0);
    }

    // Create a child process to execute the command
    // when there is just one command
    if (numberOfCommands == 1 && isSkip == 0)
    {
        // signal(SIGINT, sigint_Handler);

        pid_t pid = fork();

        if (pid < 0)
        {
            printf("Fork failed");
        }
        else if (pid == 0) // Child Process
        {
            signal(SIGUSR1, sigusr_Handler);
            signal(SIGINT, SIG_DFL); // child command handles with default behaviour
            int execReturn = execvp(arguments[0], arguments);
            if (execReturn == -1)
            {
                perror("execvp"); // Print error if execvp fails
            }
        }
        else
        {
            // wait(NULL);
            signal(SIGUSR1, sigusr_Handler);
            signal(SIGINT, sigint_Handler);

            int childStatus;
            siginfo_t processInfo;
            kill(pid, SIGUSR1);
            // waitpid(processInfo.si_pid, &childStatus, 0);

            // Wait for a short duration to ensure the files are available
            // usleep(10000);
            getProcessStatistics(pid, commands[0], childStatus);
        }
    }

    // one pipe
    if (numberOfCommands == 2 && isSkip == 0)
    {
        int fd[2];
        pipe(fd);
        if (pipe(fd) == -1)
        {
            printf("Pipe Failed. Try Again");
            return 0;
        }

        pid_t pid1;
        pid_t pid2;

        pid1 = fork();
        if (pid1 < 0)
        {
            printf("Fork failed");
        }
        else if (pid1 == 0) // Child Process 1
        {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
            signal(SIGUSR1, sigusr_Handler);
            signal(SIGINT, SIG_DFL); // child command handles with default behaviour
            int execReturn = execvp(arguments[0], arguments);
            if (execReturn == -1)
            {
                perror("execvp"); // Print error if execvp fails
            }
        }
        else
        {
            pid2 = fork();
            if (pid2 < 0)
            {
                printf("Fork failed");
            }
            else if (pid2 == 0) // Child Process 2
            {
                dup2(fd[0], STDIN_FILENO);
                close(fd[0]);
                close(fd[1]);
                signal(SIGUSR1, sigusr_Handler);
                signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                int execReturn = execvp(arguments2[0], arguments2);
                if (execReturn == -1)
                {
                    perror("execvp"); // Print error if execvp fails
                }
            }
        }
        close(fd[0]);
        close(fd[1]);

        signal(SIGUSR1, sigusr_Handler);
        signal(SIGINT, sigint_Handler);

        int childStatus1, childStatus2;
        kill(pid1, SIGUSR1);
        kill(pid2, SIGUSR1);

        getProcessStatistics(pid1, commands[0], childStatus1);
        getProcessStatistics(pid2, commands[1], childStatus2);
    }

    // two pipes
    if (numberOfCommands == 3 && isSkip == 0)
    {
        int fd[4];
        pipe(fd);
        pipe(fd + 2);
        if (pipe(fd) == -1 || pipe(fd + 2) == -1)
        {
            printf("Pipe Failed. Try Again");
            return 0;
        }

        pid_t pid1;
        pid_t pid2;
        pid_t pid3;

        pid1 = fork();
        if (pid1 < 0)
        {
            printf("Fork failed");
        }
        else if (pid1 == 0) // Child Process 1
        {
            close(fd[2]);
            close(fd[3]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);

            signal(SIGUSR1, sigusr_Handler);
            signal(SIGINT, SIG_DFL); // child command handles with default behaviour
            int execReturn = execvp(arguments[0], arguments);
            if (execReturn == -1)
            {
                perror("execvp"); // Print error if execvp fails
            }
        }
        else
        {
            pid2 = fork();
            if (pid2 < 0)
            {
                printf("Fork failed");
            }
            else if (pid2 == 0) // Child Process 2
            {
                close(fd[1]);
                dup2(fd[0], STDIN_FILENO);
                close(fd[2]);
                dup2(fd[3], STDOUT_FILENO);
                close(fd[0]);
                close(fd[3]);

                signal(SIGUSR1, sigusr_Handler);
                signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                int execReturn = execvp(arguments2[0], arguments2);
                if (execReturn == -1)
                {
                    perror("execvp"); // Print error if execvp fails
                }
            }
            else
            {
                close(fd[0]);
                close(fd[1]);
                pid3 = fork();
                if (pid3 < 0)
                {
                    printf("Fork failed");
                }
                else if (pid3 == 0) // Child Process 3
                {
                    close(fd[3]);
                    dup2(fd[2], STDIN_FILENO);
                    close(fd[2]);

                    signal(SIGUSR1, sigusr_Handler);
                    signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                    int execReturn = execvp(arguments3[0], arguments3);
                    if (execReturn == -1)
                    {
                        perror("execvp"); // Print error if execvp fails
                    }
                }
            }
        }

        close(fd[0]);
        close(fd[1]);
        close(fd[2]);
        close(fd[3]);
        signal(SIGUSR1, sigusr_Handler);
        signal(SIGINT, sigint_Handler);

        int childStatus1, childStatus2, childStatus3;
        kill(pid1, SIGUSR1);
        kill(pid2, SIGUSR1);
        kill(pid3, SIGUSR1);

        getProcessStatistics(pid1, commands[0], childStatus1);
        getProcessStatistics(pid2, commands[1], childStatus2);
        getProcessStatistics(pid3, commands[2], childStatus3);
    }

    // three pipes
    if (numberOfCommands == 4 && isSkip == 0)
    {
        int fd[6];
        pipe(fd);
        pipe(fd + 2);
        pipe(fd + 4);
        if (pipe(fd) == -1 || pipe(fd + 2) == -1 || pipe(fd + 4) == -1)
        {
            printf("Pipe Failed. Try Again");
            return 0;
        }

        pid_t pid1;
        pid_t pid2;
        pid_t pid3;
        pid_t pid4;

        pid1 = fork();
        if (pid1 < 0)
        {
            printf("Fork failed");
        }
        else if (pid1 == 0) // Child Process 1
        {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
            close(fd[2]);
            close(fd[3]);
            close(fd[4]);
            close(fd[5]);
            signal(SIGUSR1, sigusr_Handler);
            signal(SIGINT, SIG_DFL); // child command handles with default behaviour
            int execReturn = execvp(arguments[0], arguments);
            if (execReturn == -1)
            {
                perror("execvp"); // Print error if execvp fails
            }
        }
        else
        {
            pid2 = fork();
            if (pid2 < 0)
            {
                printf("Fork failed");
            }
            else if (pid2 == 0) // Child Process 2
            {
                dup2(fd[0], STDIN_FILENO);
                dup2(fd[3], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                close(fd[2]);
                close(fd[3]);
                close(fd[4]);
                close(fd[5]);
                signal(SIGUSR1, sigusr_Handler);
                signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                int execReturn = execvp(arguments2[0], arguments2);
                if (execReturn == -1)
                {
                    perror("execvp"); // Print error if execvp fails
                }
            }
            else
            {
                pid3 = fork();
                if (pid3 < 0)
                {
                    printf("Fork failed");
                }
                else if (pid3 == 0) // Child Process 3
                {
                    dup2(fd[2], STDIN_FILENO);
                    dup2(fd[5], STDOUT_FILENO);
                    close(fd[0]);
                    close(fd[1]);
                    close(fd[2]);
                    close(fd[3]);
                    close(fd[4]);
                    close(fd[5]);
                    signal(SIGUSR1, sigusr_Handler);
                    signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                    int execReturn = execvp(arguments3[0], arguments3);
                    if (execReturn == -1)
                    {
                        perror("execvp"); // Print error if execvp fails
                    }
                }

                else
                {
                    pid4 = fork();
                    if (pid4 < 0)
                    {
                        printf("Fork failed");
                    }
                    else if (pid4 == 0) // Child Process 4
                    {
                        dup2(fd[4], STDIN_FILENO);
                        close(fd[0]);
                        close(fd[1]);
                        close(fd[2]);
                        close(fd[3]);
                        close(fd[4]);
                        close(fd[5]);
                        signal(SIGUSR1, sigusr_Handler);
                        signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                        int execReturn = execvp(arguments4[0], arguments4);
                        if (execReturn == -1)
                        {
                            perror("execvp"); // Print error if execvp fails
                        }
                    }
                }
            }
        }

        close(fd[0]);
        close(fd[1]);
        close(fd[2]);
        close(fd[3]);
        close(fd[4]);
        close(fd[5]);
        signal(SIGUSR1, sigusr_Handler);
        signal(SIGINT, sigint_Handler);

        int childStatus1, childStatus2, childStatus3, childStatus4;
        kill(pid1, SIGUSR1);
        kill(pid2, SIGUSR1);
        kill(pid3, SIGUSR1);
        kill(pid4, SIGUSR1);

        getProcessStatistics(pid1, commands[0], childStatus1);
        getProcessStatistics(pid2, commands[1], childStatus2);
        getProcessStatistics(pid3, commands[2], childStatus3);
        getProcessStatistics(pid4, commands[3], childStatus4);
    }

    // four pipes
    if (numberOfCommands == 5 && isSkip == 0)
    {
        int fd[8];
        pipe(fd);
        pipe(fd + 2);
        pipe(fd + 4);
        pipe(fd + 6);
        if (pipe(fd) == -1 || pipe(fd + 2) == -1 || pipe(fd + 4) == -1 || pipe(fd + 6) == -1)
        {
            printf("Pipe Failed. Try Again");
            return 0;
        }

        pid_t pid1;
        pid_t pid2;
        pid_t pid3;
        pid_t pid4;
        pid_t pid5;

        pid1 = fork();
        if (pid1 < 0)
        {
            printf("Fork failed");
        }
        else if (pid1 == 0) // Child Process 1
        {
            dup2(fd[1], STDOUT_FILENO);
            close(fd[0]);
            close(fd[1]);
            close(fd[2]);
            close(fd[3]);
            close(fd[4]);
            close(fd[5]);
            close(fd[6]);
            close(fd[7]);
            signal(SIGUSR1, sigusr_Handler);
            signal(SIGINT, SIG_DFL); // child command handles with default behaviour
            int execReturn = execvp(arguments[0], arguments);
            if (execReturn == -1)
            {
                perror("execvp"); // Print error if execvp fails
            }
        }
        else
        {
            pid2 = fork();
            if (pid2 < 0)
            {
                printf("Fork failed");
            }
            else if (pid2 == 0) // Child Process 2
            {
                dup2(fd[0], STDIN_FILENO);
                dup2(fd[3], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
                close(fd[2]);
                close(fd[3]);
                close(fd[4]);
                close(fd[5]);
                close(fd[6]);
                close(fd[7]);
                signal(SIGUSR1, sigusr_Handler);
                signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                int execReturn = execvp(arguments2[0], arguments2);
                if (execReturn == -1)
                {
                    perror("execvp"); // Print error if execvp fails
                }
            }
            else
            {
                pid3 = fork();
                if (pid3 < 0)
                {
                    printf("Fork failed");
                }
                else if (pid3 == 0) // Child Process 3
                {
                    dup2(fd[2], STDIN_FILENO);
                    dup2(fd[5], STDOUT_FILENO);
                    close(fd[0]);
                    close(fd[1]);
                    close(fd[2]);
                    close(fd[3]);
                    close(fd[4]);
                    close(fd[5]);
                    close(fd[6]);
                    close(fd[7]);
                    signal(SIGUSR1, sigusr_Handler);
                    signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                    int execReturn = execvp(arguments3[0], arguments3);
                    if (execReturn == -1)
                    {
                        perror("execvp"); // Print error if execvp fails
                    }
                }

                else
                {
                    pid4 = fork();
                    if (pid4 < 0)
                    {
                        printf("Fork failed");
                    }
                    else if (pid4 == 0) // Child Process 4
                    {
                        dup2(fd[4], STDIN_FILENO);
                        dup2(fd[7], STDOUT_FILENO);
                        close(fd[0]);
                        close(fd[1]);
                        close(fd[2]);
                        close(fd[3]);
                        close(fd[4]);
                        close(fd[5]);
                        close(fd[6]);
                        close(fd[7]);
                        signal(SIGUSR1, sigusr_Handler);
                        signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                        int execReturn = execvp(arguments4[0], arguments4);
                        if (execReturn == -1)
                        {
                            perror("execvp"); // Print error if execvp fails
                        }
                    }
                    else
                    {

                        pid5 = fork();
                        if (pid5 < 0)
                        {
                            printf("Fork failed");
                        }
                        else if (pid5 == 0) // Child Process 5
                        {
                            dup2(fd[6], STDIN_FILENO);
                            close(fd[0]);
                            close(fd[1]);
                            close(fd[2]);
                            close(fd[3]);
                            close(fd[4]);
                            close(fd[5]);
                            close(fd[6]);
                            close(fd[7]);
                            signal(SIGUSR1, sigusr_Handler);
                            signal(SIGINT, SIG_DFL); // child command handles with default behaviour
                            int execReturn = execvp(arguments5[0], arguments5);
                            if (execReturn == -1)
                            {
                                perror("execvp"); // Print error if execvp fails
                            }
                        }
                    }
                }
            }
        }

        close(fd[0]);
        close(fd[1]);
        close(fd[2]);
        close(fd[3]);
        close(fd[4]);
        close(fd[5]);
        close(fd[6]);
        close(fd[7]);
        signal(SIGUSR1, sigusr_Handler);
        signal(SIGINT, sigint_Handler);

        int childStatus1, childStatus2, childStatus3, childStatus4, childStatus5;
        kill(pid1, SIGUSR1);
        kill(pid2, SIGUSR1);
        kill(pid3, SIGUSR1);
        kill(pid4, SIGUSR1);
        kill(pid5, SIGUSR1);

        getProcessStatistics(pid1, commands[0], childStatus1);
        getProcessStatistics(pid2, commands[1], childStatus2);
        getProcessStatistics(pid3, commands[2], childStatus3);
        getProcessStatistics(pid4, commands[3], childStatus4);
        getProcessStatistics(pid5, commands[4], childStatus5);
    }

    // if more than 4 pipes (i.e more than 5 commands)
    if (numberOfCommands > 5 && isSkip == 0)
    {
        printf("JCshell cannot accept more than 5 commands!\n");
    }
}

int main()
{
    while (1)
    {
        start_process();
        isSkip = 0;
    }
}
