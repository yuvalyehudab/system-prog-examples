/* functions QA automation comments valgrind */

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define PIPE_CONST 1
#define BACK_CONST 2
#define NONE_CONST 3

/**********************************************************************************************
* okay, so...
*********************************** GENERAL ***************************************************
*
* This is a simple shell program. Skeleton that reads and parses commands are provided by
* course staff. We were asked to implement 3 functions: prepare - the first function executed
* by the shell, finalize() - last one and process_arglist(), which responsible for inside 
* business logic. This means executing the commands that the shell gets and extra functionality:
* background processes and pipes.
*
************************************* IDEA *****************************************************
* process_arglist():
* 1. Check if the command has &. If it does, mark it as 'background process'.
* 2. Check if the command contains a pipe ('|'), and create pipe and mark it if it does.
* 3. Fork: create another copy of the program
*   3.1 in son process:
*       3.1.1 if it was marked as a pipe, put the file descriptor of pipe instead stdout
*       3.1.2 if it was marked as background process, make it ignore SIGINT
*       3.1.3 execute the command
*       3.1.4 if exectue fails, exit.
*   3.2 in parent process:
*       3.2.1 in background process - return. go get another command...
*       3.2.2 in pipe - fork to create another process, assign file descriptor of pipe instead
*           of file descriptor of std in so the process reads from pipe, execute second command.
*       3.3.3 if we are here, its a foreground process. wait until it finishes
*
************************************************************************************************/

/*action: is it pipe, background or foreground*/
/*godfather_pid: pid of shell main parent. useful for checking parenthood in sig handler*/
int action, godfather_pid;

/*
* 
* returned value
* success: 0
* failure: -1
* description
* check for pipe/background process by iteratig through arglist
* fix arglist for proper running in exec, and for pipe - open it and save place of second command
* use
* first in process_arglist, set all values for use of other functions
* 
*/
int initialize(int* indent, int* action_holder, int* fd, char** args, int count){
    /*default values*/
    *indent = 0;
    *action_holder = NONE_CONST;
    /*if: last arg is not NULL, AND it is '&'. this means that it will be background process.*/
    if ((args[count - 1] != NULL) && (args[count - 1][0] == '&') && (args[count - 1][1] == '\0')){
        /*set value of action to the action required by user*/
        *action_holder = BACK_CONST;
        /*set last memebr of arglist to NULL instead of &, so that the command will run properly*/
        args[count - 1] = NULL;
        /*no more business here. get out*/
        return 0;
    }
    int i;
    /*check all strings to find pipe*/
    for (i = 0; i < count - 1; i++){
        if (args[i][0] == '|' && args[i][1] == '\0'){
            /*founded!*/
            /*set value of action*/
            *action_holder = PIPE_CONST;
            /*the 'second' array starts here (the second command). save this place for running 
            the second command later*/
            *indent = i;
            /*partition of array + fixing it for exec format*/
            args[i] = NULL;
            /*make a pipe*/
            if (pipe(fd) == -1){
                perror("Did not pipe well");
                return -1;
            }
        }
    }
    return 0;
}
/**/
int perform_action_child(int action, int* fd){
    /*first case: pipe*/
    if (action == PIPE_CONST){
        /*put pipe writing end where you save the file desc of output of program
        in simple words - write to pipe instead of writing to stdout*/
        if (dup2(fd[1], STDOUT_FILENO) == -1){
            perror("Could not pipe well");
            close(fd[1]);
            close(fd[0]);
            exit(1);
        }
        close(fd[1]);/*write*/
        close(fd[0]);/*read*/
        return 0; /*we finished this case*/
    }
    else if (action == BACK_CONST){
        /*second case: background process*/
        /*different signal handler*/
        struct sigaction tmp;
        tmp.sa_handler = SIG_IGN;/*ignored sig handler should be inherited according to the manual*/
        /*set si handler for ignore sigint*/
        if (sigaction(SIGINT, &tmp, NULL) == -1){
            perror("Could not sign signal handler properly");
            exit(1);
        }
    }
    return 0;
}
/**/
int perform_action_parent_pipe(int action, int* fd, char** arglist, int arg_indent, int pid){
    if (action != PIPE_CONST){
        /*there is only one process, and it is a foreground one. wait for him.*/
        return pid;
    }
    /*we are in pipe command*/
    /*crate second child*/
    int second_pid = fork();
    if (second_pid == -1){
        perror("Did not fork again well");
        return 0;
    }
    /*creation succeeded*/
    else if (second_pid == 0){
        /*we are in second child's code*/
        /*same as the other side of the pipe*/
        /*read from pipe instead of reading from stdin*/
        if (dup2(fd[0], STDIN_FILENO) == -1){
            perror("Pipe error");
            exit(1);
        }
        /*close resources*/
        close(fd[0]); /*read*/
        close(fd[1]); /*write*/
        /*execute second command*/
        /*params: first string after pipe is command, and array of args is from that place*/
        execvp(arglist[arg_indent + 1], arglist + arg_indent + 1);
        perror("Did not execute well");
        exit(1);
    }
    close(fd[0]); close(fd[1]);
    /*wait for the first process to end*/
    waitpid(pid, NULL, 0);
    /*send the second for waiting*/
    return second_pid;
}
/**/
int process_arglist(int count, char** arglist){
    /*wrap all together*/
    int pid, arg_indent, fd[2];
    /*initialize args + check type of command - pipe/back/fore*/
    if (initialize(&arg_indent, &action, fd, arglist, count) == -1){
        return 1;
    }

    pid = fork();
    if (pid == 0){
        /*we are in child code*/
        /*make initialzations of children: change sig handler for background so it could be inherited
        and assign pipe writing side for pipe process*/
        perform_action_child(action, fd);
        /*exectue the command...*/
        execvp(arglist[0], arglist);
        perror("Error executing");
        exit(1);
    }
    else if (pid > 0){ /*parent*/
        if (action == BACK_CONST){
            /*process in background, dont wait for him*/
        	return 1;
        }
        int wait_pid;
        /*in pipe, fork, sign to the read end of the pipe and execute the relevant command*/
        /*in foreground processs, wait for it*/
        wait_pid = perform_action_parent_pipe(action, fd, arglist, arg_indent, pid);
        waitpid(wait_pid, NULL, 0);
    }
    else{/*error*/
        perror("Could not execute the command");
        return 0;
    }
    return 1;
}
/**/
void handle_sig_child(int signum, siginfo_t* info, void* ucontext){
    /*wait so it stops being zombie*/
	waitpid(info->si_pid, NULL, 0);
}
/**/
void handle_sig_int(int signum){
	int self_pid = getpid();
    /*check if we are in child process or in parent process*/
    /*remember thet godfather_pid is global in this file*/
	if (self_pid != godfather_pid){
        /*in child process*/
		if(action != BACK_CONST){
            /*this is not a background process*/
            /*this is a foreground process and it is not the shell but a command - exit in sigint*/
			exit(1);
		}
	}
}
/**/
int prepare(void){
	/*global var which being set from parent process to its pid.
    this way it is easy to see if some process is a child - getpid is always successful, by manual*/
	godfather_pid = getpid();
	/*sig child*/
	struct sigaction sig_child_handler;
	sig_child_handler.sa_sigaction = handle_sig_child;
	sig_child_handler.sa_flags = SA_RESTART | SA_SIGINFO | SA_NOCLDSTOP;
	if (sigaction(SIGCHLD, &sig_child_handler, NULL) == -1){
		perror("Error signing signals");
		exit(1);
	}
	/*sig int*/
	struct sigaction sig_int_handler;
	sig_int_handler.sa_handler = handle_sig_int;
	sig_int_handler.sa_flags = SA_RESTART;
	if (sigaction(SIGINT, &sig_int_handler, NULL)){
		perror("Error signing signal handler");
		exit(1);
	}
	return 0;
}
/*did not find anything to put here.*/
int finalize(void){
	return 0;
}
