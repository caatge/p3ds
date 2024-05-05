#include "silver-bun.h"
#include "ntldr.h"
#include "MinHook.h"
#include "cstdio"
#include "patches.h"
#include "ntstatus.h"

void(__thiscall* oUpdateAreaBits)(void* this_, void* pl, void* a);

void __fastcall UpdateAreaBits_not_reckless(void* this_, void* edx, void* player, void* areabits) {
	if (!player) {
		printf("null player in UpdateAreaBits\n");
		return;
	}
	return oUpdateAreaBits(this_, player, areabits);
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

void LoadCallback(const char* str, void* module_base) {

	for (auto& kv_pair : patches) {
		if (kv_pair.first == str) {
			printf("patching %s\n", str);
			// patch module
			CModule mod((uintptr_t)module_base);
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
		if (kv_pair.first == str) {
			for (auto& detour : kv_pair.second) {

				if (hooked_functions.contains(kv_pair.first) && hooked_functions[kv_pair.first].contains(detour.fn)) {
					continue;
				}

				char* mbase = (char*)module_base;
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

void UnloadCallback(const char* str, void* module_base) {
	if (hooked_functions.contains(str)) {
		for (auto& fn : hooked_functions[str]) {
			char* base = (char*)module_base;
			MH_RemoveHook(base + fn);
		}
	}
}

void __stdcall MyLdrDllNotification( ULONG NotificationReason, PLDR_DLL_NOTIFICATION_DATA NotificationData, PVOID Context) {
	char dllname[MAX_PATH];
	size_t ms_forced_me_to_create_this_variable;

	if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED) {
		wcstombs_s(&ms_forced_me_to_create_this_variable, dllname, NotificationData->Loaded.BaseDllName->Buffer, MAX_PATH);
		LoadCallback(dllname, NotificationData->Loaded.DllBase);
	}

	if (NotificationReason == LDR_DLL_NOTIFICATION_REASON_UNLOADED) {
		wcstombs_s(&ms_forced_me_to_create_this_variable, dllname, NotificationData->Unloaded.BaseDllName->Buffer, MAX_PATH);
		UnloadCallback(dllname, NotificationData->Unloaded.DllBase);
	}
}

PVOID cookie;

void PatchesInit() {
	MH_Initialize();

	_LdrRegisterDllNotification LdrRegisterDllNotification = (_LdrRegisterDllNotification)GetProcAddress(GetModuleHandle("ntdll.dll"), "LdrRegisterDllNotification");

	if (LdrRegisterDllNotification(0, MyLdrDllNotification, NULL, &cookie) != STATUS_SUCCESS) {
		printf("NTSTATUS != STATUS_SUCCESS!\n");
	}
}