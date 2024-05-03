#include "windows.h"
#include "MinHook.h"
#include "cstdio"
#include "map"
#include "string"
#include "vector"
#include "silver-bun.h"
#include "set"
#include "patches.h"

void(__thiscall* oUpdateAreaBits)(void* this_, void* pl, void* a);

void __fastcall UpdateAreaBits_not_reckless(void* this_, void* edx, void* player, void* areabits) {
	if (!player) {
		printf("null player in UpdateAreaBits\n");
		return;
	}
	return oUpdateAreaBits(this_, player, areabits);
}

// HOOK SHIT

void LoadCallback(const char* str);

// WINAPI BS
HMODULE(WINAPI* oLoadLibraryA)(LPCSTR);
HMODULE WINAPI hkLoadLibraryA(LPCSTR mod) {
	HMODULE m = oLoadLibraryA(mod);
	if (m) {
		LoadCallback(mod);
	}
	return m;
}

HMODULE(WINAPI* oLoadLibraryExA)(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags);
HMODULE WINAPI hkLoadLibraryExA(LPCSTR lpLibFileName, HANDLE hFile, DWORD dwFlags) {
	HMODULE m = oLoadLibraryExA(lpLibFileName, hFile, dwFlags);
	if (m) {
		LoadCallback(lpLibFileName);
	}
	return m;
}

std::map<std::string, std::set<uintptr_t>> hooked_functions;

mempatch_table_t patches = {
	{
		"engine.dll",
			{
				{ "51 8B 44 24 ? 8B 4C 24 ? 50", {0x31,0xC0,0xC3} }, // xor eax, eax ret
				{ "E8 ? ? ? ? 8B 4C 24 ? 51 57 8B CE E8 ? ? ? ? 5F 5E C2 ? ? CC", {0x90,0x90,0x90,0x90,0x90} }, // nop call
				{ "81 EC ? ? ? ? 89 4C 24 ? 8B 0D", {0xC2,0x04,0x00} }, // ret 4
				{ "8B 06 68 ? ? ? ? EB", {0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90}}, // lan only mode my ass
				{ "81 EC ? ? ? ? 53 55 8B D9 8B 03", {0xC2, 0x08, 0x00} }, // save restore? short answer
				{ "56 8B F1 8B 0D ? ? ? ? 8B 01 8B 90 ? ? ? ? FF D2 8D 4E", {0xC3} }, // save restore? long answer
			},
	},
	{
		"materialsystem.dll",
			{
				{ "81 EC ? ? ? ? 53 8B 9C 24 ? ? ? ? 56 57 BF", {0xC2,0x04,0x00} }, // ret 4
				{ "83 EC ? 53 55 56 57 8B F9", {0xC3} } // ret
			}
	},
	{
		"server.dll",
			{
				{"80 78 ? ? 75 ? 68 ? ? ? ? E8", {0x90,0x90,0x90,0x90,0x90,0x90} }, // gamerules
			}
	}
};

detour_table_t detours = {
	{
		"server.dll",
		{
			{0x1D9DB0 , &UpdateAreaBits_not_reckless, (void**)&oUpdateAreaBits}
		}
	}
};

//TODO: perhaps pass the HMODULE to this than using GetModuleHandle to get it
//TODO: unloading
void LoadCallback(const char* str) {

	for (auto& kv_pair : patches) {
		if (strstr(str, kv_pair.first.c_str())) {
			printf("patching %s\n", str);
			// patch module
			CModule mod(kv_pair.first.c_str());
			for (auto& patch_pair : kv_pair.second) {
				CMemory mem = mod.FindPatternSIMD(patch_pair.first.c_str());
				if (!mem) {
					printf("Failed mempatching %s! Patched already?\n", kv_pair.first.c_str());
					continue;
				}
				mem.Patch(patch_pair.second);
			}
		}
	}

	for (auto& kv_pair : detours) {
		if (strstr(str, kv_pair.first.c_str())) {
			for (auto& detour : kv_pair.second) {

				if (hooked_functions.contains(kv_pair.first) && hooked_functions[kv_pair.first].contains(detour.fn)) {
					continue;
				}

				char* mbase = (char*)GetModuleHandle(kv_pair.first.c_str());
				if (MH_CreateHook(mbase + detour.fn, detour.detour, detour.trampoline) == MH_OK && MH_EnableHook(mbase + detour.fn) == MH_OK) {
					printf("detoured %p successfully!\n", mbase + detour.fn);
					if (!hooked_functions.contains(kv_pair.first)) {
						hooked_functions[kv_pair.first] = {};
					}
					hooked_functions[kv_pair.first].insert(detour.fn);
				}
				else {
					printf("couldn't detour %p!\n", mbase + detour.fn);
				}
			}
		}
	}
}

void PatchesInit() {
	MH_Initialize();
	MH_CreateHook(&LoadLibraryA, &hkLoadLibraryA, (void**)&oLoadLibraryA);
	MH_CreateHook(&LoadLibraryExA, &hkLoadLibraryExA, (void**)&oLoadLibraryExA);
	MH_EnableHook(MH_ALL_HOOKS);
}