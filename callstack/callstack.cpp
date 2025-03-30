#include "callstack.h"

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

    printf("Call stack:\n");
    for (unsigned short i = 0; i < frames; i++) {
        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        printf("%i: %s - 0x%0llX\n", i, symbol->Name, symbol->Address);
    }

    free(symbol);
}
