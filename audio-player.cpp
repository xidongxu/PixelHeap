#include "audio.h"
#include "player.h"
#include "fastlz.h"
#include "callstack.h"

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    for (;;);
}

static void playerTask(void* parameters) {
    callstack();
    main_audio();
    main_player();
    while(1) {
        printf("player task tick:%lld\n", xTaskGetTickCount());
        vTaskDelay(100);
    }
}

static void playerTaskInit(void) {
    static StaticTask_t playerTaskTCB;
    static StackType_t playerTaskStack[256];
    printf("FreeRTOS Plyaer Project\n");
    xTaskCreateStatic(playerTask, "player", 256, NULL, configMAX_PRIORITIES - 1U, &(playerTaskStack[0]), &(playerTaskTCB));
    vTaskStartScheduler();
}

int main(void) {
    playerTaskInit();
    return 0;
}
