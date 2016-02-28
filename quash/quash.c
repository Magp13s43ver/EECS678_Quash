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

#include <sys/wait.h>
#include <sys/types.h>
#include <signal.h>
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
command_t backprocess[MAX_BACKGROUND_TASKS];

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
  if ((envVar == NULL) || (*envVar == ' ')){
    puts("Error. Empty variable.");
    return;
  }

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

void jobs(){
  for (int n = 0; n < MAX_BACKGROUND_TASKS; n++){
    if(backprocess[n].pid != 0){
      printf("[%u] %u %s\n", n+1, backprocess[n].pid, backprocess[n].cmdstr);
    }
  }
  return;
}

void killBack(int sigNum, int jobID){
  printf("sigNum: %u, jobID: %u/n", sigNum, jobID);
  if (sigNum == 0 || jobID == 0){
    puts("Invalid command.");
    return;
  }
  else{
    pid_t pidKill = 0;
    for (int n = 0; n < MAX_BACKGROUND_TASKS; n++){
      printf("n: %u, pid: %u\n", n, backprocess[n].pid);
      if((jobID-1 == n) && (backprocess[n].pid != 0)){
        pidKill = backprocess[n].pid;
        kill(pidKill, sigNum);
        break;
      }
    }
    return;
  }
}

void catch_sigchld(int sig_num){
  int status;
  for (int n = 0; n < MAX_BACKGROUND_TASKS; n++){
    if(backprocess[n].pid != 0){
      pid_t ret = waitpid(backprocess[n].pid, &status, WNOHANG);
      if (ret == 0){//child is sill alive dont do anything
      }else if(ret == -1 && errno != 10){//child had an error or child does not exist any more.
        fprintf(stderr, "[%u] %u %s encountered an error. ERROR %d\n",n+1,backprocess[n].pid,backprocess[n].cmdstr,errno);
        backprocess[n].pid = 0;
        break;
      }else{//child exited
        fprintf(stderr, "[%u] %u finished %s\n",n+1,backprocess[n].pid,backprocess[n].cmdstr);
        backprocess[n].pid = 0;
        break;
      }
    }
  }
}

void genCmd(command_t* cmd,int fdread,int fdwrite){
  int status;
  pid_t pid_1;
  int fdtopid1[2];//writing end is 1, reading end is 0 
  //int fdfrpid1[2];
  bool pipecmd;
  char* firstcmd;
  char* secondcmd;
  firstcmd = strtok(cmd->cmdstr, "|");
  if(strcmp(firstcmd,cmd->cmdstr)){//there is a second part.
    secondcmd = strtok(NULL, "");//get the rest.
    if(strncmp((strchr(firstcmd,'\0')-1)," ",1)){
      firstcmd[strlen(firstcmd)] = '\0';//get rid of the space
    }
    if(!strncmp(secondcmd," ",1)){
      secondcmd++;//move ahead the second command
    }
  }

  //pipe(fdfrpid1);

  pid_1 = fork();
  if (pid_1 == 0) { //if child
    if(fdread != -1){
      dup2(fdread,STDIN_FILENO);
    }
    if(fdwrite != -1){
      dup2(fdwrite,STDOUT_FILENO);
    }
    //close(fdtopid1[1]);//close up unused write
    //dup2(fdtopid1[0],STDIN_FILENO);

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
  if(fdread != -1){
      close(fdread);
    }
    if(fdwrite != -1){
      close(fdwrite);
    }
  //close(fdtopid1[0]);
  exit(0);

  }else{// else parent
    //close(fdtopid1[0]);//close up unused read
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
        if(errno != 10){
          fprintf(stderr, "Process %s encountered an error. ERROR %d\n", cmd->cmdstr,errno);
          //return EXIT_FAILURE;
        }
      } 

    }else{//background task
      for (int n = 0; n < MAX_BACKGROUND_TASKS; n++){
        if(backprocess[n].pid == 0){
          memcpy(&backprocess[n],cmd,sizeof(command_t));
          printf("[%u] %u\n",n+1,cmd->pid);
          break;
        }
      }     
      //save to array of tasks, at int i; 
      //char tmpCmd[1024];
      //memset(tmpCmd, 0, 1024);
      //strcpy(tmpCmd, cmd.cmdstr);
      //char *tok;
      //tok = strtok(tmpCmd, " ");

    }
    



    //close(fdtopid1[1]);
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
  else{
    if((errno != 0) && (errno != EINTR)){
    perror ("The following error occurred");
    }
    if(feof(in)){
    //  printf("EOF");
    return false;
    }
    return true;
    //}
  }
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

  memset(backprocess,0,sizeof(command_t)*MAX_BACKGROUND_TASKS);

  struct sigaction sa;
  sigset_t mask_set;  /* used to set a signal masking set. */
  /* setup mask_set */
  sigemptyset(&mask_set);
  sigfillset(&mask_set);//prevent other interupts from interupting intrupts
  sa.sa_mask = mask_set;
  sa.sa_handler = catch_sigchld;
  //sa.sa_flags = SA_RESTART;
  sigaction(SIGCHLD,&sa,NULL);

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
    if(cmd.cmdstr[cmd.cmdlen-1] == '&'){//is a background process
      cmd.cmdstr[cmd.cmdlen-1] = '\0';
      if(cmd.cmdstr[cmd.cmdlen-2] == ' '){
        cmd.cmdstr[cmd.cmdlen-2] = '\0';
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
    else if(strcmp(tok, "jobs") == 0){
      jobs();
    }
    else if(strcmp(tok, "kill") == 0){
      char *sigNum = strtok(NULL, " ");
      char *jobID = strtok(NULL, " ");
      
      if (sigNum == NULL || jobID == NULL){
          printf("Error. Invalid number of arguments.\n");
          continue;
      }
      else{
          int intSigNum = atoi(sigNum);
          int intjobID = atoi(jobID);
   
          killBack(intSigNum, intjobID);
      }
    }
    else 
    {
      memset(tmpCmd, 0, 1024);
      strcpy(tmpCmd, cmd.cmdstr);
      FILE *infileptr = NULL;
      FILE *outfileptr = NULL;
      int infiledsc = -1;
      int outfiledsc = -1;
      if(strchr(tmpCmd,'<') != NULL){
        char *tmp = strchr(tmpCmd,'<')+1;
        if(strncmp(tmp," ",1)==0)//if space, move ahead by one.
          tmp++;
        strtok(tmp, " ");//find next space or end of string. and put \0
        infileptr = fopen(tmp,"r");
        if(infileptr != NULL){
          infiledsc = fileno(infileptr);
        }else{
          perror ("The following error occurred");
          continue;
        }
      }
      strcpy(tmpCmd, cmd.cmdstr);
      if(strchr(tmpCmd,'>') != NULL){
        char *tmp = strchr(tmpCmd,'>')+1;
        if(strncmp(tmp," ",1)==0)//if space, move ahead by one.
          tmp++;
        strtok(tmp, " ");//find next space or end of string. and put \0

        outfileptr = fopen(tmp,"w+");
        outfiledsc = fileno(outfileptr);
      }
      strtok(cmd.cmdstr, "<>");//add \0 to begining of <> segment to designate end of string.
      cmd.cmdlen = strlen(cmd.cmdstr);
      
      genCmd(&cmd,infiledsc,outfiledsc);
      
      if(infileptr != NULL)
        fclose(infileptr);
      if(outfileptr != NULL)
        fclose(outfileptr);
    }
    //puts("hi");
  }
  return EXIT_SUCCESS;
}
