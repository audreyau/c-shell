/******************************************************
** Program: smallsh
** Author: Audrey Au
** Date: 11/18/23
** Description: Simple shell with basic functionality
******************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>	// string
#include <signal.h>	// catchSIGINT, catchSIGTSTP
#include <unistd.h>	// fork, execvp, chdir, getpid
#include <errno.h>	// errors
#include <fcntl.h>	// file manipulation

#include <sys/stat.h>	// file status
#include <sys/types.h>	// datat types
#include <sys/wait.h>	// wait for process

// ************************ CONSTANTS / GLOBAL VARIABLES ************************
#define MAX_LENGTH 2048
#define MAX_ARGS 512

int allowBackgroundProcess = 1;
int backgroundProcess = 0;

int processes[1000];
int processStatus;

// ************************ PROTOTYPES ************************
void handleSignals (struct sigaction, struct sigaction);
void catchSIGINT (int);
void catchSIGTSTP ();

int prompt(char**, char*, char*, char*, int*, int*);

void displayProcessStatus ();
void changeDirectory (char** inputArgs);

void executeCommandline(char**, struct sigaction, struct sigaction, int);

// ************************ FUNCTIONS ************************
int main () {
	int loop = 1;
	int status = 0;
	int numArgs = 0;

	char* inputArgs[MAX_ARGS];
	char inputChars[MAX_LENGTH];

	char inputFile[300] = "";
	char outputFile[300] = "";

	struct sigaction SIGINTAction;
	struct sigaction SIGTSTPAction;

	handleSignals(SIGINTAction, SIGTSTPAction);
	
	while (loop == 1) {
		numArgs = prompt(inputArgs, inputChars, inputFile, outputFile, &backgroundProcess, &allowBackgroundProcess);
		inputArgs[numArgs] = NULL;

		// COMMENT
		if(inputArgs[0][0] == '#' || inputArgs[0][0] == '\n') {
			continue;
		}
		// EXIT
		else if(strcmp(inputArgs[0], "exit") == 0) {
			loop = 0;
		}
		// STATUS
		else if(strcmp(inputArgs[0], "status") == 0) {
			displayProcessStatus ();
		}
		// CD
		else if(strcmp(inputArgs[0], "cd") == 0) {
			changeDirectory (inputArgs);
		}
		// OTHER COMMANDS
		else {
			executeCommandline(inputArgs, SIGINTAction, SIGTSTPAction, numArgs);
		}
	}
	return 0;
}

// handle ctrl c and ctrl z
void handleSignals (struct sigaction SIGTSTPAction, struct sigaction SIGINTAction) {
	// handle ^c (from class code)
	SIGTSTPAction.sa_handler = catchSIGINT;
    sigfillset(&SIGINTAction.sa_mask);
    sigaction(SIGINT, &SIGINTAction, NULL);
	
	// handle ^z (from class code)
	SIGTSTPAction.sa_handler = catchSIGTSTP; 
    SIGTSTPAction.sa_flags = SA_RESTART;
    sigfillset(&SIGTSTPAction.sa_mask);	
    sigaction(SIGTSTP, &SIGTSTPAction, NULL);
}

// ctrl c --> terminated by signal
void catchSIGINT (int sigNum) {
	printf("terminated by signal %d\n", sigNum);
	fflush(stdout);
}

// ctrl z --> foreground
void catchSIGTSTP () {
	char* statusMessage;
	
	switch(allowBackgroundProcess) {
		case 0:
			statusMessage = "\nExiting foreground-only mode\n";
			write(1, statusMessage, 30);
			fflush(stdout);
			allowBackgroundProcess = 1;
			break;

		case 1:
			statusMessage = "\nEntering foreground-only mode (& is now ignored)\n";
			write(1, statusMessage, 50);
			fflush(stdout);
			allowBackgroundProcess = 0;
			break;

		default:
			statusMessage = "\nError: allowBackgroundProcess is not 0 or 1\n";
			write(1, statusMessage, 30);
			fflush(stdout);
			allowBackgroundProcess = 1;
			break;
	}
	
	char* colon = ": ";
	write(STDOUT_FILENO, colon, 2);
}

// prompt user and get input
int prompt(char** inputArgs, char* inputChars, char* inputFile, char* outputFile, int* backgroundProcess, int* allowBackgroundProcess) {
	int tmp = 0;
	char tmpString[MAX_LENGTH];

	printf(": ");
	fflush(stdout);
	fgets(inputChars, MAX_LENGTH, stdin);

	// replace new lines with null terminator
	inputChars[strcspn(inputChars, "\n")] = '\0';

	// for edge case where input is an empty string
	if (inputChars[0] == '\0') {
		inputArgs[0] = strdup("");
	}

	// translate input into strings
	const char* space = " ";
	char* token = strtok(inputChars, space);

	while (token != NULL) {
		inputArgs[tmp] = token;

		// expand dollars
		for (int i = 1; inputArgs[tmp][i]; i++) {
			if(inputArgs[tmp][i] == '$' && inputArgs[tmp][i-1] == '$') {
				inputArgs[tmp][i] = '\0';
				inputArgs[tmp][i-1] = '\0';

				snprintf(tmpString, MAX_LENGTH, "%s%d", inputArgs[tmp], getpid());
				inputArgs[tmp] = tmpString;
			}
		}

		token = strtok(NULL, space);
		tmp++;
	}

	return tmp;
}

// show status of pid
void displayProcessStatus () {
    waitpid(getpid(), &processStatus, 0);

	if (WIFSIGNALED(processStatus)) {
		printf("terminated by signal %d\n", WTERMSIG(processStatus));
	} else if (WIFEXITED(processStatus)) {
		printf("exit value %d\n", WEXITSTATUS(processStatus));
	}

	fflush(stdout);
}

// change directories
void changeDirectory (char** inputArgs) {
	if (inputArgs[1] != '\0') {
		if (chdir(inputArgs[1]) == -1) {
			printf("no such file or directory.\n");
			fflush(stdout);
		}
	}
	else {
		chdir(getenv("HOME"));
	}
}

// all others including fork
void executeCommandline(char** inputArgs, struct sigaction SIGTSTPAction, struct sigaction SIGINTAction, int numArgs) {
    char inputFile[MAX_LENGTH];
	char outputFile[MAX_LENGTH];
	
    int flag = 0;
	int in = 0;
	int out = 0;
	int input = 0;

    if (strcmp(inputArgs[numArgs - 1], "&") == 0) {
        if (allowBackgroundProcess == 1)
            flag = 1;

        inputArgs[numArgs - 1] = NULL;
    }

    pid_t spawnPid = fork();

    switch (spawnPid) {
        case -1:	// unable to duplicate process
            perror("Hull has been breached.\n");
            exit(EXIT_FAILURE);
            break;

        case 0:		// clone (not original)
            for (int i = 0; inputArgs[i] != NULL; i++) {
                if (strcmp(inputArgs[i], "<") == 0) {
                    inputArgs[i] = NULL;
                    strcpy(inputFile, inputArgs[i + 1]);
                    i++;

					in = 1;
                } 
				else if (strcmp(inputArgs[i], ">") == 0) {
                    inputArgs[i] = NULL;
                    strcpy(outputFile, inputArgs[i + 1]);
                    i++;

					out = 1;
                }
            }

            // input file
            if (in == 1) {
                int input = open(inputFile, O_RDONLY);
                if (input < 0) {
                    fprintf(stderr, "error: cannot open %s for input\n", inputFile);
                    exit(EXIT_FAILURE);
                }
                dup2(input, 0);
                close(input);
            }

            // output file
            if (out == 1) {
                int input = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (input < 0) {
                    fprintf(stderr, "error: cannot open %s for output\n", outputFile);
                    exit(EXIT_FAILURE);
                }
                dup2(input, 1);
                close(input);
            }

            if (!flag)
                SIGINTAction.sa_handler = SIG_DFL;
            sigaction(SIGINT, &SIGINTAction, NULL);

            if (execvp(inputArgs[0], inputArgs) == -1) {
                perror("error");
                exit(EXIT_FAILURE);
            }

			else {
				input = open("/dev/null", O_RDONLY);
				
				// print error message if cannot open
				if (input == -1) {
					fprintf(stderr, "cannot open /dev/null for input\n");
					exit(1);
				}

				if (dup2(input, 0) == -1) {
					fprintf(stderr, "cannot open /dev/null for input\n");
					exit(1);
				}
			}
		
            break;

        default:	// original parent
            if (flag) {
                waitpid(spawnPid, &processStatus, WNOHANG);
                printf("background pid is %d\n", spawnPid);
                fflush(stdout);
            } 
			else {
                waitpid(spawnPid, &processStatus, 0);
            }
            break;
    }

    // wait 
    while ((spawnPid = waitpid(-1, &processStatus, WNOHANG)) > 0) {
        printf("background pid %d is done: ", spawnPid);
        if (WIFEXITED(processStatus)) {
            printf("exit value %d\n", WEXITSTATUS(processStatus));
        } 
		else {
            printf("terminated by signal %d\n", WTERMSIG(processStatus));
        }
        fflush(stdout);
    }
}