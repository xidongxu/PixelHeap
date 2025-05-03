#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

#include "lvgl.h"
#include "examples/lv_examples.h"
#include "demos/lv_demos.h"
#include "player.h"

using namespace std;

static lv_display_t* hal_init(int32_t w, int32_t h) {
    lv_group_set_default(lv_group_create());

    lv_display_t* disp = lv_sdl_window_create(w, h);

    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_set_group(mouse, lv_group_get_default());
    lv_indev_set_display(mouse, disp);
    lv_display_set_default(disp);

    lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
    lv_indev_set_display(mousewheel, disp);
    lv_indev_set_group(mousewheel, lv_group_get_default());

    lv_indev_t* kb = lv_sdl_keyboard_create();
    lv_indev_set_display(kb, disp);
    lv_indev_set_group(kb, lv_group_get_default());

    return disp;
}

static void playerTask(void* parameters) {
    lv_init();
    hal_init(600, 400);
    lv_demo_benchmark();
    while (1) {
        uint32_t time_till_next = lv_timer_handler();
        lv_delay_ms(time_till_next);
    }
}

static void playerTaskInit(void) {
    static StaticTask_t playerTaskTCB;
    static StackType_t playerTaskStack[256];
    printf("FreeRTOS Plyaer Project\n");
    xTaskCreateStatic(playerTask, "player", 256, NULL, configMAX_PRIORITIES - 1U, &(playerTaskStack[0]), &(playerTaskTCB));
    vTaskStartScheduler();
}

int main_player(void) {
    playerTaskInit();
    return 0;
}
