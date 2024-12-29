#include <iostream>
#include <chrono>
#include <thread>

#include "player.h"
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

using namespace std;

int main_player(void)
{
    // 初始化 SDL2 视频和其他子系统
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 初始化 SDL_image
    if (IMG_Init(IMG_INIT_PNG) == 0) { // 可根据需要使用其他格式（如 JPG）
        std::cerr << "IMG_Init Error: " << IMG_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // 初始化 SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    // 创建一个窗口
    SDL_Window* window = SDL_CreateWindow(
        "audio-player",           // 窗口标题
        SDL_WINDOWPOS_CENTERED,          // 窗口位置：居中
        SDL_WINDOWPOS_CENTERED,          // 窗口位置：居中
        600,                             // 窗口宽度
        300,                             // 窗口高度
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

    // 加载图片（例如 "button_background.png"）
    SDL_Surface* imageSurface = IMG_Load("assets/play.png");  // 替换为你的图片文件路径
    if (imageSurface == nullptr) {
        std::cerr << "IMG_Load Error: " << IMG_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);  // 销毁渲染器
        SDL_DestroyWindow(window);      // 销毁窗口
        SDL_Quit();
        return 1;
    }

    // 创建纹理
    SDL_Texture* buttonTexture = SDL_CreateTextureFromSurface(renderer, imageSurface);
    SDL_FreeSurface(imageSurface);  // 释放表面，因为我们已经创建了纹理
    if (buttonTexture == nullptr) {
        std::cerr << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 按钮的矩形区域
    SDL_Rect buttonRect = { 250, 200, 64, 64 };  // {x, y, width, height}
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

        // 绘制按钮的背景（图片）
        SDL_RenderCopy(renderer, buttonTexture, NULL, &buttonRect);  // 将图片绘制到按钮区域

        // 显示渲染内容
        SDL_RenderPresent(renderer);
    }

    // 清理资源
    SDL_DestroyTexture(buttonTexture);  // 销毁纹理
    SDL_DestroyRenderer(renderer);      // 销毁渲染器
    SDL_DestroyWindow(window);          // 销毁窗口

    // 退出 SDL 和 SDL_image
    SDL_Quit();
    IMG_Quit();

    return 0;
}
