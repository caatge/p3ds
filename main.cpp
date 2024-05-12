#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include "patches.h"

static constexpr unsigned char s_AllocConsoleShortCircuit[] =
{
    0x33, 0xc0, // XOR EAX, EAX
    0x40,       // INC EAX
    0xc3,       // RET
};

char msg[] = "We do not support vgui SRCDS and you shouldn't want that anyway if you're doing stuff properly.\nPlease launch with -console";

int main(int argc, char* argv[])
{
    // get back to where we would be if we were WinMain:
    HINSTANCE hInstance = GetModuleHandleA(NULL);
    HINSTANCE hPrevInstance = 0;
    LPSTR pCmdLine = GetCommandLineA();
    INT nCmdShow = 0;

    // dedicated.dll calls AllocConsole - we need to short-circuit it as we already have a console.
    DWORD dwOldProtect = 0;
    VirtualProtect((void*)&AllocConsole, sizeof(s_AllocConsoleShortCircuit), PAGE_EXECUTE_READWRITE, &dwOldProtect);
    memmove((void*)&AllocConsole, s_AllocConsoleShortCircuit, sizeof(s_AllocConsoleShortCircuit));
    VirtualProtect((void*)&AllocConsole, sizeof(s_AllocConsoleShortCircuit), dwOldProtect, &dwOldProtect);
    FlushInstructionCache(GetCurrentProcess(), &AllocConsole, sizeof(s_AllocConsoleShortCircuit));

    // and now back to our regularly scheduled srcds.exe
    char* pszOriginalPath = NULL;
    size_t nOriginalPathLength = 0;
    _dupenv_s(&pszOriginalPath, &nOriginalPathLength, "PATH");

    char szModuleFileName[MAX_PATH];
    if (!GetModuleFileNameA(hInstance, szModuleFileName, sizeof(szModuleFileName)))
    {
        MessageBoxA(NULL, "Failed calling GetModuleFileName", "Launcher Error", 0);
        return 0;
    }

    if (!strstr(GetCommandLine(), "-console")) {
        MessageBox(NULL, msg, "Bad Boy.", 0);
        ExitProcess(1);
    }

    char* pLastSlash = strrchr(szModuleFileName, '\\');

    char szWorkingDirectory[MAX_PATH];
    strncpy_s(szWorkingDirectory, szModuleFileName, pLastSlash - szModuleFileName);

    char szPath[0x8000];
    sprintf_s(szPath, "%s\\bin\\;%s", szWorkingDirectory, pszOriginalPath);
    free(pszOriginalPath);
    SetEnvironmentVariableA("PATH", szPath);
    PatchesInit();
    sprintf_s(szPath, "%s\\bin\\dedicated.dll", szWorkingDirectory);

    HMODULE hModule = LoadLibraryExA(szPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hModule)
    {
        char* pszErrorMessage = NULL;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            GetLastError(),
            LANG_NEUTRAL,
            (LPSTR)&pszErrorMessage,
            0,
            NULL
        );
        char szError[0x400];
        sprintf_s(szError, "Failed to load the launcher DLL:\n\n%s", pszErrorMessage);
        MessageBoxA(NULL, szError, "Launcher Error", 0);
        LocalFree(pszErrorMessage);
        return 0;
    }

    typedef int (*MainFunc)(_In_ HINSTANCE, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR pCmdLine, _In_ int nCmdShow);
    MainFunc pDedicatedMain = (MainFunc)GetProcAddress(hModule, "DedicatedMain");
    return (*pDedicatedMain)(hInstance, hPrevInstance, pCmdLine, nCmdShow);
}
