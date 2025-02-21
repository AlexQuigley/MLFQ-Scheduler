#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct process Process;
typedef struct queue Queue;
typedef struct node Node;


// Node structure representing a process in the queue
struct node {
    Node *next;  // Pointer to the next node in the queue
    Process *P;  // Pointer to the process stored in this node
};

// Process structure containing scheduling-related attributes
struct process {
    char name;            // Process name (A, B, C, etc.)
    int arrivalTime;      // Time when the process arrives in the system
    int workRemaining;    // Amount of work (CPU time) remaining for the process
    int timeInQueue;      // Time the process has spent in the current queue
    int totalTimeUsed;    // Total CPU time the process has used
    int responseTime;     // Response time (first time it gets CPU - arrival time)
    int hasStarted;       // Flag to indicate if the process has started execution
};

// Queue structure for multilevel feedback queue scheduling
struct queue {
    int timeSlice;        // Maximum time a process can run before being preempted
    int timeAllotment;    // Maximum time a process can spend in this queue before demotion
    int size;             // Number of processes currently in the queue
    Node *head, *tail;    // Pointers to the front and back of the queue
};

// Function to add a node (process) to the end of a queue
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

// Function to remove and return a node (process) from the front of a queue
Node* dequeue(Queue *Q) {
    if (Q->size == 0) return NULL;

    Node *tmp = Q->head;
    Q->head = Q->head->next;
    Q->size--;

    if (Q->size == 0) Q->tail = NULL;

    tmp->next = NULL;
    return tmp;
}

// Function to view the process at the front of the queue without removing it
Process* peek(Queue *Q) {
    if (Q->size == 0) return NULL;
    return Q->head->P;
}

// Function to print when a process runs, indicating its queue level
void printProcessRan(int queueNum, char processName)  {
    printf("%d %c\n", queueNum, processName);
}

// Function to print when a process completes, including response and turnaround times
void printProcessCompleted(char processName, int responseTime, int turnaroundTime)  {
    printf("%c %d %d\n", processName, responseTime, turnaroundTime);
}

// Function to print final statistics after all processes have finished executing
void printAllFinished(int endTime, int avgResponseTime, int avgTurnaroundTime)  {
    printf("%d %d %d\n", endTime, avgResponseTime, avgTurnaroundTime);
}

int main(int argc, char* argv[]) {
    Node *nextToArrive = NULL;
    int remainingProcesses = 0;

    // Ensure proper command-line arguments are provided
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <T> <numberOfQueues> <queue1_timeSlice> <queue1_allotment> ... <process_arrival> <process_work>\n", argv[0]);
        return 1;
    }

    // Read time period for queue priority reset
    int timePeriod;
    sscanf(argv[1], "%d", &timePeriod);

    // Read number of queues
    int numberOfQueues;
    sscanf(argv[2], "%d", &numberOfQueues);

    // Initialize queues with their time slice and allotment values
    Queue queue[numberOfQueues];
    for (int i = 1; i <= numberOfQueues; i++) {
        sscanf(argv[i*2+1], "%d", &(queue[i-1].timeSlice));
        sscanf(argv[i*2+2], "%d", &(queue[i-1].timeAllotment));
        queue[i-1].size = 0;
        queue[i-1].head = NULL;
        queue[i-1].tail = NULL;
    }

    // When two processes are added to the queue, the newer process is not getting placed ahead of the one already in the queue.
    // Add a check flag so that once a process is finished running it checks if there are new processes that have been queued 
    // and moves them to the front of the queue, if multiple processes have been added order them in alphabetical order.

    int numberOfProcesses = (argc - 3 - numberOfQueues*2) / 2;
    Node *arrivalQueue = NULL;
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
    Node* currentRunningNode = NULL;
    int timeSliceUsed = 0;
    int h = 0;

    while (remainingProcesses > 0) {
        while (arrivalQueue && arrivalQueue->P->arrivalTime == time) {
            Node *arrived = arrivalQueue;
            arrivalQueue = arrivalQueue->next;
            enqueue(&queue[0], arrived);
            if (currentRunningNode != NULL && h > 0) {
                enqueue(&queue[h], currentRunningNode);
                currentRunningNode = NULL;
                timeSliceUsed = 0;
            }
        }

        int new_h = 0;
        while (new_h < numberOfQueues && queue[new_h].size == 0) {
            new_h++;
        }
        if (new_h == numberOfQueues && currentRunningNode == NULL) {
            time++;
            continue;
        }

        if (currentRunningNode == NULL) {
            h = new_h;
            currentRunningNode = dequeue(&queue[h]);
            timeSliceUsed = 0;
        } else if (new_h < h) {
            enqueue(&queue[h], currentRunningNode);
            currentRunningNode = dequeue(&queue[new_h]);
            timeSliceUsed = 0;
            h = new_h;
        }

        Process *runningProcess = currentRunningNode->P;

        if (runningProcess->responseTime == -1) {
            runningProcess->responseTime = time - runningProcess->arrivalTime;
        }

        printProcessRan(h + 1, runningProcess->name);
        runningProcess->workRemaining--;
        runningProcess->totalTimeUsed++;
        runningProcess->timeInQueue++;
        timeSliceUsed++;

        if (runningProcess->workRemaining == 0) {
            printProcessCompleted(runningProcess->name, runningProcess->responseTime, time + 1 - runningProcess->arrivalTime);
            totalResponseTime += runningProcess->responseTime;
            totalTurnaroundTime += (time + 1 - runningProcess->arrivalTime);
            free(runningProcess);
            free(currentRunningNode);
            currentRunningNode = NULL;
            timeSliceUsed = 0;
            remainingProcesses--;
        } else if (runningProcess->timeInQueue >= queue[h].timeAllotment && h < numberOfQueues - 1) {
            runningProcess->timeInQueue = 0;
            enqueue(&queue[h + 1], currentRunningNode);
            currentRunningNode = NULL;
            timeSliceUsed = 0;
        } else if (timeSliceUsed >= queue[h].timeSlice) {
            enqueue(&queue[h], currentRunningNode);
            currentRunningNode = NULL;
            timeSliceUsed = 0;
        }

        time++;

        if (time % timePeriod == 0) {
            for (int i = 1; i < numberOfQueues; i++) {
                while (queue[i].size > 0) {
                    enqueue(&queue[0], dequeue(&queue[i]));
                }
            }
            if (currentRunningNode != NULL && h != 0) {
                runningProcess->timeInQueue = 0;
                enqueue(&queue[h], currentRunningNode);
                currentRunningNode = NULL;
                timeSliceUsed = 0;
            }
            h = 0;
        }
    }

    printAllFinished(time, totalResponseTime / numberOfProcesses, totalTurnaroundTime / numberOfProcesses);
}