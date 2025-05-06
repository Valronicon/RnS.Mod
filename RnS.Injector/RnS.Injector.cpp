#include <windows.h>
#include <tlhelp32.h>
#include <iostream>

DWORD GetProcId(const wchar_t* procName) {
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (wcscmp(entry.szExeFile, procName) == 0) {
                DWORD pid = entry.th32ProcessID;
                CloseHandle(snapshot);
                return pid;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return 0;
}

int wmain(int argc, wchar_t* argv[])
{
#if _DEBUG
	// Debug-Modus: Pfad zur DLL festgelegt
	const wchar_t* dllPath = L"C:\\Users\\richa\\source\\repos\\RnS.Mod\\x64\\Debug\\RnS.TrainingMod.dll";
#else
    if (argc != 2) {
        std::wcerr << L"Usage: injector.exe <path_to_mod_dll>" << std::endl;
        return 1;
    }
    const wchar_t* dllPath = argv[1];
#endif

    const wchar_t* targetExe = L"RabbitSteel.exe";
    DWORD pid = GetProcId(targetExe);
    if (pid == 0) {
        std::wcerr << L"Fehler: Prozess nicht gefunden." << std::endl;
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
        PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
        FALSE, pid);
    if (!hProcess) {
        std::cerr << "Fehler: OpenProcess fehlgeschlagen." << std::endl;
        return 1;
    }

    size_t pathLen = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, pathLen, MEM_COMMIT, PAGE_READWRITE);
    if (!remoteMem) {
        std::cerr << "Fehler: VirtualAllocEx fehlgeschlagen." << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    if (!WriteProcessMemory(hProcess, remoteMem, dllPath, pathLen, nullptr)) {
        std::cerr << "Fehler: WriteProcessMemory fehlgeschlagen." << std::endl;
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
    FARPROC pLoadLibraryW = GetProcAddress(hKernel, "LoadLibraryW");
    if (!pLoadLibraryW) {
        std::cerr << "Fehler: GetProcAddress LoadLibraryW fehlgeschlagen." << std::endl;
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0,
        (LPTHREAD_START_ROUTINE)pLoadLibraryW,
        remoteMem, 0, nullptr);
    if (!hThread) {
        std::cerr << "Fehler: CreateRemoteThread fehlgeschlagen." << std::endl;
        VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    std::cout << "DLL erfolgreich injiziert!" << std::endl;
    return 0;
}