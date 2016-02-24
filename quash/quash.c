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
#include <sys/wait.h>
#include <sys/types.h>



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
  memset(pPath,0,1024);
  memset(hHome,0,1024);
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

void set(char *envVar){
  char *token;

  /* get the first token */
  token = strtok(envVar, "=");

  if (strcmp(token, "PATH") == 0){
    strcpy(pPath, strtok(NULL, ""));
  }
  else if (strcmp(token, "HOME") == 0){
    strcpy(hHome, strtok(NULL, ""));  
  }
  else{
    puts("Error. No such variable.");
  }

  return;
}

void echo(char *string){
  //If empty string, just print a new line.
	if(string == NULL){
    printf("\n");
  }
  else if (strcmp(string, "$HOME") == 0){
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
	
	if (dir == NULL || dir[0] == '~'){
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
	puts(currentDir);
	free(currentDir);
	
	return;
}

void genCmd(command_t* cmd){
  int status;
  pid_t pid_1;
  int fdtopid1[2];//writing end is 1, reading end is 0 
  //int fdfrpid1[2];

  pipe(fdtopid1);
  //pipe(fdfrpid1);

  pid_1 = fork();
  if (pid_1 == 0) { //if child
    close(fdtopid1[1]);//close up unused write
    dup2(fdtopid1[0],STDIN_FILENO);

    int pathNum;// number in the path
    //puts(cmd.cmdstr); // Echo the input string //maybe execute the command.
    char ncmd[1024];
    memset(ncmd,0,1024);
    
    if (!strncmp(cmd->cmdstr, "/",1)){
      //puts("/ detected using HOME");
      strcpy(ncmd,hHome);
      strcat(ncmd,strtok(cmd->cmdstr," "));
      //puts(ncmd);//newcmd now contains the directory in command.
      if(execl(ncmd,ncmd,strtok(NULL," "),strtok(NULL," "),strtok(NULL," "),strtok(NULL," ")) <0){
        fprintf(stderr, "\nError. ERROR # %d\n", errno);

      }

    }else{
      //puts("/ not detected, using PATH");
      strtok(cmd->cmdstr," ");
      char* arg[4] = {strtok(NULL," "),strtok(NULL," "),strtok(NULL," "),strtok(NULL," ")};
      strcpy(ncmd,strtok(pPath,":"));//copy first part of path to newcmd
      pathNum = 1;
      
      bool executed = false;
      while((strncmp(ncmd,"\0",1) != 0) && (executed == false)){ //while the path is not null, and not executed
        strcat(ncmd,"/");//add "/" between path and command.
        strcat(ncmd,cmd->cmdstr); //cat on the cmd str
        
        if (execl(ncmd,ncmd,arg[0],arg[1],arg[2],arg[3]) < 0){//try the next one //if failed:
          char *tmp = NULL;
          for(int i = 0; i<pathNum; i++){
            tmp = strtok(NULL,":");//go to the next part of the path.
          }
          pathNum++;

          if (tmp != NULL){//if not found a null termenator?
            strcpy(ncmd,tmp);//copy new str to ncmd
          }else{
            ncmd[0] = '\0';//else set ncmd first char to null
          }
          
        } else{

          executed = true;
        }
      }
      if (executed == false){
        puts("Failed to find command.");
      }
    }
  close(fdtopid1[0]);
  exit(0);

  }else{// else parent
    close(fdtopid1[0]);//close up unused read
    cmd->pid = pid_1;
    //int tempdup;
    //tempdup = dup(STDIN_FILENO);//save stdin?
    //dup2(fdtopid1[1],STDIN_FILENO);
    if(cmd->background == false){
      //puts("hi!");
      //while(waitpid(pid_1, &status, WNOHANG) >= 0){//wile true, pipe stdin to shild.
      //  char ch;
      //  if (read(STDIN_FILENO, &ch, 1) > 0){
      //    write(fdtopid1[1],&ch,1);
      //  }
      //}
      //close(fdtopid1[1]);
      if ((waitpid(pid_1, &status, 0)) == -1) {
        fprintf(stderr, "Process 1 encountered an error. ERROR%d", errno);
      return EXIT_FAILURE;
      } 

    }else{//background task
      //save to array of tasks, at int i; 
      //char tmpCmd[1024];
      //memset(tmpCmd, 0, 1024);
      //strcpy(tmpCmd, cmd.cmdstr);
      //char *tok;
      //tok = strtok(tmpCmd, " ");
      int i =0; 
      printf("[%u] %u ",i,cmd->pid);


    }
    



    close(fdtopid1[1]);
    //dup2(tempdup,STDIN_FILENO);//replace stdin.



  }

}

bool get_command(command_t* cmd, FILE* in) { //checks for an input from in, and returns the command.
  memset(cmd->cmdstr,0,1024);
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
  
  char tmpCmd[1024];
  memset(tmpCmd, 0, 1024);

  start();
  
  puts("Welcome to Quash!");
  printf("$PATH is: %s\n", pPath);
  printf("$HOME is: %s\n", hHome);
  puts("Type \"exit\" to quit");

  // Main execution loop
  while (is_running() && get_command(&cmd, stdin)) {
    //Copy the command string to a temporary variable.
    memset(tmpCmd, 0, 1024);
    strcpy(tmpCmd, cmd.cmdstr);

    //Split tmpCmd to get the command 
    char *tok;
    tok = strtok(tmpCmd, " ");
    
    // NOTE: I would not recommend keeping anything inside the body of
    // this while loop. It is just an example.
 
    //If not receiving any command from user, skip iteration to prevent segmentation fault.
    if ((strlen(tmpCmd) == 0)||(tok == NULL)){
      continue;
    }
    if(cmd.cmdstr[cmd.cmdlen] == '&'){//is a background process
      cmd.cmdstr[cmd.cmdlen] = '\0';
      if(cmd.cmdstr[cmd.cmdlen-1] == ' '){
        cmd.cmdstr[cmd.cmdlen-1] = '\0';
      }
      cmd.background = true;

    }else{
      cmd.background = false;
    }

    // The commands should be parsed, then executed.
    if ((!strcmp(tmpCmd, "exit")) ||(!strcmp(tmpCmd, "quit")))//if the command is "exit"
    {
      terminate(); // Exit Quash
    }
    else if(strcmp(tok, "echo") == 0){
      echo(strtok(NULL, ""));
    }
    else if(strcmp(tok, "cd") == 0){
      cd(strtok(NULL, ""));
    }
    else if(strcmp(tok, "pwd") == 0){
      pwd();
    }
    else if(strcmp(tok, "set") == 0){
      set(strtok(NULL, ""));
    }
    else 
    {

      genCmd(&cmd);

    }
    //puts("hi");
  }
  return EXIT_SUCCESS;
}
