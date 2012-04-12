#include "judge_queue.h"
#include "../trace/trace.h"
#include <string.h>
#include <assert.h>

int queue_init(queue_t *q)
{
    pthread_mutex_init(&(q->mutex), NULL);
    pthread_cond_init(&(q->pop_ready), NULL);
    pthread_cond_init(&(q->push_ready), NULL);
    memset(q->buff, 0, sizeof(q->buff));
    q->hidx = q->tidx = 0;
    return 0;
}

int queue_enqueue(queue_t *q, solution_t *s)
{
    assert(q);
    pthread_mutex_lock(&(q->mutex));
    while(FULL(q)) pthread_cond_wait(&(q->push_ready), &(q->mutex));
    q->buff[q->tidx] = *s;
    q->tidx = (q->tidx + 1) % QUEUE_MAX;
    pthread_mutex_unlock(&(q->mutex));
    pthread_cond_signal(&(q->pop_ready));
    return 0;
}

int queue_front_pop(queue_t *q, solution_t *s)
{
    assert(q);
    pthread_mutex_lock(&(q->mutex));
    while(EMPTY(q)) pthread_cond_wait(&(q->pop_ready), &(q->mutex));
    *s = q->buff[q->hidx];
    q->hidx = (q->hidx + 1) % QUEUE_MAX;
    pthread_mutex_unlock(&(q->mutex));
    pthread_cond_signal(&(q->push_ready));
    return 0;
}

int queue_fini(queue_t *q)
{
    pthread_mutex_destroy(&(q->mutex));
    pthread_cond_destroy(&(q->push_ready));
    pthread_cond_destroy(&(q->pop_ready));
    memset(q->buff, 0, sizeof(q->buff));
    q->hidx = q->tidx = 0;
    return 0;
}
