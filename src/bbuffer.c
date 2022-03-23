#include "bbuffer.h"

#include <stdio.h>
#include <stdlib.h>

struct BNDBUF {
    unsigned int size;
    unsigned int tail;
    unsigned int head;
    int *data;

    /*
    We have to use semaphores and a separate lock. The semaphores simply keep track of
    available resources. A case exists where e.g. two (or more) processes may attempt to
    insert data at the same time. If there are two (or more) available slots these will
    collide. We could use another semaphore for this aswell, but there is really no
    advantage to doing that to just using a normal lock.
    */

    pthread_mutex_t rw_lock;  // Lock for making sure we don't concurrently modify

    SEM *add_lock;  // Keeps track of amount of open slots in the buffer
    SEM *rem_lock;  // Keeps track of amount of closed slots in the buffer
};

BNDBUF *bb_init(unsigned int size) {
    int *data = malloc(sizeof(int) * size);
    if (data == NULL) {
        return NULL;
    }

    SEM *add_lock = sem_init(size);
    if (add_lock == NULL) {
        free(data);
        return NULL;
    }

    SEM *rem_lock = sem_init(0);
    if (rem_lock == NULL) {
        free(data);
        sem_del(add_lock);
        return NULL;
    }

    BNDBUF *buf = malloc(sizeof(BNDBUF));
    if (buf == NULL) {
        free(data);
        sem_del(add_lock);
        sem_del(rem_lock);
        return NULL;
    }

    buf->size = size;
    buf->data = data;
    buf->head = 0;
    buf->tail = 0;

    int lock_res = pthread_mutex_init(&buf->rw_lock, NULL);
    if (lock_res == -1) {
        free(data);
        sem_del(add_lock);
        sem_del(rem_lock);
        return NULL;
    }

    buf->add_lock = add_lock;
    buf->rem_lock = rem_lock;

    return buf;
}

void bb_del(BNDBUF *bb) {
    sem_del(bb->add_lock);
    sem_del(bb->rem_lock);
    pthread_mutex_destroy(&bb->rw_lock);
    free(bb->data);
    free(bb);
}

int bb_get(BNDBUF *bb) {
    P(bb->rem_lock);  // Wait for an available resource

    // Aquire lock for modification. This handles the case where there are
    // multiple elements available, and multiple consumers requesting them.
    pthread_mutex_lock(&bb->rw_lock);

    int element = bb->data[bb->tail++];
    bb->tail %= bb->size;

    pthread_mutex_unlock(&bb->rw_lock);

    V(bb->add_lock);  // Notify any waiting producers that there is a slot available

    return element;
}

void bb_add(BNDBUF *bb, int fd) {
    P(bb->add_lock);

    // Aquire lock for modification. Handles the case where multiple threads
    // are attempting to add at the same time, given that there is space in
    // the buffer
    pthread_mutex_lock(&bb->rw_lock);

    bb->data[bb->head++] = fd;
    bb->head %= bb->size;

    pthread_mutex_unlock(&bb->rw_lock);

    V(bb->rem_lock);  // Notify any waiting consumers that there is data available
}