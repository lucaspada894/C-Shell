#include <stdio.h> 
#include <string.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/stat.h> 
#include <fcntl.h>

#define MAX_LINE 80
#define MAX_PARAMS 8
#define HISTORY_COUNT 10

/**
**This program creates a shell over the linux system. It implements simple
 features such as history, most recent command, I/O redirection, and pipelining. 
 Be aware that sometimes there is some weird behavior with history after having 
 called redirection or pipelining. Working on it! 
**/

// Checks for the pipe symbol
int check_for_pipe(char**params, char** command1, char** command2){
    for(int i = 0; params[i] != NULL; i++){
        if(params[i][0] == '|'){
            //free(params[i]);
            for(int k = 0; k < i; k++){
                command1[k] = params[k];
            }  
            int n = 0;
            for(int m = i+1; params[m] != NULL; m++){
                command2[n] = params[m];
                n++;
            }
            if(command1[strlen(*command1)-1] == '\n') {
                command1[strlen(*command1)-1] = '\0';
            }
            if(command2[strlen(*command2)-1] == '\n') {
                command2[strlen(*command2)-1] = '\0';
            }
            return 1;
        }
    }
    return 0;
}


//Checks for redirect '>' symbol. If it finds it, it return 1. Returns 0 otherwise.
int check_redirect_input(char **args, char **input_filename) {
  int i;
  int j;

    
  for(i = 0; args[i] != NULL; i++) {

    // Look for the <
    if(args[i][0] == '<') {
      //free(args[i]);
    
      // Read the filename
      if(args[i+1] != NULL) {
	    *input_filename = args[i+1];
         } else {
	    return -1;
        }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	    args[j] = args[j+2];
      }

      return 1;
    }
  }
  return 0;
}

//Checks for redirect '<' symbol.
int check_redirect_output(char **args, char **output_filename) {
  int i;
  int j;

  for(i = 0; args[i] != NULL; i++) {

    // Look for the >
    if(args[i][0] == '>') {

      // Get the filename 
      if(args[i+1] != NULL) {
	*output_filename = args[i+1];
      } else {
	return -1;
      }

      // Adjust the rest of the arguments in the array
      for(j = i; args[j-1] != NULL; j++) {
	args[j] = args[j+2];
      }

      return 1;
    }
  }
  return 0;
}

//parses the entire command into the string array params.
void parseCommand(char* cmd, char** params)
{       
    for(int i = 0; i < MAX_PARAMS; i++) {
        params[i] = strsep(&cmd, " ");
        if(params[i] == NULL) break;
    }
}

//Executes the given command.
int executeCommand(char** params, int input, char *input_filename, int output, char *output_filename)
{
    // Fork process
    pid_t pid = fork();

    // Error
    if (pid == -1) {
        printf("error with forking command");
        return 1;
    }
    // Child process
    else if (pid == 0) {
        // Execute command
        execvp(params[0], params);  
        // Error occurred
        printf("error executing command\n");
        return 0;
    }
    // Parent process
    else {
        // Wait for child process to finish
        int childStatus;
        wait(NULL);
        return 1;
    }
}

//Performs the piping of two commands.
int Pipe(char **command1, char **command2)
{
    int fd[2];
    pid_t pid;
    
    pipe(fd);

    pid = fork();
    int terminal_stdin = dup(0);
    int terminal_stdout = dup(1);

    if (pid == -1)
        perror("failed to fork");

    if (pid == 0)  
     {  
        close(fd[0]); 
        dup2(fd[1], STDOUT_FILENO); 
        close(fd[1]); 
        execvp(command1[0], command1);
    }
    else
    {
     
            close(fd[1]); 
            dup2(fd[0], STDIN_FILENO); 
            close(fd[0]); 
            wait(NULL);
            execvp(command2[0], command2);
            dup2(terminal_stdout,fd[1]);
            return 0;
        
    } 
    return 0;
}

//Gets previous 10 commands in history. However, this does not seem to work too well when redirection
// or piping commands are given before calling history.
int getHistory(char *history[], int current)
{
        int i = current;
        int history_num = 1;

        do {
                if (history[i]) {
                    if(history_num > 1){
                        printf("%s\n", history[i]);
                        history_num++;
                    }
                    else {
                        printf("%s\n", history[i]);
                        history_num++;
                    }
                }
                i = (i + 1) % HISTORY_COUNT;

        } while (i != current);

        return 0;
}

int main() {

    char cmd[MAX_LINE + 1];
    char* params[MAX_PARAMS + 1];
    char *history[HISTORY_COUNT];
    int commandCount = 0;
    int i, current = 0;
    char previousCommand[MAX_PARAMS + 1];
    char* output_filename;
    char* input_filename;
    int input;
    int output;
    int pipe;
    char* command1[MAX_LINE + 1];
    char* command2[MAX_LINE + 1];

    

    for (i = 0; i < HISTORY_COUNT; i++)
                history[i] = NULL;

    while(1) {
        printf("osh>");
        fflush(stdout);
        
        // Read command from standard input
        
        if(fgets(cmd, sizeof(cmd), stdin) == NULL) {
            break; 
        }
        
        // Remove any trailing newline character
        if(cmd[strlen(cmd)-1] == '\n') {
            cmd[strlen(cmd)-1] = '\0';
        }
        fflush(stdout);

        if(strcmp(cmd,"!!") != 0){
          
            strcpy(previousCommand,cmd);
            //printf(previousCommand);
            //free(history[current]);
            history[current] = strdup(cmd);
        }
        
        current = (current + 1) % HISTORY_COUNT;

         if (strcmp(cmd, "history") == 0){
            getHistory(history, current);
            continue;
         }
         if(strcmp(cmd, "!!") == 0){
             
             if(strcmp(previousCommand,"") != 0){
                strcpy(cmd, previousCommand);
                //parseCommand(previousCommand,params);
                //if(executeCommand(params, 0, input_filename, 0, output_filename) == 0) continue;
             }
            else
                printf("no commands in history\n");
         }
         
        parseCommand(cmd, params);
        input = check_redirect_input(params, &input_filename);
        output = check_redirect_output(params,&output_filename);
        pipe = check_for_pipe(params, command1, command2);
        

    
        if(input == 1){
            int terminal_stdin = dup(0);
            pid_t parent = fork();
            if(parent == 0){
                    int in = open(input_filename, O_RDONLY);
                    dup2(in,0);
                    close(in);
                    if(executeCommand(params, input, input_filename, output, output_filename) == 0){
                         break;
                    }
                    else
                        dup2(terminal_stdin,0);
            }
            else {
                //dup2(terminal_stdin,0);
                //close();
                wait(NULL);
            }
        }
         else if(output == 1){
            int terminal_stdout = dup(1);
            pid_t parent = fork();
            //Child
            if(parent == 0){
                int out = open(output_filename, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
                dup2(out,1);
                close(out);
                if(executeCommand(params, input, input_filename, output, output_filename) == 0){
                    break;
                }
                else
                    dup2(terminal_stdout,1);
                     
            }
            else {
                //dup2(terminal_stdout,1);
                wait(NULL); 
            }
        }
        else if(pipe == 1){
            //printf("%s\n",*command1);
            //printf("%s\n",*command2);
            //printf("%d",getpid());
            pid_t pipe_pid = fork();
            //we are in child
            if(pipe_pid == 0){
                command1[strlen(*command1)-1] = '\0';
                command2[strlen(*command2)-1] = '\0';
                Pipe(command1, command2);
                exit(0);
            }
            // }
            else {
                wait(NULL);
                continue;
             }
        } 
        else if(strcmp(params[0], "exit") == 0) break;
        else {
            if(executeCommand(params, input, input_filename, output, output_filename) == 0) break;
        }
}
return 0;
}
