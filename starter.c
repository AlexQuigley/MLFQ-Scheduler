

 
// Initial code to build your program off of.
// Just I left content in a struct/function, doesn't mean you shouldn't add more to it.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Aliases for the struct types
typedef struct process Process;
typedef struct queue Queue;
typedef struct node Node;

struct node {
    Node *next;     // Pointer to the next node in the queue
    Process *P;     // Pointer to a process stored in this node
};

struct process {
    char name;          // Name of the process
    int arrivalTime;    // Process Arrival Time
    int workRemaining;  // Ammount of Work left to do for the process
    int timeInQueue;
    int totalTimeUsed;
    int responseTime;
    int hasStarted;
};

struct queue {
    int timeSlice;
    int timeAllotment;
    int size;
    Node *head;         // Head of the linked list
    Node *tail;         // Tail of the linked list
};

// Function to add a process node to the queue
void enqueue(Queue *Q, Node *N) {
    if (Q->size == 0) {
        Q->head = Q->tail = N;
    } else {
        Q->tail->next = N;
        Q->tail = N;
    }
    N->next = NULL;
    Q->size++;
}

Node* dequeue(Queue *Q) {
    if (Q->size == 0) return NULL;

    Node *tmp = Q->head;
    Q->head = Q->head->next;
    Q->size--;

    if (Q->size == 0) Q->tail = NULL;

    tmp->next = NULL;
    return tmp;
}


// Function to get the first process in a queue without removing it
Process* peek(Queue *Q) {
    if (Q->size == 0) return NULL;
    return Q->head->P;
}


//I wouldn't recommend modifing this at all.
void printProcessRan(int queueNum, char processName)  {
    printf("%d %c\n", queueNum, processName);
}

//I wouldn't recommend modifing this at all.
void printProcessCompleted(char processName, int responseTime, int turnaroundTime)  {
    printf("%c %d %d\n", processName, responseTime, turnaroundTime);
}

//I wouldn't recommend modifing this at all.
void printAllFinished(int endTime, int avgResponseTime, int avgTurnaroundTime)  {
    printf("%d %d %d\n", endTime, avgResponseTime, avgTurnaroundTime);
}

int main(int argc, char* argv[]) // Setup to take cmd line arguments
{
    Node *nextToArrive = NULL;  //Have a queue of programs which 'haven't arrived yet', ordered by their arrival times.
    int remainingProcesses = 0; //Processes which still haven't completed

    // Ensure at least 3 arguments are provided before accessing argv[2]
    if (argc < 3) {
    fprintf(stderr, "Usage: %s <T> <numberOfQueues> <queue1_timeSlice> <queue1_allotment> ... <process_arrival> <process_work>\n", argv[0]);
    return 1;
    }

    // Read system-wide time period T
    int timePeriod;
    sscanf(argv[1], "%d", &timePeriod);

    // Read number of queues
    int numberOfQueues;
    sscanf(argv[2], "%d", &numberOfQueues);

    //create the queues
    Queue queue[numberOfQueues];
    for(int i = 1; i<=numberOfQueues; i++) {
        sscanf(argv[i*2+1], "%d", &(queue[i-1].timeSlice));
        sscanf(argv[i*2+2], "%d", &(queue[i-1].timeAllotment));
        queue[i-1].size = 0;
        queue[i-1].head = NULL;
        queue[i-1].tail = NULL;
    }

    //create the processes
     int numberOfProcesses = (argc - 3 - numberOfQueues*2) / 2;
    Node *arrivalQueue = NULL; // Sorted list of arriving processes
    for (int i = 0; i < numberOfProcesses; i++) {
        Node *NP = malloc(sizeof(Node));
        Process *P = malloc(sizeof(Process));
        NP->P = P;

        sscanf(argv[i*2 + numberOfQueues*2 + 3], "%d", &(P->arrivalTime));
        sscanf(argv[i*2 + numberOfQueues*2 + 4], "%d", &(P->workRemaining));
        
        P->name = 'A' + i;
        P->totalTimeUsed = 0;
        P->timeInQueue = 0;
        P->hasStarted = 0;
        P->responseTime = -1;

        // Insert into arrivalQueue (sorted by arrivalTime)
        if (!arrivalQueue || P->arrivalTime < arrivalQueue->P->arrivalTime) {
            NP->next = arrivalQueue;
            arrivalQueue = NP;
        } else {
            Node *iter = arrivalQueue;
            while (iter->next && iter->next->P->arrivalTime <= P->arrivalTime)
                iter = iter->next;
            NP->next = iter->next;
            iter->next = NP;
        }
        remainingProcesses++;
    }


    int time = 0;
    int totalResponseTime = 0;
    int totalTurnaroundTime = 0;
    
     // **Main Scheduling Loop**
    while (remainingProcesses > 0) {
        // Process arrivals
        while (arrivalQueue && arrivalQueue->P->arrivalTime == time) {
            Node *arrived = arrivalQueue;
            arrivalQueue = arrivalQueue->next;
            enqueue(&queue[0], arrived);
        }

        // Find the highest priority queue with processes
        int h = 0;
        while (h < numberOfQueues && queue[h].size == 0) {
            h++;
        }
        if (h == numberOfQueues) {
            time++; // No processes to run, move time forward
            continue;
        }

        // Get the process at the front of queue h
        Node *runningNode = dequeue(&queue[h]);
        Process *runningProcess = runningNode->P;

        if (runningProcess->responseTime == -1)
            runningProcess->responseTime = time - runningProcess->arrivalTime;

        // Run the process for one time unit
        printProcessRan(h + 1, runningProcess->name);
        runningProcess->workRemaining--;
        runningProcess->totalTimeUsed++;
        runningProcess->timeInQueue++;

        // If process finishes
        if (runningProcess->workRemaining == 0) {
            printProcessCompleted(runningProcess->name, runningProcess->responseTime, time + 1 - runningProcess->arrivalTime);
            totalResponseTime += runningProcess->responseTime;
            totalTurnaroundTime += (time + 1 - runningProcess->arrivalTime);
            free(runningProcess);
            free(runningNode);
            remainingProcesses--;
        } 
        // If process exceeds queue time allotment, demote
        else if (runningProcess->timeInQueue >= queue[h].timeAllotment && h < numberOfQueues - 1) {
            runningProcess->timeInQueue = 0;
            enqueue(&queue[h + 1], runningNode);
        } 
        // Otherwise, re-enqueue to same queue
        else {
            enqueue(&queue[h], runningNode);
        }

        time++;

        // Reset all queues after time period T
        if (time % timePeriod == 0) {
            for (int i = 1; i < numberOfQueues; i++) {
                while (queue[i].size > 0) {
                    enqueue(&queue[0], dequeue(&queue[i]));
                }
            }
        }
    }

    printAllFinished(time, totalResponseTime / numberOfProcesses, totalTurnaroundTime / numberOfProcesses);
    return 0;
}
