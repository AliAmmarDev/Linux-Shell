#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>

#define CYAN "\x1b[96m"
#define NONE "\033[m"
static char* currentDirectory;

void removeEndOfLine(char line[]);
void readLine(char line[]);
int parseLine(char* args[], char line[]);
int processLine(char* args[], char line[]);
void info(char* args[]);
void cd(char* args[]);
void exitShell(char* args[]);
void pwd(char* args[]);
void clear(char * args[]);
void removeDirectory(char* args[]);
void makeDirectory(char* args[]);
void launchProgram(char **args, int background);
void pipeHandler(char * args[]);
void redirectOutput(char * args[], char* outputFile);
int executeCommands(char* args[]);
void printShell();

void removeEndOfLine(char line[]){
  int i = 0;
  while(line[i] != '\n'){
    i++;
  }
  line[i] = '\0';
}

void readLine(char line[]){
  fgets(line, 100, stdin);
  removeEndOfLine(line);
}

int parseLine(char* args[], char line[]){
  readLine(line);
  processLine(args, line);
  return 1;
}

int processLine(char* args[], char line[]){
  int i = 0;
  args[i] = strtok(line, " ");
  if(args[i] == NULL){
    return 1;
  }
  while(args[i] != NULL){
    i++;
    args[i] = strtok(NULL, " ");
  }
}

void info(char* args[]){
  printf("COMP2211 Simplified shell by SC16AA\n");
}

void exitShell(char* args[]){
  exit(0);
}

void pwd(char* args[]){
  char* temp =getcwd(currentDirectory, 1024);
  printf("%s\n", temp);
  free(temp);
}

void cd(char* args[]){
  if (args[1] == NULL){
    chdir(getenv("HOME"));
  }
  else {
    if (chdir(args[1]) != 0){
      printf("Error: %s: No such file or directory", args[1]);
    }
  }
}

void clear(char* args[]){
  printf("\e[1;1H\e[2J");
}

void makeDirectory(char* args[]){
  int i = 1;
  int status = 0;
  while(args[i] != NULL){
    int status = mkdir(args[i], 0700);
    if(status==-1){
        printf("Error: %s: in mkdir\n", args[i]);
    }
    i++;
  }
}

void removeDirectory(char* args[]){
  int i = 1;
  int status = 0;
  while(args[i] != NULL){
    status = rmdir(args[i]);
    if(status==-1){
        perror("Error in rmdir ");
    }
    i++;
  }
}

void launchProgram(char **args, int background){
  pid_t pid = fork();
  int error = -1;
  if(pid==-1){
    printf("Child process could not be created\n");
    exit(1);
  }
  if(pid==0){
		if (execvp(args[1],args)==error){
			printf("Command not found");
			kill(pid,SIGTERM);
		}
	 }
  if(background == 0){
    //foreground process
    waitpid(pid,NULL,0);
  }
  else if(background == 1){
    //Do nothing
    //Its a background process
  }

}

void pipeHandler(char * args[]){
	// File descriptors
	int fileDescriptor[2];// pos. 0 output, pos. 1 input of the pipe
	char *command[256];
	pid_t pid;
	int error = -1;
	int end = 0;
	int i = 0, j = 0, tmp = 0;
  pipe(fileDescriptor);

	while (args[j] != NULL && end != 1){
		tmp = 0;

		while (strcmp(args[j],"|") != 0){
			command[tmp] = args[j];
			j++;
			if (args[j] == NULL){
				end = 1;
				tmp++;
				break;
			}
			tmp++;
		}
		command[tmp] = NULL;
    //increment to skip the index of '|'
		j++;
    //create process
		pid=fork();

    //if forking failed
		if(pid==-1){
      if(i == 0){
  		  close(fileDescriptor[1]);
      }
      else{
        close(fileDescriptor[0]);
      }
			printf("Process could not be created\n");
			return;
		}
		if(pid==0){
			// If first program
			if(i == 0){
				dup2(fileDescriptor[1], STDOUT_FILENO);
        execvp(command[1],command);
        close(fileDescriptor[1]);
			}
			// If second program
			else if (i == 1){ // for even number of commands
        dup2(fileDescriptor[0],STDIN_FILENO);
        execvp(command[1],command);
        close(fileDescriptor[0]);
			}
		}
		waitpid(pid,NULL,0);
		i++;
	}
}

void redirectOutput(char * args[], char* outputFile){
  pid_t pid;
	int error = -1;
	int fileDescriptor; // between 0 and 19, describing the output or input file
	if((pid=fork())==-1){
		printf("Child process could not be created\n");
		return;
	}
	if(pid==0){
		// Option 0: output redirection
			// We open (create) the file truncating it at 0, for write only
			fileDescriptor = open(outputFile, O_CREAT | O_TRUNC | O_WRONLY, 0600);
			// We replace de standard output with the appropriate file
			dup2(fileDescriptor, STDOUT_FILENO);
			close(fileDescriptor);
		  if(execvp(args[1],args)==error){
  			printf("error");
  			kill(pid,SIGTERM);
		  }
	}
	waitpid(pid,NULL,0);
}

int executeCommands(char* args[]){
  int i = 0, j = 0;
  int background = 0;
  if (args[0] == NULL) {
    return 1;
  }
  while (args[i] != NULL){
    if (strcmp(args[i],"|") == 0){
      if (args[i+1] == NULL){
        printf("Not enough input arguments\n");
        return -1;
      }
      pipeHandler(args);
      return;
    }
    else if (strcmp(args[i],">") == 0){
      if (args[i+1] == NULL){
        printf("Not enough input arguments\n");
        return -1;
      }
      redirectOutput(args,args[i+1]);
      return;
    }
    i++;
  }
  args[i] = NULL;
  if (strcmp(args[0],"ex") == 0){
    background = 0;
    launchProgram(args,background);
  }
  else if (strcmp(args[0],"exb") == 0){
    background = 1;
    launchProgram(args,background);
  }
  else if(strcmp(args[0],"info") == 0){
    info(args);
  }
  else if(strcmp(args[0],"exit") == 0){
    exitShell(args);
  }
  else if(strcmp(args[0],"pwd") == 0){
    pwd(args);
  }
  else if(strcmp(args[0],"cd") == 0){
    cd(args);
  }
  else if(strcmp(args[0],"clear") == 0){
    clear(args);
  }
  else if(strcmp(args[0],"mkdir") == 0){
    makeDirectory(args);
  }
  else if(strcmp(args[0],"rmdir") == 0){
    removeDirectory(args);
  }
  else{
    printf("%s: command not found...\n", args[0]);
  }
}

void printShell(){
  char* str =getcwd(currentDirectory, 1024);
  char * pch[10];
  char *name[10];
  char hostn[1204];
	gethostname(hostn, sizeof(hostn));
  int i = 0;
  pch[i] = strtok (str,"/");
  while (pch[i] != NULL){
    name[0] = pch[i];
    pch[i] = strtok (NULL, "/");
  }
  printf( CYAN "[%s@%s %s]$" NONE " ", getenv("LOGNAME"), hostn,name[0]);
  free(str);
}

int main(){
  char* args[20];
  char line[150];
  int status = 1;
  while(1){
    printShell();
    parseLine(args, line);
    executeCommands(args);
    }
  return 0;
}
