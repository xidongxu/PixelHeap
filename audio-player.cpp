#include "audio.h"
#include "player.h"
#include "fastlz.h"
#include "callstack.h"
#include <FreeRTOS.h>
#include <task.h>

#include <stdio.h>
#include "tx_api.h"

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    for (;;);
}

#define DEMO_STACK_SIZE         1024
#define DEMO_BYTE_POOL_SIZE     9120
static TX_THREAD               thread_0;
static TX_THREAD               thread_1;
static TX_BYTE_POOL            byte_pool_0;
static TX_BYTE_POOL            byte_pool_1;
static UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];

static void thread_entry(ULONG thread_input) {
    while (1) {
        printf("=================== hello ================\n");
        tx_thread_sleep(100);
    }
}

void tx_application_define(void* first_unused_memory) {

    static CHAR* pointer = NULL;
    tx_byte_pool_create(&byte_pool_0, "byte pool", memory_area, DEMO_BYTE_POOL_SIZE);
    tx_byte_allocate(&byte_pool_0, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&thread_0, "thread", thread_entry, 0, pointer, DEMO_STACK_SIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);

    tx_byte_allocate(&byte_pool_1, (VOID**)&pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
    tx_thread_create(&thread_1, "thread", thread_entry, 0, pointer, DEMO_STACK_SIZE, 1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);
}

int main(void) {
    tx_kernel_enter();
    callstack();
    main_audio();
    main_player();
    return 0;
}
