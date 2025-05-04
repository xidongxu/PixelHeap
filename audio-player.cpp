#include "audio.h"
#include "player.h"
#include "fastlz.h"
#include "callstack.h"
#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>
#include "pthread.h"
extern "C" unsigned int posix_errno = 0;

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    for (;;);
}

void* pthread_entry(void* parameter) {
    int result = 0, counter = 0;
    struct timespec sleep = { 0,0 };
    while (true) {
        sleep.tv_nsec = 999999999;
        sleep.tv_sec = 0;
        result = nanosleep(&sleep, 0);
        printf("%s:%d \n", __FUNCTION__, counter++);
        if (result)
            break;
    }
    return(&result);
}

void tx_application_define(void* first_unused_memory) {
    static pthread_t pthread = { NULL };
    static pthread_attr_t ptattr = { NULL };
    static uint32_t memory[256 * 1024 / sizeof(uint32_t)] = { NULL };

    void *pheap = (void*)posix_initialize(memory);
    pthread_attr_init(&ptattr);

    struct sched_param parameter = { NULL };
    memset(&parameter, 0, sizeof(parameter));
    parameter.sched_priority = 10;
    pthread_attr_setstackaddr(&ptattr, pheap); 
    pthread_attr_setschedparam(&ptattr, &parameter);
    pthread_create(&pthread, &ptattr, pthread_entry, NULL);
}

int main(void) {
    tx_kernel_enter();
    callstack();
    main_audio();
    main_player();
    return 0;
}
