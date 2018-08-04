#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#define LIMIT 4096

/*
* BOOT
*
* Boot() reads the profile file and sets the HOME and PATH environment
* variables accordingly, returning to a 'clean' state when finished.
*/
void boot(char *working_directory, char *path, char *home) {
	char *buffer = working_directory;
	strcat(buffer, "/profile");
	FILE *f = fopen(buffer, "r");
	if (f == NULL) {
		fputs("Cannot find profile.\n", stderr);
		exit(EXIT_FAILURE);
	}
	fseek(f, 0L, SEEK_END);
	int l = ftell(f);
	rewind(f);
	char *content = calloc(l, sizeof(char));
	while (fgets(content, l, f) != NULL) {
		if (!strncmp(content, "PATH=", 5)) {
			strcpy(path, content + 5);
			path[strlen(path) - 1] = 0;
			setenv("PATH", path, 1);
		}
		if (!strncmp(content, "HOME=", 5)) {
			strcpy(home, content + 5);
			home[strlen(home) - 1] = 0;
			setenv("HOME", home, 1);
		}
	}
	fclose(f);
	free(content);
	working_directory[strlen(working_directory) - 8] = '\0';
	setenv("PWD", working_directory, 1);
}

/*
* SCAN
*
* Scan() reads user input for cases other than PATH/HOME assignment
* and cd (see main()). It stores the input as an array of arguments.
*/
void scan(char *line, char **arguments) {
	while (*line != '\0') {
		while (*line == ' ') {
			*line++ = '\0';
		}
		*arguments++ = line;
		while (*line != ' ' && *line != '\0') {
			line++;
		}
	}
	*arguments = "";
}

/*
* RUN
*
* Run() attempts to fork and execute programs, taking in the
* arguments stored by scan(). The first element of the array is
* the name of the program. The command 'exit' is also included for
* ease of use.
*/
void run(char **arguments) {
	if (!strncmp(arguments[0], "exit", 4)) {
		exit(EXIT_SUCCESS);
	}
	pid_t controller;
	int status;
	if ((controller = fork()) < 0) {
		fputs("Cannot fork process\n", stderr);
		exit(EXIT_FAILURE);
	}
	else if (controller == 0) {
		if (execvp(*arguments, arguments) < 0) {
			perror("Cannot execute program");
			exit(EXIT_FAILURE);
		}
	}
	else {
		while (wait(&status) != controller) {
			;
		}
	}
}

/*
* MAIN
*
* First, main() initialises required variables and allocates memory.
* Second, it runs boot(). Next, main() creates the shell prompt for
* the user. Lastly, it parses input: intitally, it looks for PATH and
* HOME assignments, as well as calls to cd; if none are found it runs
* scan(), then run().
*
* Note that PATH assignments are appended to the existing path as per
* convention.
*/
int main(int length, char *input[]) {
	char *working_directory = getenv("PWD");
	char *path = (char*)calloc(LIMIT, sizeof(char));
	char *home = (char*)calloc(LIMIT, sizeof(char));
	char line[LIMIT];
	char *arguments[LIMIT];
	boot(working_directory, path, home);
 	while (1) {
		printf("%s>", working_directory);
		if (!fgets(line, LIMIT, stdin)) {
			break;
		}
		if (strlen(line) > 1) {
			line[strlen(line) - 1] = 0;
			if (!strncmp(line, "$PATH=", 6)) {
				if(!opendir(line + 6)) {
					fputs("No such file or directory.\n", stderr);
				}
				else {
					strcat(path, ":");
					strcat(path, line + 6);
					setenv("PATH", path, 1);
				}
			}
			else if (!strncmp(line, "$HOME=", 6)) {
				if(!opendir(line + 6)) {
					fputs("No such file or directory.\n", stderr);
				}
				else {
					strcpy(home, line + 6);
					setenv("HOME", home, 1);
				}
			}
			else if (!strncmp(line, "cd", 2)) {
				if (line[2] == ' ' && line[3] != 0) {
					if(!opendir(line + 3)) {
						fputs("No such file or directory.\n", stderr);
					}
					else {
						strcpy(working_directory, line + 3);
						chdir(working_directory);
						setenv("PWD", working_directory, 1);
					}
				}
				else if (line[2] == 0) {
					if(!opendir(home)) {
						fputs("No such file or directory.\n", stderr);
					}
					else {
						strcpy(working_directory, home);
						chdir(working_directory);
						setenv("PWD", working_directory, 1);
					}
				}
				else {
					fputs("Cannot parse input.\n", stderr);
				}
			}
			else {
				scan(line, arguments);
				run(arguments);
			}
		}
	}
}
