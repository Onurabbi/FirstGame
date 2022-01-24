
#define NUM_THREADS 4

typedef struct 
{
    void(*job_function)(void*);
    void* arg;
}thread_job;

void execute_job(thread_job* p_job);
void func1(void* arg);
void func2(void* arg);
void submit_job(thread_job job);
int  start_thread(void* args);
void job_system_init(void);