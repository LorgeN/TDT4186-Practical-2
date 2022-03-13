#include <bbuffer.h>
#include <stdlib.h>

BNDBUF *bb_init(unsigned int size)
{
    int *data = malloc(sizeof(int) * size);
    SEM *add_lock = sem_init(size);
    SEM *rem_lock = sem_init(0);

    BNDBUF *buf = malloc(sizeof(BNDBUF));

    buf->size = size;
    buf->data = data;
    buf->head = 0;
    buf->tail = 0;

    int lock_res = pthread_mutex_init(buf->rw_lock, NULL);
    if (lock_res == -1)
    {
        exit(EXIT_FAILURE); // Failed to allocate lock
    }
    
    buf->add_lock = add_lock;
    buf->rem_lock = rem_lock;

    return buf;
}

void bb_del(BNDBUF *bb)
{
    sem_del(bb->add_lock);
    sem_del(bb->rem_lock);
    pthread_mutex_destroy(bb->rw_lock);
    free(bb->data);
    free(bb);
}

int bb_get(BNDBUF *bb)
{
    P(bb->rem_lock); // Wait for an available resource

    // Aquire lock for modification. This handles the case where there are
    // multiple elements available, and multiple consumers requesting them.
    pthread_mutex_lock(bb->rw_lock);

    int element = bb->data[bb->tail];
    bb->tail++;
    bb->tail %= bb->size;

    pthread_mutex_unlock(bb->rw_lock);

    V(bb->add_lock); // Notify any waiting producers that there is a slot available
}

void bb_add(BNDBUF *bb, int fd)
{
    P(bb->add_lock);

    // Aquire lock for modification. Handles the case where multiple threads
    // are attempting to add at the same time, given that there is space in
    // the buffer
    pthread_mutex_lock(bb->rw_lock);

    bb->data[bb->head] = fd;
    bb->head %= bb->size;

    pthread_mutex_unlock(bb->rw_lock);

    V(bb->rem_lock); // Notify any waiting consumers that there is data available
}