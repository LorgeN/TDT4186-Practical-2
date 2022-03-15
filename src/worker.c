#include "worker.h"
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

int __check_active(worker_control_t *ctl)
{
    pthread_mutex_lock(&ctl->active_lock);
    unsigned int res = ctl->active;
    pthread_mutex_unlock(&ctl->active_lock);
    return res;
}

void __deactivate(worker_control_t *ctl)
{
    pthread_mutex_lock(&ctl->active_lock);
    ctl->active = 0;
    pthread_mutex_unlock(&ctl->active_lock);
}

void *__work(void *ctl_void)
{
    worker_control_t *control = (worker_control_t *)ctl_void;

    // Loop forever. Use SIGKILL to terminate. Could alternatively make some constant
    // boolean poll to check if the process should terminate, then pthread_join in the
    // destroy function
    while (1)
    {
        int fd = bb_get(control->fd_buffer);
        // If it is not active, we should not process the request
        if (!__check_active(control))
        {
            break;
        }

        // Hand off to handler
        control->func(fd);
    }

    return EXIT_SUCCESS;
}

worker_control_t *worker_init(unsigned int worker_count, unsigned int buffer_slots, ProcessingFunction func)
{
    BNDBUF *buf = bb_init(buffer_slots);
    if (buf == NULL)
    {
        return NULL;
    }

    worker_control_t *wc = malloc(sizeof(worker_control_t));
    if (wc == NULL)
    {
        bb_del(buf);
        return NULL;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * worker_count);
    if (threads == NULL)
    {
        free(wc);
        bb_del(buf);
        return NULL;
    }

    int lock_res = pthread_mutex_init(&wc->active_lock, NULL);
    if (lock_res == -1)
    {
        free(wc);
        free(threads);
        bb_del(buf);
        return NULL;
    }

    wc->active = 1;
    wc->fd_buffer = buf;
    wc->func = func;

    for (unsigned int i = 0; i < worker_count; i++)
    {
        pthread_create(&threads[i], NULL, __work, (void *)wc);
    }

    wc->worker_threads = threads;
    wc->worker_count = worker_count;

    return wc;
}

void worker_destroy(worker_control_t *control)
{
    __deactivate(control);

    // Push a number of negative values to the ring buffer so that there is a
    // signal to the processing threads, ensuring that they actually terminate
    for (unsigned int i = 0; i < control->worker_count; i++)
    {
        worker_submit(control, -1);
    }

    for (unsigned int i = 0; i < control->worker_count; i++)
    {
        pthread_join(control->worker_threads[i], NULL);
        printf("Terminated worker thread %u\n", i);
    }

    bb_del(control->fd_buffer);
    free(control->worker_threads);
    free(control);
}

void worker_submit(worker_control_t *control, int fd)
{
    bb_add(control->fd_buffer, fd);
}