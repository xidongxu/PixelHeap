#include <iostream>
#include <chrono>
#include <thread>

#include "player.h"

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ALLOW_MONO_STEREO_TRANSITION
#include "minimp3_ex.h"
#include <SDL.h>

using namespace std;

int main_player(void)
{
    // 初始化 SDL2 视频和其他子系统
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 创建一个窗口
    SDL_Window* window = SDL_CreateWindow(
        "audio-player",           // 窗口标题
        SDL_WINDOWPOS_CENTERED,          // 窗口位置：居中
        SDL_WINDOWPOS_CENTERED,          // 窗口位置：居中
        800,                             // 窗口宽度
        600,                             // 窗口高度
        SDL_WINDOW_SHOWN                 // 窗口可见
    );

    // 检查窗口是否成功创建
    if (window == nullptr) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();  // 如果失败，退出 SDL
        return 1;
    }

    // 创建渲染器
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);  // 销毁窗口
        SDL_Quit();
        return 1;
    }

    // 按钮的矩形区域
    SDL_Rect buttonRect = { 300, 250, 200, 100 };  // {x, y, width, height}
    bool quit = false;
    SDL_Event event;

    // 主事件循环
    while (!quit) {
        // 处理事件
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;  // 用户关闭窗口
            }

            // 处理鼠标点击事件
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x, y;
                SDL_GetMouseState(&x, &y);

                // 检查鼠标是否点击在按钮区域内
                if (x >= buttonRect.x && x <= buttonRect.x + buttonRect.w &&
                    y >= buttonRect.y && y <= buttonRect.y + buttonRect.h) {
                    std::cout << "Button clicked!" << std::endl;
                }
            }
        }

        // 清屏（填充背景色）
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // 白色背景
        SDL_RenderClear(renderer);

        // 绘制按钮（填充矩形）
        SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);  // 蓝色按钮
        SDL_RenderFillRect(renderer, &buttonRect);

        // 绘制按钮边框
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // 黑色边框
        SDL_RenderDrawRect(renderer, &buttonRect);

        // 显示渲染内容
        SDL_RenderPresent(renderer);
    }

    // 销毁渲染器和窗口
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    // 退出 SDL
    SDL_Quit();
	return 0;
}
