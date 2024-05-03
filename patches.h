#pragma once
#include "map"
#include "vector"
#include "set"
#include "string"
void PatchesInit();
struct jmphook_s {
	jmphook_s(uintptr_t fn_, void* detour_, void** trampoline_) : fn(fn_), detour(detour_), trampoline(trampoline_) {};
	uintptr_t fn; // rva
	void* detour; // patched
	void** trampoline; // ptr to trampoline
};

typedef const std::map<const std::string, const std::vector<jmphook_s>> detour_table_t;
typedef const std::map < const std::string, std::map<const std::string, const std::vector<uint8_t>>> mempatch_table_t;