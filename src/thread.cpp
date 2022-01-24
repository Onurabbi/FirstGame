#include <stdio.h>
#include <SDL_thread.h>
#include "thread.h"

SDL_Thread* threads[NUM_THREADS];
SDL_mutex* queue_mutex;
SDL_cond* queue_cond;

thread_job job_queue[256];
uint32_t num_jobs = 0;

void execute_job(thread_job* p_job)
{
    p_job->job_function(p_job->arg);
}

void func1(void* arg)
{
    SDL_LockMutex(queue_mutex);
    printf("Func1 is printing...\n");
    SDL_UnlockMutex(queue_mutex);
}

void func2(void* arg)
{
    const char* str = (const char*)arg;
    printf("Func2 is printing %s\n", str);
}

void submit_job(thread_job job)
{
    SDL_LockMutex(queue_mutex);
    job_queue[num_jobs] = job;
    num_jobs++;
    SDL_UnlockMutex(queue_mutex);
    SDL_CondSignal(queue_cond);
}

int start_thread(void* args)
{
    for(;;)
    {
        thread_job job;
        SDL_LockMutex(queue_mutex);
        
        while(num_jobs == 0)
        {
            SDL_CondWait(queue_cond, queue_mutex);
        }

        job = job_queue[0];

        for(uint32_t i = 0; i < num_jobs - 1; ++i)
        {
            job_queue[i] = job_queue[i+1];
        }
        num_jobs--;
        SDL_UnlockMutex(queue_mutex);
        execute_job(&job);
    }
    return 0;
}

void job_system_init(void)
{
    queue_mutex = SDL_CreateMutex();
    queue_cond  = SDL_CreateCond();

    for(uint32_t i = 0; i < NUM_THREADS; ++i)
    {
        threads[i] = SDL_CreateThread(&start_thread, "Thread", NULL);
        if(threads[i] == NULL)
        {
            perror("Failed to create the thread\n");
        }
    }
}