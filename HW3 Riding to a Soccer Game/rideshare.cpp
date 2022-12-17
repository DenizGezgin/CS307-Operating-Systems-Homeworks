#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <semaphore.h>
#include <vector>
#include <pthread.h>

using namespace std;

vector<pthread_t> threads;

pthread_barrier_t barrier;
sem_t sem, semPrint;
sem_t sem_A_waiters, sem_B_waiters;
int A_waiters = 4, B_waiters = 4;

void* thread_func(void* arg)
{
    // wait on the semaphore
    sem_wait(&sem);
    sem_t *sem_Friends, *sem_Others;
    char team = *((char*)arg);
    bool isDriver = false;
     
    printf("Thread ID: %ld, Team: %c, I am looking for a car\n", pthread_self(), team);
    int *num_friends_wait, *num_other_wait;
    if (team == 'A') {
        sem_Friends = &sem_A_waiters;
        sem_Others = &sem_B_waiters;
        num_friends_wait = &A_waiters;
        num_other_wait = &B_waiters;
    } else {
        sem_Friends = &sem_B_waiters;
        sem_Others = &sem_A_waiters;
        num_friends_wait = &B_waiters;
        num_other_wait = &A_waiters;
    }

    //cout << (A_waiters) << " | " << (B_waiters) << endl;

    //if number of waiting friends and other's enough form a car
    if((*num_friends_wait) < 4 && (*num_other_wait) < 3) {
        isDriver = true; 
        //Relase sem for friends for other friend in the car
        sem_post(sem_Friends);
        //relase sem for others for 2 other team in the car
        sem_post(sem_Others);
        sem_post(sem_Others);

        (*num_friends_wait)+=1;
        (*num_other_wait)+=2;
    } else if((*num_friends_wait) < 2) { //if number of friends waiting is enough form a car
        isDriver = true; 
        //Relase sem for friends for 3 other friend in the car so they can pass wait
        sem_post(sem_Friends);
        sem_post(sem_Friends);
        sem_post(sem_Friends);
        (*num_friends_wait)+=3;
    } else {
        //waiter job done relase main lock so others can continue
        (*num_friends_wait)--;
        //cout << (A_waiters) << " | " << (B_waiters) << endl;
        sem_post(&sem);
        //wait for somebody to form a car and free semaphores for other's in car
        sem_wait(sem_Friends);
    }
    sem_wait(&semPrint);
    printf("Thread ID: %ld, Team: %c, I have found a spot in a car\n", pthread_self(), team);
    sem_post(&semPrint);
    pthread_barrier_wait(&barrier);
    if(isDriver)
    {
        printf("Thread ID: %ld, Team: %c, I am the captain and driving the car\n", pthread_self(), team);
        //replace barrier
        pthread_barrier_destroy(&barrier); 
	    pthread_barrier_init(&barrier, NULL, 4); 
        sem_post(&sem);
    }
}

int main(int argc, char* argv[])
{
    //Check if the number of arguments are corret
    if (argc < 2) {
        //cout << "Usage: " << argv[0] << " [teamA] [teamB]" << endl;
        return 1;
    }
    int teamA = atoi(argv[1]);
    int teamB = atoi(argv[2]);
    int total = teamA + teamB;

    //Each group size must be an even number.
    if(!(teamA % 2 == 0 && teamB % 2 == 0))
    {
        //cout << "At least one group size is not even bye..." << endl;
        printf("The main terminates\n");
        return 1;
    }
    //Total number of supporters must be a multiple of four
    if(!((total) % 4 == 0))
    {
        //cout << "Total number of supporters is not a multiple of four bye..." << endl;
        printf("The main terminates\n");
        return 1;
    }


    // initialize the barrier with the number of threads that will wait on it
    pthread_barrier_init(&barrier, NULL, 4);

    // initialize the lock semaphore 1 can pass by defult
    sem_init(&sem, 0, 1);
    sem_init(&semPrint, 0, 1);
    // initialize the team semaphore no one can pass wait by defult only after driver posts teams can pass
    sem_init(&sem_A_waiters, 0, 0);
    sem_init(&sem_B_waiters, 0, 0);

    // create the threads here
    for(int i = 0; i < teamA; i++)
    {
        char* team = strdup("A");
        pthread_t thread;
        pthread_create(&thread, NULL, thread_func, team);
        threads.push_back(thread);
    }
    for(int i = 0; i < teamB; i++)
    {
        char* team = strdup("B");
        pthread_t thread;
        pthread_create(&thread, NULL, thread_func, team);
        threads.push_back(thread);
    }

    // Wait for all threads to finish
    for (int i = 0; i < threads.size(); i++) {
        pthread_join(threads[i], NULL);
    }

    // destroy the barrier
    pthread_barrier_destroy(&barrier);

    printf("The main terminates\n");
    return 0;
} 