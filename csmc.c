#include <stdio.h>
#include <stdlib.h> 
#include <unist.h>
#include <string.h>

#include <pthread.h>
#include <semaphore.h>
#include <assert.h> 

// macro for max students 
#define MAX_STUDENTS 2000

// shared variables for print statements inside of different threads
int totalFinished = 0; // completed sessions
int totalRequests = 0; // total session requests, for coordinator threads output 
int totalSessions = 0; // done and current sessions, for tutor threads output 
int tutoringNow = 0; // current sessions,for tutor threads output

// thread defs
void *student_thread(void *student_id); // track these id's, will 
void *tutor_thread(void *tutor_id); // have to print later

void *coordinator_thread(); // only 1 coordinator, no need for id

int studentNum = 0; 
int tutorNum = 0;
int helpNum = 0;
int chairNum = 0;
int occupiedChairNum = 0;

// arrays
int newStudentQueue[MAX_STUDENTS]; // queue of students newly arrived 
int readyTutorQueue[MAX_STUDENTS]; // queue of tutors finished with a session
int pq[MAX_STUDENTS][2]; // priority queue with students and their matching priority
// REMEMBER LOWER VALUE OF [i][1] MEANS HIGHER PRIORITY 
int studentIDArray[MAX_STUDENTS];
int tutorIDArray[MAX_STUDENTS];

// semaphores
sem_t sem_student;
sem_t sem_coordinator;
// sem_t sem_tutor;     --- dont think we need ---
// locks
pthread_mutex_t seatLock; // don't want 2 student threads getting same seat
pthread_mutex_t queueLock; 
pthread_mutex_t readyTutorQueueLock; // lock free tutors when assigning to student 

// end of global variables and function/thread definitions


int main(int argc, const char * argv[]) { 
	// validate number of args
	if(argc != 5) { 
		printf("incorrect number of inputs");
		printf("correct input: [#students],[#tutors],[#chairs],[#timesHelped]");
		exit(0);
	}
	// remember: argv[0] is not first arg, argv[1] is











} // END OF MAIN
