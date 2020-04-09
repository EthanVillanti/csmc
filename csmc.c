#include <stdio.h>
#include <stdlib.h> 
#include <unistd.h>
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
void *student_thread(void *studentID); // track these id's, will 
void *tutor_thread(void *tutorID); // have to print later

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
int studentPriority[MAX_STUDENTS];
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
		exit(1); // exit(non zero) because error
	}
	// remember: argv[0] is not first arg, argv[1] is
  // correct number of command line arguments, assign to variables
  studentNum = atoi(argv[1]);
  tutorNum = atoi(argv[2]);
  chairNum = atoi(argv[3]);
  helpNum = atoi(argv[4]);
  
  // validate number of students and number of tutors 
  if(studentNum > MAX_STUDENTS || tutorNum > MAX_STUDENTS) { 
    printf("Can have a maximum of %d students, and %d tutors", MAX_STUDENTS);
    exit(1); // exit(non zero) because error
  }
  // input read in and validated
  
  // initialize semaphores 
  sem_init(&sem_student, 0, 0);
  sem_init(&sem_coordinator, 0, 0); 
  
  // initialize locks
  pthread_mutex_init(&seatLock, NULL);
  pthread_mutex_init(&queueLock, NULL);
  pthread_mutex_init(&readyTutorQueueLock, NULL);
  
  // allocate space for threads
  pthread_t students[studentNum];
  pthread_t tutors[tutorNum];
  pthread_t coordinator; // only 1 coordinator
  
  // initialize arrays w dummy vals
  int i; // declare i before for loops bc C is weird
  for(i = 0; i < studentNum; i++) { 
    newStudentQueue[i] = -1;
    readyTutorQueue[i] = -1;
    pq[i][0] = -1;
    pq[i][1] = -1;
    studentPriority[i] = 0;
  }
  
  // create student threads
  for(i = 0; i < studentNum; i++) { 
    // need to create a new thread for every i (every student)
    studentIDArray[i] = i+1; // offset due to student 1 being at array[0]
    // create thread
    assert(pthread_create(&students[i], NULL, student_thread, (void*) & studentIDArray[i]) == 0);
    // assert() will do nothing if thread cannot be properly created (returns nonzero)
  }
  
  // create tutor threads
  for(i = 0; i < tutorNum; i++) {
    // same logic as for loop with students
    tutorIDArray[i] = i + studentNum + 1;
    assert(pthread_create(&tutors[i], NULL, tutor_thread, (void*) & tutorIDArray[i]) == 0);
  }
  
  //create coordinator thread
  assert(pthread_create(&coordinator, NULL, coordinator_thread, NULL) == 0); 
  
  //threads created, need to join them 
  // need loops for student and tutor threads
  //student threads
  for(i = 0; i < studentNum; i++) { 
    pthread_join(students[i], NULL);
  }
  // tutor threads
  for(i = 0; i < tutorNum; i++) { 
    pthread_join(tutors[i] ,NULL);
  }
  // single coordinator thread
  pthread_join(coordinator, NULL);

} // END OF MAIN

//////////////////////////////////////////////////
//////////      thread code        ///////////////
//////////////////////////////////////////////////

// student thread
// check for empty seat
// if found, sit and tell cordinator
// wait for tutor to free up
void *student_thread(void *studentID) {
  int myID = *(int*)studentID; 
  
  // forever loop
  while(1) {  
    if(studentPriority[myID-1] >= helpNum) { 
      // lock the seat
      pthread_mutex_lock(&seatLock);
      totalFinished++;
      pthread_mutex_unlock(&seatLock);
      
      // signal to release
      sem_post(&sem_student);
      
      // done
      pthread_exit(NULL);
    }
    
    // sleep for some time (max 2 ms)
    float sleepy = (float)(rand()%200)/100;
    usleep(sleepy);
    
    // lock seat and try again
    pthread_mutex_lock(&seatLock);
    // check if empty chair 
    if(occupiedChairNum >= chairNum) { 
      // print unable to find chair
      printf("St: Student %d found no empty chair. Will try again later\n", myID);
      // unlock seat
      pthread_mutex_unlock(&seatLock);
      continue; // skip rest of loop, unable to find a chair
    }
    // student found a chair
    // incremembt number of occupied chairs
    occupiedChairNum = occupiedChairNum + 1;
    // increment requests
    totalRequests = totalRequests + 1;
    newStudentQueue[myID -1] = totalRequests;
    // student is able to take a seat
    // print statement
    printf("St: Student %d takes a seat. Empty chairs = %d.\n", myID, chairNum - occupiedChairNum); // empty chairs is total chairs - occupied chairs 
    // unlock seat
    pthread_mutex_unlock(&seatLock);
    
    //post on student, tells coordinator one more student is seated
    sem_post(&sem_student);
    
    // seated in waiting room, wait for tutor
    while(readyTutorQueue[myID - 1] == -1); // wait until tutor[myid-1] is ready
    // after above line, student has been helped
    // last print from student thread
    printf("St: Student %d received help from Tutor %d.\n", myID, readyTutorQueue[myID - 1] - studentNum);
    
    
    // student thread operations complete
    // reset any data used between threads
    // lock readyTutorQueue
    pthread_mutex_lock(&readyTutorQueueLock);
    readyTutorQueue[myID - 1] = -1; // reset to init value
    pthread_mutex_unlock(&readyTutorQueueLock);
    
    // increment student priotity 
    pthread_mutex_lock(&seatLock);
    studentPriority[myID - 1]++; // student has completed one more tutor session, increment priority to represent another visit to csmc
    pthread_mutex_unlock(&seatLock);
  
  } // end of forever loop

} // end of student thread


// tutor thread
// waiting until coordinator notifies student needs tutoring
// find student with appropriate priority 
// tutor
// after tutor, communicate to student (and coordinator?) that tutoring is complete 
void *tutor_thread(void *tutorID) { 
  int myTID = *(int*)tutorID; // tutor id
  int studentOrdering; // if same # times tutored, first come first serve
  int studentVisits; // number of times student has been to csmc
  int mySID; // student id
  
  while(1) { 
    // see if anyone needs tutoring
    if(totalFinished == studentNum) { 
      // every student has been tutored
      // terminate thread
      pthread_exit(NULL);
    }
  // if here, still students to be helped 
  studentVisits = helpNum -1;
  studentOrdering = studentNum * helpNum + 1;
  mySID = -1; // use -1 as a flag later to see if no one in queue
  // wait on coordinator until notified tutor help is needed
  sem_wait(&sem_coordinator);
  
  // once here, coordinator has notified to tutor
  pthread_mutex_lock(&seatLock);
  // tutor student with highest priority
  // if same priority, first come first serve
  int i; 
  for(i = 0; i < studentNum; i++) { 
    if(pq[i][0] > -1 && pq[i][0] <= studentVisits && pq[i][1] < studentOrdering) { 
    // first checks to see if pq has been changed from init value (-1)
    // second checks if pq is higher priority 
    studentVisits = pq[i][0]; 
    studentOrdering = pq[i][1];
    mySID = studentIDArray[i]; // assign studend with highest priority to be tutored
    }
  }
  // make sure mySID is changed from init val
  if(mySID == -1) { // no students in queue 
    // printf("no student in queue)" COMMENT OUT BEFORE SUBMISSION
    pthread_mutex_unlock(&seatLock);
    continue; //break loop, following operations not needed
  
  }
  
  // reset pq to init vals
  pq[mySID - 1][0] = -1;
  pq[mySID - 1][1] = -1;
  
  // student tutoring, one more chair in waiting room ready
  occupiedChairNum = occupiedChairNum - 1;
  // student is currently receiving tutoring
  tutoringNow = tutoringNow + 1;
  // done with seats for now, unlock so other students can sit
  pthread_mutex_unlock(&seatLock);
  
  // sleep for up to .2 ms to emulate tutoring
  float sleepy = (float)(rand()%200)/1000;
  usleep(sleepy);
  
  // done tutoring
  pthread_mutex_lock(&seatLock);
  tutoringNow = tutoringNow -1; 
  // increment total sessions
  totalSessions = totalSessions + 1;
  
  // done tutoring, printf statement
  printf("Tu: Student %d tutored by Tutor %d. Students tutored now = %d. Total sessions tutored = &d\n", mySID, myTID, tutoringNow, totalSessions); // myTID is funky
  
  pthread_mutex_unlock(&seatLock);
  
  // let student know who tutor was for print statements inside of student thread
  pthread_mutex_lock(&readyTutorQueueLock);
  readyTutorQueue[mySID - 1] = myTID;
  pthread_mutex_unlock(&readyTutorQueueLock);
  
  } // end of forever loop

} // end of tutor thread

// coordinator thread
// wait until student needs help 
// put student in appropriate position in queue
// communicate to tutor
void *coordinator_thread() { 

  while(1) { 
    // check for everyone being tutored
    if(totalFinished == studentNum) { 
      // terminate coordinator and tutor threads
      // and exit
      int i;
      // terminating tutor threads
      for(i = 0; 0 < tutorNum; i++) { 
        sem_post(&sem_coordinator);
      }
      // terminate coordinator thread
      pthread_exit(NULL); 
    } 
    // if here, still students to tutor
    // wait until student requests help 
    sem_wait(&sem_student);
    
    // lock seat before moving
    pthread_mutex_lock(&seatLock);
    int i;
    for(i = 0; i < studentNum; i++) { 
      if(newStudentQueue[i] > -1) { // changed from init
        pq[i][0] = studentPriority[i]; // change priority
        pq[i][1] = newStudentQueue[i]; // and put student to front
        
        // print statement
        printf("Co: Student %d with priority %d in the queue. Waiting students now = %d. t Total requests = %d\n", studentIDArray[i], studentPriority[i], occupiedChairNum, totalRequests);
        // back to init val
        newStudentQueue[i] = -1;
        
        
        // student in queue, assign tutor
        sem_post(&sem_coordinator);
      }
    
    }
    // out of cs, unlock seats
    pthread_mutex_unlock(&seatLock);

  } // end of forever loop
} // end of coordinator thread





