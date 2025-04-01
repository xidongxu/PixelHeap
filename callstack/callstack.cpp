#include "callstack.h"
#include "fastlz.h"
#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>

#pragma comment(lib, "DbgHelp.lib")

void callstack() {
    void* stack[64];
    unsigned short frames;
    SYMBOL_INFO* symbol;
    HANDLE process = GetCurrentProcess();

    SymInitialize(process, NULL, TRUE);
    frames = CaptureStackBackTrace(0, 64, stack, NULL);

    symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256);
    symbol->MaxNameLen = 255;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

    char* buffer = (char*)malloc(sizeof(UINT8) * 256);
    for (unsigned short i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        snprintf((buffer + i * 15), (256 - i * 15), "0x%0llX ", symbol->Address);
    }
    int length = strnlen(buffer, 256);
    printf("compress before :[%d][%s]\n", length, buffer);

    void* compress = malloc(sizeof(UINT8) * 256);
    memset(compress, 0, 256);
    int result = fastlz_compress((void *)buffer, strnlen(buffer, 256), compress);
    printf("compress result :[%d]\n", result);

    void* decompress = malloc(sizeof(UINT8) * 256);
    memset(decompress, 0, 256);
    result = fastlz_decompress(compress, result, decompress, 256);
    printf("decompress result :[%d][%s]\n", result, (char*)decompress);

    free(symbol);
    free(buffer);
    free(compress);
    free(decompress);
}
