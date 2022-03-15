#ifndef __WORKER_H__
#define __WORKER_H__

#include "bbuffer.h"
#include <pthread.h>

// Define a pretty type for
typedef void (*ProcessingFunction)(int);

typedef struct worker_control_t
{
    BNDBUF *fd_buffer;
    unsigned int worker_count;
    pthread_t *worker_threads;
    ProcessingFunction func;

    // Graceful shutdown
    volatile unsigned int active;
    pthread_mutex_t active_lock;
} worker_control_t;

worker_control_t *worker_init(unsigned int count, unsigned int buffer_slots, ProcessingFunction func);

void worker_destroy(worker_control_t *control);

void worker_submit(worker_control_t *control, int fd);

void get_thread_name(char *str);

#endif