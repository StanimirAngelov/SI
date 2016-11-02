/*le         Customshell.c
  @author       Stanimir Angelov - C12733849 - DT211C/4
       Networking Shell
*/


#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/stat.h>
#include <dirent.h>
#include <assert.h>


/*
  Function Declarations for builtin shell commands:
 */
int Registercd(char **args);
int Registerhelp(char **args);
int Registerexit(char **args);
int Registerifc(char **args);
int Registerpw(char **args);
int Registerdt(char **args);
int Registerud(char **args);
int Registerbtb(char **args);



/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
  "ifc",
  "pw",
  "dt",
  "ud",
  "btb",
  "cd",
  "help",
  "exit"
};

int (*builtin_func[]) (char **) = {
  &Registerifc,
  &Registerpw,
  &Registerdt,
  &Registerud,
  &Registerbtb,
  &Registercd,
  &Registerhelp,
  &Registerexit
};

int Registernum_builtins() {
  return sizeof(builtin_str) / sizeof(char *);
}



/*
    Builtin function implementations. 
    Builtin command: shortcut for running the ifconfig command   
    Always returns 1, to continue executing.
 */
int Registerifc(char **args)
{
      
  if (args[1] == NULL) {
    //reading the ifconfig file
    FILE *fp = popen("/sbin/ifconfig", "r");
    
    char returnData[64];
    //outputting the ifconfig function
    while (fgets(returnData, 64, fp) !=NULL)
    {
    printf("%s", returnData);
    } 
  } else {
    printf("error with ifconfig command\n");
  }
  
  return 1;
}

/*
   Builtin command: shortcut to display the current directory path.
   Always returns 1, to continue executing.
 */
int Registerpw(char **args){

  if(args[1]==NULL){
    char cwd[1024];
    getcwd(cwd, sizeof(cwd));
    printf("%s\n",cwd);
  }else{
    printf("error");
  }
  return 1;
}

/*
   Builtin command: shortcut to the date command and displays in a new format
   List of args.  Not arguments
   Always returns 1, to continue executing.
 */
int Registerdt(char **args)
{

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  printf("%d%d%d%d%d%d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, 
    tm.tm_hour, tm.tm_min, tm.tm_sec);
  return 1;
}

/*
    Builtin command: displays the userID, groupID, username, groupname and iNode of /home
 */
int Registerud(char **args){
    /*
        Declaring variables for the userID, groupID, username, 
        groupname and iNode and storing them
    */
    uid_t uid = geteuid();                      
    struct passwd *pw = getpwuid(uid);  //getting userid        
    gid_t gid = getgid();                       
    struct group *grp = getgrgid(gid);                  
    DIR *dir;
    struct dirent *dp;
    int inode;
    
    if (pw)
    {
	    
            printf("%d,%d,%s,%s,", uid, gid, pw->pw_name, grp->gr_name);
            
            if((dir = opendir("/home")) == NULL){
                printf ("Cannot open /home");
                exit(1);  
            }
            if((dp = readdir(dir)) != NULL){
                inode = dp->d_ino;
                printf("%d\n", inode);// iNode of the home directory
            }
            closedir(dir);  
    }else{
        printf("failed\n");
    }
    return 1;
}

/*
    Builtin command: Changes the default shell for users. 
    List of args.  args[0] is "btb".  args[1] is the directory 
    path and args[3] is the USERNAME.
    Always returns 1, to continue executing.
 */
int Registerbtb(char **args)
{
  char buf[50];

  if(args[1] == NULL){
    printf("No args\n");
  }
  else if(args[1] != NULL){
    int ret;
    ret = strcmp(args[1],"man");
    if(ret == 0){
      printf("This command changes the default shell of a user.\nCommand example: btb /bin/bash USERNAME \n");
    }
    else{
      snprintf(buf, sizeof(buf), "chsh -s %s %s", args[1], args[2]);
      system(buf);}
  }
  
  return 1;
}

/*
    Builtin command: change directory.
    List of args.  args[0] is "cd".  args[1] is the directory.
    Always returns 1, to continue executing.
 */
int Registercd(char **args)
{
  if (args[1] == NULL) {
    fprintf(stderr, "Error: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("");
    }
  }
  return 1;
}

/*
   Builtin command: print help.
   List of args.  Not examined.
   Always returns 1, to continue executing.
 */
int Registerhelp(char **args)
{
  int i;
  printf("The following commands are built in:\n");

  for (i = 0; i < Registernum_builtins(); i++) {
    printf("  %s\n", builtin_str[i]);
  }
  return 1;
}

/*
    Builtin command: exit.
    List of args.  Not examined.
    Always returns 0, to terminate execution.
 */
int Registerexit(char **args)
{
  return 0;
}


/*
    Launch a program and wait for it to terminate.
    Null terminated list of arguments (including program).
    Always returns 1, to continue execution.
 */

int Registerlaunch(char **args)
{
  pid_t pid;
  int status;

  pid = fork();
  if (pid == 0) {
    // Child process
    if (execvp(args[0], args) == -1) {
      perror("");
    }
    exit(EXIT_FAILURE);
  } else if (pid < 0) {
    // Error forking
    perror("");
  } else {
    // Parent process
    do {
      waitpid(pid, &status, WUNTRACED);
      //wifexited return true if child terminated normally
      //wifsignaled return true if child terminated by a signal
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));
  }

  return 1;
}

/*
    Execute shell built-in or launch program.
    Null terminated list of arguments.
    return 1 if the shell should continue running, 0 if it should terminate
 */
int Registerexecute(char **args)
{
  int i;

  if (args[0] == NULL) {
    // An empty command was entered.
    return 1;
  }

  for (i = 0; i < Registernum_builtins(); i++) {
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }

  return Registerlaunch(args);
}

#define RegisterRL_BUFSIZE 1024
/*
    Read a line of input from stdin.
    return The line from stdin.
*/
char *Registerread_line(void)
{
  int bufsize = RegisterRL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, ": allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    // Read a character
    c = getchar();

    // If we hit EOF, replace it with a null character and return.
    if (c == EOF || c == '\n') {
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    // If we have exceeded the buffer, reallocate.
    if (position >= bufsize) {
      bufsize += RegisterRL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, ": allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define RegisterTOK_BUFSIZE 64
#define RegisterTOK_DELIM " \t\r\n\a"
/*
   Split a line into tokens 
   line The line.
   Return Null-terminated array of tokens
 */
char **Registersplit_line(char *line)
{
  int bufsize = RegisterTOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;
  //checking allocation is ok
  if (!tokens) {
    fprintf(stderr, ": allocation error\n");
    exit(EXIT_FAILURE);
  }


  //while token is not empty keep incrementing
  token = strtok(line, RegisterTOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;
    
   
    // rallocate to buffer if its too small
    if (position >= bufsize) {
      bufsize += RegisterTOK_BUFSIZE;
      tokens_backup = tokens;
      tokens = realloc(tokens, bufsize * sizeof(char*));
      if (!tokens) {
        free(tokens_backup);
        fprintf(stderr, ": allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, RegisterTOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

/*
   Loop getting input and executing it.
 */
void Registerloop(void)
{
  char *line;
  char **args;
  int status;

  do {
    printf("CustomShell$ ");
    line = Registerread_line();
    args = Registersplit_line(line);
    status = Registerexecute(args);

    free(line);
    free(args);
  } while (status);
}

/**
   Main entry point.
   argc Argument count.
   argv Argument vector.
   Return status code
 */
int main(int argc, char **argv)
{
  

  // Run command loop.
  Registerloop();

  // Perform any shutdown/cleanup.

  return EXIT_SUCCESS;
}

