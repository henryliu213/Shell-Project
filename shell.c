// PID: 730532086
// I pledge the COMP211 honor code.

// ----------------------------------------------
// These are the only libraries that can be
// used. Under no circumstances can additional
// libraries be included

#include <ctype.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "shell.h"

const char *PATH_SEPARATOR = ":";

// --------------------------------------------
// Currently only two builtin commands for this
// assignment exist
// --------------------------------------------
const char* BUILT_IN_COMMANDS[] = { "cd", "exit", NULL };


/* ------------------------------------------------------------------------------
 *
 * YOU NEED TO COMPLETE THIS FUNCTION.
 *
 * Allocate memory for the command. The last element in `p_cmd->argv` should be
 * NULL.
 *
 * The length of `p_cmd->argv` should be `argc+1`: the first `argc` slots are
 * used to store the arguments and the last one is set to NULL.
 *
 * Arguments:
 *      p_cmd : pointer to the command need to allocate memory.
 *      argc :  the number of arguments.
 *
 * Return:
 *      None
 */
void alloc_mem_for_command(command_t* p_cmd, int argc) {
    p_cmd->argc = argc;
    if (argc <=0){
        return;
    }

    p_cmd->argv = (char**)malloc(sizeof(char*)*(argc+1));
    p_cmd->argv[argc] = NULL;
    
    for (int i = 0; i < argc; i++){
        p_cmd->argv[i]= (char*)malloc(sizeof(char)*(MAX_ARG_LEN+1));
        p_cmd->argv[MAX_ARG_LEN] = 0;
    }

} // end alloc_mem_for_command function

/* ------------------------------------------------------------------------------
 *
 * YOU NEED TO COMPLETE THIS FUNCTION.
 *
 * This function is used to free memory that may be malloc'd for the name and
 * argv fields in the command_t structure.  To be safe, you should also set the
 * pointer values to NULL.
 *
 * HINT(s): See man page for more information about free function
 *
 * Arguments:
 *         p_cmd : pointer to a command_t structure
 * Return: 
 *        None
 *
 */
void cleanup(command_t* p_cmd) {
    for (int i = 0; i < p_cmd->argc; i++){
        free(p_cmd->argv[i]);
        p_cmd->argv[i]= NULL;
    }
    free(p_cmd->argv);
    p_cmd->argv = NULL;

} // end cleanup function

/* ------------------------------------------------------------------------------
 *
 * YOU NEED TO COMPLETE THIS FUNCTION.
 *
 * This function will parse the command (cmd for short) and its arguments
 * specified by the user.
 *
 * HINT(s): This function is "essentially" an exercise in string parsing.
 *
 *         EXAMPLE#1, if the user enters
 *
 *                                 cd /mnt/cdrom
 *
 *                 at the shell prompt, line would contain "cd /mnt/cdrom".
 *                 Using the space character as the delimiter, the fields in
 *                 the command struct would be:
 *
 *                                 - argc = 2
 *                                 - argv = {"cd", "/mnt/cdrom", NULL}
 *
 *         EXAMPLE#2, if the user enters
 *
 *                                 ls -la
 *
 *                 at the shell prompt, line would contain "ls -la". Using the
 *                 space character as the delimiter, the fields in the command
 *                 struct would be:
 *
 *                                 - argc = 2
 *                                 - argv = {"ls", "-la", NULL}
 *
 *         EXAMPLE#3, if the user enters nothing at the shell prompt (i.e.
 *         simply hits the return key), line would NULL, and the fields in
 *         the command struct would be:
 *
 *                                 - argc = 0
 *                                 - argv = {NULL}
 *
 *  Arguments:
 *      line: pointer to the string containing the cmd.
 *      p_cmd: pointer to the command_t structure
 *
 *  Return:
 *      N/A
 *
 */
void parse(char* line, command_t* p_cmd) {
    int argc = 0;
    bool new = true;

    //Find argc
    for (int i = 0; i<strlen(line); i++){
        if (line[i] == 0 && new){
            new = false;
            break;}
        else if(line[i] == 0 && !new){
            break;}
        else if (line[i] != ' ' && new){
            new = false;
            argc++;}
        else if(line[i] == ' ' && !new){
            new = true;}
    }

    alloc_mem_for_command(p_cmd, argc);
    if (argc <= 0){
        return;
    }

    int indexofargv = 0;
    char* current = p_cmd->argv[indexofargv]; 
    int indexofargument = -1;
    bool linehasbeenreset = true; 
    bool isthereendsapces = false;

    for (int i = 0; i < strlen(line); i++){
        if(line[i]!= ' '){
            linehasbeenreset = false; 
            current[++indexofargument]=line[i];
            isthereendsapces = false;
        }
        else if(line[i]== ' ' && !linehasbeenreset){ 
            linehasbeenreset = true; 
            current[++indexofargument] = 0;
            current = p_cmd->argv[++indexofargv];
            indexofargument =-1;
            isthereendsapces = true;
        }
    }

    if(!isthereendsapces){
        current[++indexofargument] = 0;
        linehasbeenreset = true;
        indexofargument = 0;
    }
} // end parse function

/* ------------------------------------------------------------------------------
 *
 * YOU NEED TO COMPLETE THIS FUNCTION.
 *
 * This function is used determine if the named command (cmd for short) entered
 * by the user in the shell can be found in one of the folders defined in the
 * PATH environment (env or short) variable. If the file exists, then the name
 * of the executable at argv[0] is replaced with the location (fully qualified
 * path) of the executable.
 *
 * For example,
 *
 * if command_t.argv[0] is "ls", and "ls" is in the "/usr/bin" folder,
 * then command_t.argv[0] would be changed to "/usr/bin/ls"
 *
 * HINT(s): Use getenv() system function to retrieve the folders defined in the
 *                 PATH variable. An small code segment is shown below that
 * demonstrates how to retrieve folders defined in your PATH.
 *
 *                         char* path_env_variable;
 *                         path_env_variable = getenv( "PATH" );
 *                         printf("PATH = %s\n", path_env_variable );
 *
 *                 The format of the PATH is very simple, the ':' character is
 * delimiter, i.e. used to mark the beginning and end of a folder defined in the
 * path.
 *
 *                 Write a loop that parses each folder defined in the path,
 * then uses this folder along with the stat function (see "File/Directory
 * Status" section in the assignment PDF).
 *
 *  Arguments:
 *         p_cmd: pointer to the command_t structure
 *  Return:
 *      true: if cmd is in the PATH
 *      false:  if not in the PATH.
 *
 */
bool find_fullpath(command_t* p_cmd) {
    char* path_env_variable;
    path_env_variable = getenv( "PATH" );
    int count = 0; 
    char str[MAX_ARG_LEN+1];

    for (int i = 0; i<strlen(path_env_variable); i++){
        if (path_env_variable[i] == ':' || path_env_variable[i] == 0){
            //Add name of file to path. 
            str[count] = '/';
            count++;
            for (int j = 0; j<strlen(p_cmd->argv[0]); j++){
                str[count] = p_cmd->argv[0][j];
                count++;
            }
            str[count] = 0;            
            
            //see if file is in this location.
            struct stat buff;
            if (stat(str, &buff) == 0 && (S_IFREG & buff.st_mode)) {
                int i = 0;
                for (i = 0; i <strlen(str); i++){
                    p_cmd->argv[0][i] = str[i]; 
                    }
                p_cmd->argv[0][i] = 0;
                return true;
            } 
            //reset directory. 
            else {  
                str[0] = 0;
                count = 0;
            } 
        }
        else{
            str[count] = path_env_variable[i];
            count++;
        }
    }
    return false;
} // end find_fullpath function

/* ------------------------------------------------------------------------------
 *
 * YOU NEED TO COMPLETE THIS FUNCTION.
 *
 * This function will execute external commands (cmd for short).
 *
 *
 * Arguments:
 *      p_cmd: pointer to the command_t structor
 *
 * Return:
 *      SUCCESS: successfully execute the command.
 *      ERROR: error occurred.
 *
 */
int execute(command_t* p_cmd) {
    int status = SUCCESS;
    int child_process_status;
    pid_t child_pid;
    
    if(is_builtin(p_cmd)){
        return do_builtin(p_cmd);
    }

    else if (!find_fullpath(p_cmd)){
        printf("Command '%s' not found!\n", p_cmd->argv[0]);
        status = ERROR;
    }

    else{
        child_pid = fork();
        if (child_pid == 0){
            execv(p_cmd->argv[0], p_cmd->argv);
            exit(-1);
        }
        else{
            wait(&child_process_status);
        }
    }

    return status;

} // end execute function

/* ------------------------------------------------------------------------------
 *
 * This function will determine if command (cmd for short) entered in the shell
 * by the user is a valid builtin command.
 *
 * Arguments:
 *      p_cmd: pointer to the command_t structure
 * Return:
 *      true:  the cmd is in array `valid_builtin_commands`.
 *      false: not in array `valid_builtin_commands`.
 *
 */
bool is_builtin(command_t* p_cmd) {
    int cnt = 0;

    while(BUILT_IN_COMMANDS[cnt] != NULL) {
        if (strcmp(p_cmd->argv[0], BUILT_IN_COMMANDS[cnt]) == 0) {
            return true;
        }

        cnt++;
    }

    return false;
} // end is_builtin function

/* ------------------------------------------------------------------------------
 *
 * This function is used execute built-in commands such as change directory (cd)
 *
 * HINT(s): See man page for more information about chdir function
 *
 * Arguments:
 *      p_cmd: pointer to the command_t structure
 * Return:
 *      SUCCESS: no error occurred during chdir operation.
 *      ERROR: an error occured during chdir operation.
 *
 */
int do_builtin(command_t* p_cmd) {
    // If the command was 'cd', then change directories
    // otherwise, tell the program to EXIT

    struct stat buff;
    int status = SUCCESS;

    // exit
    if (strcmp( p_cmd->argv[0], "exit") == 0) {
        exit(status);
    }

    // cd
    if (p_cmd->argc == 1) {
        // -----------------------
        // cd with no arg
        // -----------------------
        // change working directory to that
        // specified in HOME environmental
        // variable

        status = chdir(getenv("HOME"));
    } else if ( (status = stat(p_cmd->argv[1], &buff)) == 0 && (S_IFDIR & buff.st_mode) ) {
        // -----------------------
        // cd with one arg
        // -----------------------
        // only perform this operation if the requested
        // folder exists

        status = chdir(p_cmd->argv[1]);
    }

    return status;

} // end do_builtin function
