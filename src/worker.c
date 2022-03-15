#include "worker.h"
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

void *__work(void *ctl_void)
{
    worker_control_t *control = (worker_control_t *)ctl_void;

    // Loop forever. Use SIGKILL to terminate. Could alternatively make some constant
    // boolean poll to check if the process should terminate, then pthread_join in the
    // destroy function
    while (1)
    {
        int fd = bb_get(control->fd_buffer);
        // Hand off to handler
        control->func(fd);
    }
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

    wc->fd_buffer = buf;
    wc->worker_threads = threads;
    wc->worker_count = worker_count;
    wc->func = func;

    for (int i = 0; i < worker_count; i++)
    {
        pthread_create(threads + i, NULL, __work, (void *)wc);

        char thread_name[16];
        sprintf(thread_name, "Worker %d", i);
        pthread_setname_np(threads[i], thread_name);

        printf("Created thread %s\n", thread_name);
    }

    return wc;
}

void worker_destroy(worker_control_t *control)
{
    char name[16];
    for (unsigned int i = 0; i < control->worker_count; i++)
    {
        printf("Killing thread %s\n", name);
        pthread_getname_np(control->worker_threads[i], name, 16);
        pthread_kill(control->worker_threads[i], SIGKILL);
        printf("Killed thread %s\n", name);
    }

    bb_del(control->fd_buffer);
    free(control->worker_threads);
    free(control);
}

void worker_submit(worker_control_t *control, int fd)
{
    bb_add(control->fd_buffer, fd);
}

void get_thread_name(char *str)
{
    pthread_getname_np(pthread_self(), str, 16);
}