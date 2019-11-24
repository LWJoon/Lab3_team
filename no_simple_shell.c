#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
 #include <sys/stat.h>
#include <fcntl.h>

/* The array below will hold the arguments: args[0] is the command. */
static char* args[512];
pid_t pid;
int command_pipe[2];
 
#define READ  0
#define WRITE 1

bool cmd_cd (char* args[]);
bool cmd_output( char* args[] );
struct COMMAND{ // 커맨드 구조체
        char* name;
        char* desc;
        bool ( *func )( char* args[] ); // 함수포인터. 사용할 함수들의 매개변수를 맞춰줌
};

struct COMMAND  builtin_cmds[] =
{
    { "cd",    "change directory",                    cmd_cd   },
    { ">",    "Redirection output",                    cmd_output   }
};

bool cmd_output( char* args[] ){ //cd : change directory

   int fd; /* file descriptor */
   char *buf = "This is a test output file.\n";
   int flags = O_WRONLY | O_CREAT | O_TRUNC;
   mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; /* == 0644 */

   if ( (fd = open(args[1], flags, mode)) == -1 ) {
      perror("open"); /* errno에 대응하는 메시지 출력됨*/
   }
   if ( dup2(fd, 1) == -1 ) {
      perror("dup2"); /* errno에 대응하는 메시지 출력됨 */
   }
   if ( close(fd) == -1 ) {
      perror("close"); /* errno에 대응하는 메시지 출력됨*/
   }
   write(1, buf, strlen(buf));
   return true;
}

bool cmd_cd( char* args[] ){ //cd : change directory
        if( sizeof(args)/4 == 1 )
                chdir( getenv( "HOME" ) );
        else if( sizeof(args)/4 == 2 ){
            if( chdir( args[1] ) )
                printf( "No directory\n" );
        }
        else
            printf( "USAGE: cd [dir]\n" );
   
   return true;
}


static int command(int input, int first, int last)
{
   int pipettes[2];
 

   pipe( pipettes );   
   pid = fork();
 
    int i = 0;
   if (pid == 0) {
      for( i = 0; i < sizeof( builtin_cmds ) / sizeof( struct COMMAND ); i++ ){
              if( strcmp( builtin_cmds[i].name, args[0] ) == 0 )
                      return builtin_cmds[i].func( args );
      }
      if (first == 1 && last == 0 && input == 0) {
         // First command
         dup2( pipettes[WRITE], STDOUT_FILENO );
      } else if (first == 0 && last == 0 && input != 0) {
         // Middle command
         dup2(input, STDIN_FILENO);
         dup2(pipettes[WRITE], STDOUT_FILENO);
      } else {
         // Last command
         dup2( input, STDIN_FILENO );
      }
 
      if (execvp( args[0], args) == -1)
         _exit(EXIT_FAILURE); // If child fails
   }
 
   if (input != 0) 
      close(input);
 
   // Nothing more needs to be written
   close(pipettes[WRITE]);
 
   // If it's the last command, nothing more needs to be read
   if (last == 1)
      close(pipettes[READ]);
 
   return pipettes[READ];
}
 
/* Final cleanup, 'wait' for processes to terminate.
 *  n : Number of times 'command' was invoked.
 */
static void cleanup(int n)
{
   int i;
   for (i = 0; i < n; ++i) 
      wait(NULL); 
}
 
static int run(char* cmd, int input, int first, int last);
static char line[1024];
static int n = 0; /* number of calls to 'command' */
 
int main()
{
   printf("SIMPLE SHELL: Type 'exit' or send EOF to exit.\n");
   while (1) {
      /* Print the command prompt */
      printf("Lab3_Team_돌멩이$> ");
      fflush(NULL);
 
      /* Read a command line */
      if (!fgets(line, 1024, stdin)) 
         return 0;
 
      int input = 0;
      int first = 1;
 
      char* cmd = line;
      char* next = strchr(cmd, '|'); /* Find first '|' */
      while (next != NULL) {
         /* 'next' points to '|' */
         *next = '\0';
         input = run(cmd, input, first, 0);
 
         cmd = next + 1;
         next = strchr(cmd, '|'); /* Find next '|' */
         first = 0;
      }
      input = run(cmd, input, first, 1);
      cleanup(n);
      n = 0;
   }
   return 0;
}
 
static void split(char* cmd);
 
static int run(char* cmd, int input, int first, int last)
{
   split(cmd);
   if (args[0] != NULL) {
      if (strcmp(args[0], "exit") == 0) 
         exit(0);
      n += 1;
      return command(input, first, last);
   }
   return 0;
}
 
static char* skipwhite(char* s)
{
   while (isspace(*s)) ++s;
   return s;
}



static void split(char* cmd)
{
   cmd = skipwhite(cmd);
   char* next = strchr(cmd, ' ');
   int i = 0;
 
   while(next != NULL) {
      next[0] = '\0';
      args[i] = cmd;
      ++i;
      cmd = skipwhite(next + 1);
      next = strchr(cmd, ' ');
   }
 
   if (cmd[0] != '\0') {
      args[i] = cmd;
      next = strchr(cmd, '\n');
      next[0] = '\0';
      ++i; 
   }
 
   args[i] = NULL;
}
