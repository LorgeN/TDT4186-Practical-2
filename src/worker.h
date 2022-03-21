#ifndef __WORKER_H__
#define __WORKER_H__

#include <pthread.h>

#include "bbuffer.h"

/*
 * This header defines a set of functions allowing creation of an arbitrary number of
 * worker threads. The "work" allowed by these definitions should be contained within
 * the provided ProcessingFunction, and should only take a single integer as input. 
 * 
 * For our specific use case this integer will always be a file descriptor.
 */

/**
 * Pretty type definition for a function that processes a file descriptor. What this 
 * "processing" entails is not relevant for the functionality of the methods defined 
 * in this header.
 */
typedef void (*ProcessingFunction)(int);

/**
 * Struct for keeping track of various values and such to control all worker threads
 * created by a single call to #worker_init.
 */
typedef struct worker_control_t {
    // Buffer for supplying the worker threads with new file descriptors in a thread
    // safe way
    BNDBUF *fd_buffer;
    // The amount of workers contained within this pool
    unsigned int worker_count;
    // An array containing each worker thread as a pthread_t
    pthread_t *worker_threads;
    // The function to execute on the new data (the "work")
    ProcessingFunction func;

    // Graceful shutdown control values. Allows us to signal
    // to the worker threads that they should not process new
    // data passed to them
    volatile unsigned int active;
    pthread_mutex_t active_lock;
} worker_control_t;

/**
 * Initializes a new worker thread pool
 * 
 * @param count        The amount of worker threads
 * @param buffer_slots The amount of slots that should be processing in the ring buffer 
 *                     used to supply the threads with work
 * @param func         The processing function
 */
worker_control_t *worker_init(unsigned int count, unsigned int buffer_slots, ProcessingFunction func);

/**
 * Destroys the given worker pool, waiting for each thread to exit before returning
 * 
 * @param control The worker pool
 */
void worker_destroy(worker_control_t *control);

/**
 * Submits new work to the worker pool in the form of a file descriptor
 * 
 * @param control The worker pool
 * @param fd      The file descriptor
 */
void worker_submit(worker_control_t *control, int fd);

#endif