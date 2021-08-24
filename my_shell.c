#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64
int parallelProcess = 0;
pid_t pidarr[64];
pid_t bgArr[64];
int bgnum = 0;

/* Splits the string by space and returns the array of tokens */
char **tokenize(char *line, int *parallel, int *bg)
{
	char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
	char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
	int i, tokenIndex = 0, tokenNo = 0;

	for (i = 0; i < strlen(line); i++)
	{

		char readChar = line[i];

		if (readChar == ' ' || readChar == '\n' || readChar == '\t')
		{
			token[tokenIndex] = '\0';
			if (tokenIndex != 0)
			{
				tokens[tokenNo] = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0;
				if (strcmp(token, "&&&") == 0)
				{
					*parallel = 1;
				}
				if (strcmp(token, "&") == 0)
				{
					*bg = 1;
				}
			}
		}
		else
		{
			token[tokenIndex++] = readChar;
		}
	}

	free(token);
	tokens[tokenNo] = NULL;
	return tokens;
}

void pwd();
void handle_sigint(int sig)
{
    for (int i = 0; i < parallelProcess; i++)
	{
		kill(pidarr[i], SIGKILL);
		wait(0);
	} 
    printf("Caught signal %d\n", sig);
}


int execut(int, int, char *, char **, int);

void my_sleep(int);


void bgHandler()
{
	if (bgnum != 0)
	{
		int wstat;
		pid_t pid;
		pid = wait3(&wstat, WNOHANG, (struct rusage *)NULL);

		for (size_t i = 0; i < bgnum; i++)
		{
			if (bgArr[i] == pid)
			{
				bgnum--;
				printf("Shell: Background process finished \n");
				bgArr[i] = bgArr[bgnum];
				break;
			}
		}
	}
}


int main(int argc, char *argv[])

{
	int execut(int parallel, int bg, char *command, char **args, int size)
{
	pid_t pid, wpid;
	int status;
	int i;
	char *new_arg[size + 1];

	for (i = 0; i < size; i++)
	{
		new_arg[i] = args[i];
	}

	new_arg[size] = NULL;

	pid = fork();
    if(pid < 0) printf("Shell: Error in Fork\n");
	else if (pid == 0)
	{
        setpgid(0,0);
		if (execvp(command, new_arg)<0)
		{
			// printf("Shell: Incorrect command \n");
			exit(0);
		};
	}
	else if (pid)
	{
		if (parallel == 1)
		{ // if parallel
			pidarr[parallelProcess] = pid;
			parallelProcess++;
		}
		else if (bg== 1)
		{ // if background
			bgArr[bgnum] = pid;
			bgnum++;
		}
		if (parallel != 1 && bg != 1)
		{ // if blocking, reap it before going ahead
			wpid = waitpid(pid, &status, 0);
		}
        else if (pid < 0)
	{
		perror("error forking");
	}
	else
	{
		do
		{
			wpid = waitpid(pid, &status, WUNTRACED);
		} while (!WIFEXITED(status) && !WIFSIGNALED(status));
	}

	}
	return size;
}


	signal(SIGINT, handle_sigint);
	signal(SIGCHLD, bgHandler);

	char line[MAX_INPUT_SIZE];
	char **tokens;
	int i;
    

	FILE *fp;
	if (argc >= 2)
	{
		fp = fopen(argv[1], "r");
		if (fp < 0)
		{
			printf("File doesn't exists.");
			return -1;
		}
	}
	// int ex = 0;
	while (1)
	{
		int parallel = 0;
		int bg = 0;
		parallelProcess = 0;

		/* BEGIN: TAKING INPUT */
		bzero(line, sizeof(line));
		if (argc == 2)
		{ // batch mode
			if (fgets(line, sizeof(line), fp) == NULL)
			{ // file reading finished
				break;
			}
			line[strlen(line) - 1] = '\0';
		}
		else
		{ // interactive mode
			printf("$ ");
			scanf("%[^\n]", line);
			getchar();
		}
		/* END: TAKING INPUT */

		line[strlen(line)] = '\n'; //terminate with new line
		tokens = tokenize(line, &parallel, &bg);
		char *command[64];
		int commandLen = 0;
        
		for (i = 0; tokens[i] != NULL; i++)
		{
			if (strcmp(tokens[i], "&") == 0)
			{
				bg = 1;
				execut(parallel, bg, command[0], command, commandLen);
				commandLen = 0;
			}
			else if (strcmp(tokens[i], "&&") == 0)
			{
				execut(parallel, bg, command[0], command, commandLen);
				commandLen = 0;
			}
			
            else if (strcmp(tokens[i], "exit") == 0)
			{ 
				for (int i = 0; i < parallelProcess; i++){
								kill(pidarr[i], SIGKILL);
								wait(0);
							}
				for (int i = 0; i < bgnum; i++){
							kill(bgArr[i], SIGKILL);
							wait(0);
						}
				exit(0);
				break;
			}

			else if (strcmp(tokens[i], "cd") == 0)
			{ 
                char s[100];
                // printf("%s\n", getcwd(s, 100));
				chdir(tokens[i++]);
                 printf("%s\n",getcwd(s, 100));
			}
            // 
            else if (strcmp(tokens[i], "&&&") == 0)  //finally
			{
				parallel = 1;
				execut(parallel, bg, command[0], command, commandLen);
				commandLen = 0;
			}
			

			else
			{
				command[commandLen] = tokens[i];
				commandLen++;
			}
		}
		execut(parallel, bg, command[0], command, commandLen);

		// Freeing the allocated memory
		for (i = 0; tokens[i] != NULL; i++)
		{
			free(tokens[i]);
		}
		free(tokens);
        

		for (int i = 0; i < parallelProcess; i++)
		{
			pid_t wpid = wait(0);
			parallelProcess--;
			pidarr[i] = 0;
		}
	}
	return 0;
}
