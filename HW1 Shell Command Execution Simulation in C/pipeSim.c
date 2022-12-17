#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>

int
main(int argc, char *argv[])
{
    printf("I’m SHELL process, with PID: %d - Main command is: man ping | grep \"\\-A\" -A2 > output.txt \n", (int) getpid());
    int status;
    int fd[2];
    pipe(fd);
	
    int rc1 = fork();
    
    if (rc1 < 0) {
        // fork failed; exit
        fprintf(stderr, "a fork failed\n");
        exit(1);
    } else if (rc1 == 0) {
		// child: exec man and write to cout
        printf("I’m MAN process, with PID: %d - My command is: man ping\n", (int) getpid());
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);

		// now exec "man"...
        char *myargs[2];
        myargs[0] = strdup("man");	
        myargs[1] = strdup("ping"); 	
	    myargs[2] = NULL;        	
        execvp(myargs[0], myargs);  	
    } else {
        int rc2 = fork();
        if (rc1 < 0) {
            // fork failed; exit
            fprintf(stderr, "a fork failed\n");
            exit(1);
        }
        else if (rc2 == 0) {
		    // child: wait for c1 than read from cin and exec grep
            waitpid(rc1,&status,0);
            printf("I’m GREP process, with PID: %d - My command is: grep \"\\-A\" -A2 > output.txt\n", (int) getpid());
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);

            close(STDOUT_FILENO);
            open("./output.txt", O_RDWR | O_CREAT | O_TRUNC, 0777 );

            // now exec "grep"...
            char *myargs[3];            
            myargs[0] = strdup("grep");	
            myargs[1] = strdup("\\-A"); 
	        myargs[2] = strdup("-A2");  
            myargs[3] = NULL;           
            execvp(myargs[0], myargs);  
            printf("Done");             
        }
        else{
            // parent goes down this path (shell process)
            wait(NULL);
            printf("I’m SHELL process, with PID: %d - execution is completed, you can find the results in output.txt\n", (int) getpid());
        }   
    }
    return 0;
}