#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct process Process;
typedef struct queue Queue;
typedef struct node Node;

struct node {
    Node *next;
    Process *P;
};

struct process {
    char name;
    int arrivalTime;
    int workRemaining;
    int timeInQueue;
    int totalTimeUsed;
    int responseTime;
    int hasStarted;
};

struct queue {
    int timeSlice;
    int timeAllotment;
    int size;
    Node *head;
    Node *tail;
};

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

Process* peek(Queue *Q) {
    if (Q->size == 0) return NULL;
    return Q->head->P;
}

void printProcessRan(int queueNum, char processName)  {
    printf("%d %c\n", queueNum, processName);
}

void printProcessCompleted(char processName, int responseTime, int turnaroundTime)  {
    printf("%c %d %d\n", processName, responseTime, turnaroundTime);
}

void printAllFinished(int endTime, int avgResponseTime, int avgTurnaroundTime)  {
    printf("%d %d %d\n", endTime, avgResponseTime, avgTurnaroundTime);
}

int main(int argc, char* argv[]) {
    Node *nextToArrive = NULL;
    int remainingProcesses = 0;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <T> <numberOfQueues> <queue1_timeSlice> <queue1_allotment> ... <process_arrival> <process_work>\n", argv[0]);
        return 1;
    }

    int timePeriod;
    sscanf(argv[1], "%d", &timePeriod);

    int numberOfQueues;
    sscanf(argv[2], "%d", &numberOfQueues);

    Queue queue[numberOfQueues];
    for (int i = 1; i <= numberOfQueues; i++) {
        sscanf(argv[i*2+1], "%d", &(queue[i-1].timeSlice));
        sscanf(argv[i*2+2], "%d", &(queue[i-1].timeAllotment));
        queue[i-1].size = 0;
        queue[i-1].head = NULL;
        queue[i-1].tail = NULL;
    }

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