// Omri Fridental

#include "threadPool.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define ERROR_MESSAGE "Error in system call"

void tpThread(ThreadPool*);

/**
 * creates the threadpool and return a pointer to it.
 * allocates memeory.
 * setts the number of threads running to be numOfThreads.
 *
 * @param numOfThreads the number of threads the threadpool had.
 * @return
 */
ThreadPool* tpCreate(int numOfThreads) {

    if (numOfThreads <= 0) return NULL;


    // allocating memory for the ThreadPool and its members;

    ThreadPool* threadPool = (ThreadPool*) malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {

        fprintf(stderr, ERROR_MESSAGE);
        return NULL;
    }

    threadPool->N = numOfThreads;
    threadPool->tasksQueue = osCreateQueue();
    threadPool->threads = (pthread_t*) calloc(sizeof(pthread_t), numOfThreads);
    threadPool->destroying = 0;
    threadPool->taskPendingCount = 0;


    // setting up a lock on the queue, and starting act in different thread to start the thread pool loop.

    if( pthread_mutex_init(&threadPool->queueLock, NULL) != 0 ||  pthread_cond_init(&threadPool->conditional, NULL)
        || threadPool->threads == NULL || threadPool->tasksQueue == NULL)
    {
        fprintf(stderr, ERROR_MESSAGE);
        return NULL;
    };

    // opening numOfThreads new threads of the threadpool action.
    int i;
    for (i = 0; i < numOfThreads; i++) {

        // if a creation of a thread failed, shutting down all threadpool.
        if (pthread_create(&threadPool->threads[i], NULL, (void* (*) (void*))tpThread, (void*) threadPool) != 0) {

            tpDestroy(threadPool, 0);
            return NULL;
        }
    }
    // returning the ThreadPool created.
    return threadPool;
}

/**
 * destroys threadPool. free resources.
 * blocks the option to add more tasks.
 *
 *
 *
 * @param threadPool the threadPool pointer to destroy.
 * @param shouldWaitForTasks a flag of:
 *      if its != 0, waits until *all* tasks, including ones on the queue, to end.
 *      else, if its 0, waits until the current tasks that are running to end. throws away the tasks queue.
 */

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks) {


    // if already destroying from other thread, do not destroy again.
    if(threadPool->destroying)
        return;


    // defining that we currently destroying the threadpool, and setts the type of destroy by shouldWaitForTasks flag.
    threadPool->destroying = 1;
    threadPool->destoryType = shouldWaitForTasks ? DESTROY_WHEN_QUEUE_EMPTY: DESTROY_IMMIDETLY;

    // if the shouldWaitForTasks is not 0, puts flag of wait until all

    pthread_t   *iter = threadPool->threads,
                *end = threadPool->threads + threadPool->N;
    while (iter != end) {
        pthread_join(*iter, NULL);
        iter ++;

    }

    // freeing sources:

    // freeing queue memory.
    while (! osIsQueueEmpty(threadPool->tasksQueue))
        free(osDequeue(threadPool->tasksQueue));

    osDestroyQueue(threadPool->tasksQueue);

    free(threadPool->threads);
    free(threadPool);

}

#define INSERTION_FAILTURE -1
#define INSERTION_SUCCESS 0

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param) {

    // if arguments are null, or the destroying flag is on, return FAILTURE

    if (threadPool == NULL || computeFunc == NULL || threadPool->destroying)
        return  INSERTION_FAILTURE;

    // else, add the function to the threadPool queue, and return SUCCESS

    else {


        // creating a task of the insertion parameter.
        Task* task = (Task*) malloc (sizeof(Task));
        task->func_ptr = computeFunc;
        task->arg = param;


        // thread safe of the queue by using a mutex, and adding the task to the queue. signaling threads that is
        // an available new tasks in queue using conditional.

        pthread_mutex_lock(&threadPool->queueLock);

        osEnqueue(threadPool->tasksQueue, task);
        threadPool-> taskPendingCount += 1;
        pthread_cond_signal(&threadPool->conditional);

        pthread_mutex_unlock(&threadPool->queueLock);

        // return success.
        return INSERTION_SUCCESS;
    }

}

/**
 * the work that each one of the threads in the threadPool is doing.
 * waits until a new task is in the queue.
 * aquire the task (so other threads won't get it)
 * do the task.
 * when the flag of destroying the queue is on, by the type of destroyment and the state of the queue,
 * sutting down the work of the thread.
 *
 * @param threadPool
 */
void tpThread(ThreadPool* threadPool) {

    threadPool->numThreadsRunning ++;
    //  while we allowed to get new tasks, do it.

    while (1) {

        // locking the mutex to reach queue.
        pthread_mutex_lock(&threadPool->queueLock);

        // while the the numbers of tasks waiting in queue is 0, and the threadpool isn't destroying, wait.
        while (threadPool->taskPendingCount == 0 && !threadPool->destroying) {
            pthread_cond_wait(&threadPool->conditional, &threadPool->queueLock);
        }


        // if we do destroying: if its a SHUTTDOWN_IMMIDETLY, or if its a SHUTTDOWN_WHEN_QUEUE_EMPTY,
        //  and queue is indeed empty, stop the work of the thread (exit)

        if  (threadPool->destroying &&
            (threadPool->destoryType == DESTROY_IMMIDETLY ||
            (threadPool->destoryType == DESTROY_WHEN_QUEUE_EMPTY && threadPool->taskPendingCount == 0))
            ) {

            break;
        }
        // pull the first task in the queue.

        Task *taskToPreform = (Task *) osDequeue(threadPool->tasksQueue);
        threadPool->taskPendingCount -= 1;

        // unlocking the mutex.
        pthread_mutex_unlock(&threadPool->queueLock);

        // preform the task, and free memory.
        if (taskToPreform != NULL) {

            taskToPreform->func_ptr(taskToPreform->arg);
            free(taskToPreform);
        }

    }
    threadPool->numThreadsRunning --;
    // unlocking the mutex if its on, and exit the thread.
    pthread_mutex_unlock(&threadPool->queueLock);
    pthread_exit(NULL);

}
