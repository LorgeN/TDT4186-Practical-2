#include <sem.h>
#include <stdlib.h>

SEM *sem_init(int initVal)
{
    SEM *sem = malloc(sizeof(SEM));

    // Nonrecursive lock
    int lock_res = pthread_mutex_init(sem->lock, NULL);
    if (lock_res == -1)
    {
        exit(EXIT_FAILURE); // Failed to allocate lock
    }

    // Condition variable for waiting for increment
    int cond_res = pthread_cond_init(sem->cond, NULL);
    if (cond_res == -1)
    {
        exit(EXIT_FAILURE);
    }

    return sem;
}

int sem_del(SEM *sem)
{
    pthread_mutex_destroy(sem->lock);
    pthread_cond_destroy(sem->cond);
    free(sem);
}

void P(SEM *sem)
{
    pthread_mutex_lock(sem->lock);

    /*
    From documentation of pthread_cond_signal;
    The pthread_cond_signal() function shall unblock at least one of the threads
    that are blocked on the specified condition variable cond (if any threads
    are blocked on cond).

    Use a loop since there is no guarantee that the signal call will only unlock
    1 thread. Additionally, there may occur other errors that cause this code to
    continue without actually having available resource so we should check every
    time.
    */
    while (1)
    {
        unsigned int val = sem->value;

        if (val)
        {
            break; // val is not 0, no resources available
        }

        pthread_cond_wait(sem->cond, sem->lock);
    }

    sem->value--;
    pthread_mutex_unlock(sem->lock);
}

void V(SEM *sem)
{
    pthread_mutex_lock(sem->lock);
    sem->value++;
    pthread_mutex_unlock(sem->lock);
    pthread_cond_signal(sem->cond);
}