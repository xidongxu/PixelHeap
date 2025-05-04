#include "audio.h"
#include "player.h"
#include "fastlz.h"
#include "callstack.h"
#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>
#include "pthread.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    for (;;);
}

static void* pthread_entry(void* parameter) {
    int result = 0, counter = 0;
    struct timespec sleep = { 0,0 };
    callstack();
    main_audio();
    main_player();
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
    static uint8_t pheap[256 * 1024] = { NULL };
    struct sched_param parameter = { NULL };
    void* pmemery = posix_initialize(pheap);

    memset(&parameter, 0, sizeof(parameter));
    parameter.sched_priority = 10;
    pthread_attr_init(&ptattr);
    pthread_attr_setstackaddr(&ptattr, pmemery);
    pthread_attr_setschedparam(&ptattr, &parameter);
    pthread_create(&pthread, &ptattr, pthread_entry, NULL);
}

int main(void) {
    tx_kernel_enter();
    return 0;
}
