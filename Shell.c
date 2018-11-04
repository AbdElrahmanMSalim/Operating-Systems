#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAX_ARGS 15
#define MAX_LINE 200
int background;
int bgp=0,done=0;

void error_massage(){
     printf("Please Enter A Valid Shell Command\n");
}

void remove_end_line(char line[]){
    int i =0;
    while(line[i] != '\n' && line[i] != '>' && line[i] != '<' && line[i] != '|')
        i++;

    line[i] = '\0';
}

int detect_redirection(char line[], char redirection_line[]){
    int i = 0;
    int ret;
    while (line[i] != '\0' && line[i] != '>' && line[i] != '<' && line[i] != '|')
        i++;

    if (line [i] == '\0')
        return -1;
    if (line[i] == '>'){
        if(line[i+1] == '>'){
            i++;
            ret = 2;
        }
        else
            ret = 1;
    }
    else if (line[i] == '<')
        ret = 0;
    else
        ret = 3;

    i++;
    int j = 0;
    while (line [i] == ' ')
        i++;
    while (line[i] != '\0'){
        redirection_line[j] = line[i];
        i++;
        j++;
    }
    redirection_line[j-1] = '\0';

    return ret;
}

// reading shell command line
int read_line(char line[], char redirection_line[]){
    int i=0;
    char* command=fgets(line,MAX_LINE,stdin);
    int ret = detect_redirection(line, redirection_line);
    while(line[i]!='\n')
    i++;
    if(line[i-1]=='&'){
	line[i-1]='\n';
        background=1;
    }
    remove_end_line(line);
    if (strcmp(line,"exit")==0 || command==NULL)
    exit(0);
    return ret;
}

// parsing line
int split_line(char line[],char* args[]){
    int i = 0;
    args[i] = strtok(line," ");

    if(args[i] == NULL)
    {
        if (done!=bgp)
		return 1;
        printf("NO COMMANDS !\n");
        return 1;
    }
    while(args[i] != NULL)
    {
        i++;
        args[i] = strtok(NULL," ");
    }
    return 1;
}


int main()
{

    char line[200], redirection_line[100];
    char * args[20];
    char * pipe_args[20];
    while(1){
        printf("sish:> ");
        int ret= read_line(line, redirection_line);
        split_line(line,args);  // parsing the line

        // execute the parsing line
        pid_t child_pid = fork();
        if(child_pid >0){
            if(background==1){
                bgp++;
                printf("[%d]%d\n",bgp,child_pid);
                if(waitpid(-1,NULL,WNOHANG) &&bgp>1){
                    done++;
                    printf("[%d] Done\n",done);
                }
                background=0;
                continue;
            }
            else if(waitpid(-1,NULL,WNOHANG) &&done<bgp){
                done++;
                printf("[%d]+Done\n",done);
                if(done==bgp){
                    done=0;
                    bgp=0;
                }
            }
            else
                waitpid(child_pid,NULL,0);
        }
        else if(child_pid == 0){

            if (ret == 3){
                pid_t pipe_child_pid = fork();

                if(pipe_child_pid == 0){
                    dup2( open("pipeline", O_RDWR | O_TRUNC | O_CREAT, 777), 1);
                    int err = execvp(args[0],args);
                    if (err == -1)
                        error_massage();
                }
                else{
                    waitpid(pipe_child_pid,NULL,0);
                    split_line(redirection_line, args);
                    dup2( open("pipeline", O_RDONLY, 777), 0);
                    int err = execvp(args[0],args);
                    if (err == -1)
                        error_massage();
                }
//                char * rmv_pipe[] = { "rm", "pipeline\0"};
//                execvp("rm", rmv_pipe);
                exit(0);
            }

            if (ret != -1){
                int file;
                if(ret == 2){
                    file = open(redirection_line, O_RDWR | O_APPEND | O_CREAT, 777);
                    ret = 1;
                }
                else if (ret == 1)
                    file = open(redirection_line, O_RDWR | O_TRUNC | O_CREAT, 777);
                else if(ret == 0)
                    file = open(redirection_line, O_RDONLY | O_CREAT, 777);
                dup2(file, ret);
            }
            if(ret != 3){
                int err = execvp(args[0], args);
                if(err == -1)
                     error_massage();

            }
        }
    }


    return 0;
}
