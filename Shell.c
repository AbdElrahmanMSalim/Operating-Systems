#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

int background_flag;
int background_process = 0, done = 0;
char pipe_files[2][20] = { "pipeline_0\0", "pipeline_1\0" } ;


void error_massage(){
     printf("Please Enter A Valid Shell Command\n");
}

void remove_end_line(char line[]){
    int i =0;
    while(line[i] != '\n' && line[i] != '>' && line[i] != '<')
        i++;

    line[i] = '\0';
}

int detect_redirection(char line[], char redirection_file[]){
    int i = 0;
    int ret;

    while (line[i] != '\0' && line[i] != '>' && line[i] != '<' && line[i] != '|')
        i++;

    if (line [i] == '\0')   //No redirection
        return -1;
    if(line [i] == '|')     //Pipelining
        return 3;
    if (line[i] == '>'){
        if(line[i+1] == '>'){
            i++;
            ret = 2;
        }
        else
            ret = 1;
    }
    else
        ret = 0;

    i++;
    int j = 0;
    while (line [i] == ' ')
        i++;
    while (line[i] != '\0'){
        redirection_file[j] = line[i];
        i++;
        j++;
    }
    redirection_file[j-1] = '\0';   //removing end line from file name

    return ret;
}

// reading shell command line
int read_line(char line[], char redirection_file[]){
    int i = 0;
    char* command = fgets(line, 200, stdin);
    int ret = detect_redirection(line, redirection_file);
    while(line[i] != '\n')
        i++;
    if(line[i-1] == '&'){
        line[i-1] = '\n';
        background_flag = 1;
    }
    remove_end_line(line);
    if (strcmp(line,"exit") == 0 || command == NULL)
        exit(0);
    return ret;
}

// parsing line
int split_line(char line[], char* args[], char del[]){
    int i = 0;
    args[i] = strtok(line, del);

    if(args[i] == NULL){
        if (done != background_process)
            return 1;
        printf("NO COMMANDS !\n");
        return 1;
    }
    while(args[i] != NULL){
        i++;
        args[i] = strtok(NULL, del);
    }
    return 1;
}



//Converting non-alphanumeric characters to spaces
int checkForAlphanumericChars(char line[], char alphanumericChars[]) {

    int i = 0; //outer loop counter
    int isALPHANUMERIC = 0;

    while (line[i] != '\0') {
        int j = 0;  //inner loop counter

        while (alphanumericChars[j] != '\0') {

            if (line[i] == alphanumericChars[j]) {
                isALPHANUMERIC = 1;
                break;
            }

            j++;
        }

        // if it's not alphanumeric, put a space in its place
        if (isALPHANUMERIC == 0)
            line[i] = ' ';
        else
            isALPHANUMERIC = 0;

        i++;
    }
    return 1;
}

void remove_piping_files(){
    int i;
    for(i = 0; i < 2; i++){
        char * rmv_pipe[] = { "rm", pipe_files[i]};
        pid_t rmv_child_pid = fork();

        if(rmv_child_pid == 0)
            execvp("rm", rmv_pipe);
        else
            waitpid(rmv_child_pid, NULL, 0);
    }

}

void piping(char line[]){

    int file_ptr = 0;

    char * pipe_line[20];
    char * args[20];
    split_line(line, pipe_line, "|");
    int i = 0;

    while(pipe_line[i] != NULL){

        pid_t pipe_child_pid = fork();

        if(pipe_child_pid == 0){
            split_line(pipe_line[i], args, " ");

            dup2( open(pipe_files[!file_ptr], O_RDONLY | O_CREAT, 777), 0);
            dup2( open(pipe_files[file_ptr], O_RDWR | O_TRUNC | O_CREAT, 777), 1);

            int err = execvp(args[0],args);
            if (err == -1){
                error_massage();
//                exit(0);
            }
        }
        else {
            waitpid(pipe_child_pid, NULL, 0);
            i++;

            if(pipe_line[i+1] == NULL){         //the the last command in the pipeline
                pid_t rmv_child_pid = fork();

                if(rmv_child_pid == 0){
                    split_line(pipe_line[i], args, " ");

                    dup2( open(pipe_files[file_ptr], O_RDONLY, 777), 0);

                    int err = execvp(args[0],args);
                    if (err == -1){
                        error_massage();
//                        exit(0);
                    }
                }
                else{
                    waitpid(rmv_child_pid, NULL, 0);
                    remove_piping_files();
                    exit(0);
                }
            }
            file_ptr = !file_ptr;
        }
    }
}



int main()
{

    char line[200], redirection_file[30];
    char * args[20];
    char * pipe_args[20];
    char alphanumericChars[] = "1234567890QWERTYUIOPLKJHGFDSAZXCVBNMqwertyuioplkjhgfdsazxcvbnm -'_<>|&";
    while(1){

        printf("sish:> ");
        int ret = read_line(line, redirection_file);
        checkForAlphanumericChars(line, alphanumericChars); //Converting non-alphanumeric characters to spaces
        if(ret != 3)        //parsing in pipelining is handled separately
            split_line(line, args, " ");  // parsing the line
        // execute the parsing line
        pid_t child_pid = fork();

        if(child_pid > 0){

            if(background_flag == 1){
                background_process++;
                printf("[%d]%d\n", background_process, child_pid);
                if(waitpid(-1, NULL, WNOHANG) && background_process > 1){
                    done++;
                    printf("[%d] Done\n",done);
                }

                background_flag = 0;
                continue;
            }

            else if(waitpid(-1, NULL, WNOHANG) && done < background_process){
                done++;
                printf("[%d]+Done\n",done);
                if(done == background_process){
                    done = 0;
                    background_process = 0;
                }
            }
            else
                waitpid(child_pid, NULL, 0);
        }
        else if(child_pid == 0){
            if (ret == 3)   //pipelining happened
                piping(line);

            if (ret != -1){     //redirection happened
                int file;
                if(ret == 2){       //appending
                    file = open(redirection_file, O_RDWR | O_APPEND | O_CREAT, 777);
                    ret = 1;
                }
                else if (ret == 1)      //overwriting
                    file = open(redirection_file, O_RDWR | O_TRUNC | O_CREAT, 777);
                else if(ret == 0)       //reading
                    file = open(redirection_file, O_RDONLY | O_CREAT, 777);
                dup2(file, ret);
            }
            int err = execvp(args[0], args);
            if (err == -1)
                error_massage();
        }
    }

    return 0;
}
