#include "audio.h"
#include "player.h"
#include "fastlz.h"
#include "callstack.h"
#include <FreeRTOS.h>
#include <task.h>

void vApplicationStackOverflowHook(TaskHandle_t xTask, char* pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    for (;;);
}

int main(void) {
    callstack();
    main_audio();
    main_player();
    return 0;
}
