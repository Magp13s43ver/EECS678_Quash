/**
 * @file quash.c
 *
 * Quash's main file
 */

/**************************************************************************
 * Included Files
 **************************************************************************/ 
#include "quash.h" // Putting this above the other includes allows us to ensure
                   // this file's header's #include statements are self
                   // contained.
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**************************************************************************
 * Private Variables
 **************************************************************************/
/**
 * Keep track of whether Quash should request another command or not.
 */
// NOTE: "static" causes the "running" variable to only be declared in this
// compilation unit (this file and all files that include it). This is similar
// to private in other languages.
static bool running;
char pPath[1024], hHome[1024];

/**************************************************************************
 * Private Functions 
 **************************************************************************/
/**
 * Start the main loop by setting the running flag to true
 */
static void start() {	
  strcpy(pPath, getenv("PATH"));
  strcpy(hHome, getenv("HOME"));
	
	
  running = true;
}

/**************************************************************************
 * Public Functions 
 **************************************************************************/
bool is_running() {
  return running;
}

void terminate() {
  running = false;
  printf("Bye!\n");
}

void set(){

}

void echo(char *string){
	if (strcmp(string, "$HOME") == 0){
		puts(hHome);
	}	
	else if (strcmp(string, "$PATH") == 0){
		puts(pPath);
	}	
	else{
		puts(string);
	}
	
	return;
}

void cd(char *dir){
	int ret = 0;
	
	if (dir == NULL){
		ret = chdir(hHome);	
	}
	else{
		ret = chdir(dir);
	}
	
	if (ret == -1){
		puts("Error, no such directory.");	
	}
	
	return;
}

void pwd(){
	char *currentDir = getcwd(NULL, 0);
	puts(&currentDir);
	free(currentDir);
	
	return;
}

bool get_command(command_t* cmd, FILE* in) { //checks for an input from in, and returns the command.
  if (fgets(cmd->cmdstr, MAX_COMMAND_LENGTH, in) != NULL) {
    size_t len = strlen(cmd->cmdstr);
    char last_char = cmd->cmdstr[len - 1];

    if (last_char == '\n' || last_char == '\r') {
      // Remove trailing new line character.
      cmd->cmdstr[len - 1] = '\0';
      cmd->cmdlen = len - 1;
    }
    else
      cmd->cmdlen = len;
    
    return true;
  }
  else
    return false;
}

/**
 * Quash entry point
 *
 * @param argc argument count from the command line
 * @param argv argument vector from the command line
 * @return program exit status
 */
int main(int argc, char** argv) { 
  command_t cmd; //< Command holder argument
  
  start();
  
  puts("Welcome to Quash!");
  printf("$PATH is: %s\n", pPath);
  printf("$HOME is: %s\n", hHome);
  puts("Type \"exit\" to quit");

  // Main execution loop
  while (is_running() && get_command(&cmd, stdin)) {
    // NOTE: I would not recommend keeping anything inside the body of
    // this while loop. It is just an example.

    // The commands should be parsed, then executed.
    if ((!strcmp(cmd.cmdstr, "exit")) ||(!strcmp(cmd.cmdstr, "quit")))//if the command is "exit"
    {
      terminate(); // Exit Quash
    }
    else
    {
      //puts(cmd.cmdstr); // Echo the input string //maybe execute the command.
      char ncmd[1024];

      if (!strncmp(cmd.cmdstr, "/",1)){
        //puts("/ detected using HOME");
        strcpy(ncmd,hHome);
        strcat(ncmd,strtok(cmd.cmdstr," "));
        //puts(ncmd);//newcmd now contains the directory in command.
        if(execl(ncmd,ncmd,strtok(NULL," "),strtok(NULL," "),strtok(NULL," "),strtok(NULL," ")) <0){
          fprintf(stderr, "\nError. ERROR # %d\n", errno);

        }

      }else{
        puts("/ not detected, using PATH");
        strcpy(ncmd,pPath);//need to change... to try each one
        strcat(ncmd,cmd.cmdstr);
        puts(ncmd);

      }


    }

  }
  return EXIT_SUCCESS;
}
