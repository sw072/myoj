#ifndef _JUDGE_QUEUE_
#define _JUDGE_QUEUE_

#include "../judgerd.h"
#include <pthread.h>

/* PENDING QUEUE MAX LENGTH */
#ifndef QUEUE_MAX
#define QUEUE_MAX 64
#endif

typedef struct _queue
{
    pthread_mutex_t mutex;
    pthread_cond_t pop_ready;
    pthread_cond_t push_ready;
    solution_t buff[QUEUE_MAX];
    int hidx;       /* header index */
    int tidx;       /* tail index */
}queue_t;

#ifndef EMPTY
#define EMPTY(q) ((q)->hidx == (q)->tidx)
#endif

#ifndef FULL
#define FULL(q) (((q)->hidx + 1) % QUEUE_MAX == (q)->tidx)
#endif

int queue_init(queue_t *q);

int queue_enqueue(queue_t *q, solution_t *s);

int queue_front_pop(queue_t *q, solution_t *s);

int queue_fini(queue_t *q);

#endif
