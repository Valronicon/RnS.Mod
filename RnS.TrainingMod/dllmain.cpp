#include "pch.h"
#include <windows.h>
#include "MinHook.h"

// Function die ich hooken möchte
using RunScript = int(__fastcall*)(void*);

// Pointer auf die Originalfunktion
static RunScript fpRunScript = nullptr;

// Hook-Funktion
int __fastcall Detour_RunScript(void* thisPtr) {
    int origResult = fpRunScript(thisPtr); // TODO: Hier dann anderes Script aufrufen? 
    return origResult;
}

BOOL InitializeHook() {
    if (MH_Initialize() != MH_OK)
        return FALSE;

    uintptr_t targetAddr = 0x01234567; // TODO: Echte Adresse der target function herausfinden

    if (MH_CreateHook((LPVOID)targetAddr, &Detour_RunScript,
        reinterpret_cast<LPVOID*>(&fpRunScript)) != MH_OK)
        return FALSE;

    if (MH_EnableHook((LPVOID)targetAddr) != MH_OK)
        return FALSE;

    return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        InitializeHook();
        break;
    case DLL_PROCESS_DETACH:
        // Hooks entfernen und MinHook aufräumen
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
        break;
    }
    return TRUE;
}

