// Omri Fridental

#include "osqueue.h"
#include <pthread.h>

#ifndef __THREAD_POOL__
#define __THREAD_POOL__

typedef enum  destroy_type {
    DESTROY_IMMIDETLY = 1,
    DESTROY_WHEN_QUEUE_EMPTY = 2
} destroy_t;

/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *
 * @var tasksQueue  queue of tasks functions to run.
 * @var N           number of thread running.
 * @var threads     array of threads.
 * @var destroying  a flag to notify if the threadpool is currently destroying.
 * @var queueLock   a mutex on the queeu.
 * @var conditional a conditional variable to notify working threads.
 * @var tasksPendingCount  a count of how many tasks waits in queue.
 */
typedef struct thread_pool
{
    // a queue of tasks.
    OSQueue* tasksQueue;
    int     taskPendingCount;

    // an array of threads.
    int N;
    pthread_t* threads;

    // locking insertion while destroying.
    int destroying;
    destroy_t destoryType;

    // locks for each thread pool funcs.
    pthread_mutex_t queueLock;
    pthread_cond_t conditional;

    int numThreadsRunning;



} ThreadPool;



// Task: a struct of func + arg. and isDone boolean.

typedef struct task_t
{
    void (* func_ptr) (void*);
    void* arg;

} Task;

ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
