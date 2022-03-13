#ifndef ____BBUFFER___H___
#define ____BBUFFER___H___

#include <sem.h>
#include <pthread.h>

/*
 * Bounded Buffer implementation to manage int values that supports multiple
 * readers and writers.
 *
 * The bbuffer module uses the sem module API to synchronize concurrent access
 * of readers and writers to the bounded buffer.
 */

typedef struct BNDBUF
{
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

    pthread_mutex_t *rw_lock; // Lock for making sure we don't concurrently modify

    SEM *add_lock; // Keeps track of amount of open slots in the buffer
    SEM *rem_lock; // Keeps track of amount of closed slots in the buffer
} BNDBUF;

/* Creates a new Bounded Buffer.
 *
 * This function creates a new bounded buffer and all the helper data
 * structures required by the buffer, including semaphores for
 * synchronization. If an error occurs during the initialization the
 * implementation shall free all resources already allocated by then.
 *
 * Parameters:
 *
 * size     The number of integers that can be stored in the bounded buffer.
 *
 * Returns:
 *
 * handle for the created bounded buffer, or NULL if an error occured.
 */

BNDBUF *bb_init(unsigned int size);

/* Destroys a Bounded Buffer.
 *
 * All resources associated with the bounded buffer are released.
 *
 * Parameters:
 *
 * bb       Handle of the bounded buffer that shall be freed.
 */

void bb_del(BNDBUF *bb);

/* Retrieve an element from the bounded buffer.
 *
 * This function removes an element from the bounded buffer. If the bounded
 * buffer is empty, the function blocks until an element is added to the
 * buffer.
 *
 * Parameters:
 *
 * bb         Handle of the bounded buffer.
 *
 * Returns:
 *
 * the int element
 */

int bb_get(BNDBUF *bb);

/* Add an element to the bounded buffer.
 *
 * This function adds an element to the bounded buffer. If the bounded
 * buffer is full, the function blocks until an element is removed from
 * the buffer.
 *
 * Parameters:
 *
 * bb     Handle of the bounded buffer.
 * fd     Value that shall be added to the buffer.
 *
 * Returns:
 *
 * the int element
 */

void bb_add(BNDBUF *bb, int fd);

#endif
