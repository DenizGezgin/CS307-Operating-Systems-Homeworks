#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/wait.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <pthread.h>
#include <cstdio>
#include <vector>

using namespace std;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
std::vector<pthread_t> threads; // vector of threads
std::vector<pid_t> forks;

//Command line data structure
struct CliLine
{
    string command;
    string input;
    string option;
    string redirectionSymbol;
    string redirectionFname;
    bool isBackgroundJob;
    int commandLenght;
};

std::vector<CliLine> codeLines; // vector of cli lines

//Shell's thread for printing
void *shellCommandThreadFunction(void *arg){
    pthread_mutex_lock(&lock);
    int pipefd = *((int *)arg);
    cout << "---- " << pthread_self() << endl;

    // Open a file stream for the pipe
    FILE *fp = fdopen(pipefd, "r");
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
      printf("%s", buffer); // print the read data to the console
      fflush(stdout);
    }
    

    // Close the file stream when finished(buggy?)
    //fclose(fp);

    cout << "---- " << pthread_self() << endl;
    pthread_mutex_unlock(&lock);
    return NULL;
}

void writeCliLineToFile(CliLine cliLine, ofstream& outfile)
{
    // Write to the file using the same format as before
    outfile << "----------" << endl;
    outfile << "Command: " << cliLine.command << endl;
    outfile << "Inputs: " << cliLine.input << endl;
    outfile << "Options: " << cliLine.option << endl;
    outfile << "Redirection: " << cliLine.redirectionSymbol << endl;
    if(cliLine.isBackgroundJob)
        outfile << "Background Job: " << "y" << endl;
    else
        outfile << "Background Job: " << "n" << endl;
    outfile << "----------" << endl;
}
    
int main() {

    ifstream myCommandFile("commands.txt");
    ofstream outfile;
    outfile.open("parse.txt");

    string myLine;
    string myWord;
    int numBackgroundJobs = 0;
    while (getline (myCommandFile, myLine)) {
        stringstream lineStream(myLine);
        CliLine myCliLine;
        
        bool isFirstInLine = true;
        bool isNextRedirectionFile = false;
        myCliLine.isBackgroundJob = false;
        myCliLine.redirectionSymbol = "-";
        myCliLine.commandLenght = 0;
        //Parse line increment command lenght as new words added
        while(lineStream >> myWord)
        {
            if(isFirstInLine) {
                myCliLine.command = myWord;
                myCliLine.commandLenght++;
                isFirstInLine = false;
            } else if(isNextRedirectionFile) {
                myCliLine.redirectionFname = myWord;
                myCliLine.commandLenght++;
                isNextRedirectionFile = false;
            } else if(myWord[0] == '-') {
                myCliLine.option = myWord;
                myCliLine.commandLenght++;
            } else if(myWord[0] == '>' || myWord[0] == '<') {
                myCliLine.redirectionSymbol = myWord;
                myCliLine.commandLenght++;
                isNextRedirectionFile = true;
            } else if(myWord[0] == '&') {
                myCliLine.isBackgroundJob = true;
            } else {
                myCliLine.input = myWord;
            }
        }
        writeCliLineToFile(myCliLine, outfile);
        codeLines.push_back(myCliLine);
    }
    //After done parsing execute commands
    for (int i = 0; i < codeLines.size(); i++) {
        CliLine myCliLine = codeLines[i];

        //Do wait
        if(myCliLine.command == "wait")
        {
            // Wait for all the child processes to finish
            for (int i = 0; i < forks.size(); i++) {
                int status;
                waitpid(forks[i], &status, 0);
            }

            // Wait for all threads to finish
            for (int i = 0; i < threads.size(); i++) {
                pthread_join(threads[i], NULL);
            }
            int status;
            while ( waitpid(-1, &status, WNOHANG) >= 0)
            {
                int status;
                wait(NULL);
            }
            numBackgroundJobs = 0;
            //skip rest of the line
            continue;
        }



        if (myCliLine.redirectionSymbol == ">")
        {
            pid_t pid = fork();
            if (pid == 0) {
                // child: exec command
                
                //Do run command
                int arrayLenght = myCliLine.commandLenght  + 1;
                int i = 0;
                char *myargs[arrayLenght];
                myargs[i] = strdup(myCliLine.command.c_str());
                i++;
                if(!myCliLine.input.empty()){
                    myargs[i] = strdup(myCliLine.input.c_str()); 
                    i++;
                }
                if(!myCliLine.option.empty())
                {
                myCliLine.option = myCliLine.option;
                myargs[i] = strdup(myCliLine.option.c_str()); 
                i++;
                }
                FILE *file = fopen(myCliLine.redirectionFname.c_str(), "w");
                dup2(fileno(file), STDOUT_FILENO);
                        	
                myargs[i] = NULL;     	
                execvp(myargs[0], myargs);
            }
            else{
                //Mark as background and wait                     
                if (myCliLine.isBackgroundJob){
                    forks.push_back(pid);
                    numBackgroundJobs++;  
                    continue;
                }
                
                int status;
                waitpid(pid, &status, 0);
            }
        }
        else
        {
            int *pipefd = (int*)malloc(sizeof(int)*2);
            pipe(pipefd); 
            pthread_t thread_id;

            pid_t pid = fork();
            if (pid == 0) {
                // child: exec command
                            
                // Close the unused end of the pipe
                close(pipefd[0]);
                // Open a file stream for the pipe
                FILE *fp = fdopen(pipefd[1], "w");
                dup2(fileno(fp), STDOUT_FILENO);                

                //do run
                int arrayLenght = myCliLine.commandLenght  + 1;
                int i = 0;
                char *myargs[arrayLenght];
                myargs[i] = strdup(myCliLine.command.c_str());
                i++;
                if(!myCliLine.input.empty()){
                    myargs[i] = strdup(myCliLine.input.c_str()); 
                    i++;
                }
                if(!myCliLine.option.empty())
                {
                    myCliLine.option = myCliLine.option; 
                    myargs[i] = strdup(myCliLine.option.c_str()); 
                    i++;
                }
                if(myCliLine.redirectionSymbol == "<"){
                    //Input type file
                    FILE *file = fopen(myCliLine.redirectionFname.c_str(), "r");
                     dup2(fileno(file), STDIN_FILENO);
                }

                myargs[i] = NULL;      	
                execvp(myargs[0], myargs);
            }
            else{
                close(pipefd[1]);
                pthread_create(&thread_id, NULL, shellCommandThreadFunction, (void *)&pipefd[0]);
                //threads.push_back(thread_id);

                if (myCliLine.isBackgroundJob){
                    threads.push_back(thread_id);
                    forks.push_back(pid);
                    numBackgroundJobs++;  
                    continue;
                }
                else
                {
                    int status;
                    waitpid(pid, &status, 0);
                    pthread_join(thread_id, NULL);
                }  
            }
        }
    }
        
        
    // Wait for all the child processes to finish
    for (int i = 0; i < numBackgroundJobs; i++) {
        int status;
        waitpid(-1, &status, 0);
    }
    int status;
    while ( waitpid(-1, &status, WNOHANG) >= 0)
    {
        int status;
        wait(NULL);
    }
   // Wait for all threads to finish
    for (int i = 0; i < threads.size(); i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}