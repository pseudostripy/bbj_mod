/*
 *    build cmd with mingw64:
 * 		gcc -m32 -Wall -Wl,--enable-stdcall-fixup exports.DEF nobbj.c -shared -o DINPUT8.dll -l Psapi
 */

 /*
		 This DLL implements the bbj mod originally named in CE:
			"Baby Jump fix (only one can be active at a time)"
*/

#include <windows.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <psapi.h>
#include <processthreadsapi.h>
//#define NOLOGO

 /*
  *    This patching mechanism is taken from:
  *        https://github.com/bladecoding/DarkSouls3RemoveIntroScreens/blob/master/SoulsSkipIntroScreen/dllmain.cpp
  */
struct patch {
	DWORD rel_addr;
	DWORD size;
	char patch[50];
	char orig[50];
};

typedef HRESULT(WINAPI* dinp8crt_t)(HINSTANCE, DWORD, REFIID,
	LPVOID*, LPUNKNOWN);
dinp8crt_t oDirectInput8Create;

__attribute__((dllexport))
HRESULT WINAPI DirectInput8Create(HINSTANCE inst, DWORD ver, REFIID id,
	LPVOID* pout, LPUNKNOWN outer)
{
	return oDirectInput8Create(inst, ver, id, pout, outer);
}

void setup_d8proxy(void)
{
	char syspath[320];
	GetSystemDirectoryA(syspath, 320);
	strcat(syspath, "\\dinput8.dll");
	HMODULE mod = LoadLibraryA(syspath);
	oDirectInput8Create = (dinp8crt_t)GetProcAddress(mod, "DirectInput8Create");
}

void attach_hook(void)
{ 	
	void* module_addr = GetModuleHandle(NULL);
	MODULEINFO moduleInfo;
	GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &moduleInfo, sizeof(moduleInfo));

	// Declare address variables:
	DWORD basea_offset;
	DWORD jumpfcn_offset;
	//DWORD nologo_offset;
	byte speed_final_offset; // Only LOWER-BYTE of final offset actually!

	// Switch version / Populate addresses:
	if (moduleInfo.SizeOfImage == 31145984) {
		// Vanilla 1.12 (Online Patch):
		basea_offset = 0x01150414;
		jumpfcn_offset = 0x3A7364;
		speed_final_offset = 0xE8;
	}
	else if (moduleInfo.SizeOfImage == 33902592) {
		// Vanilla 1.11 (Vuln Patch):
		basea_offset = 0x11493F4;
		jumpfcn_offset = 0x3A09C4;
		speed_final_offset = 0xE8;
	}
	else if (moduleInfo.SizeOfImage == 20176896) {
		// Vanilla 1.02 (Old Patch):
		basea_offset = 0x0FB3E3C;
		jumpfcn_offset = 0x033A424;
		speed_final_offset = 0xD8;
	}
	else {
		return; // unimplemented version
	}

	//////////////////////////////////////////
	//				NOLOGO Mod:
//#ifdef NOLOGO
//	* (byte*)(module_addr + nologo_offset) = 0x01;
//#endif
	//////////////////////////////////////////


	// Setup the (version-specific) basepointer:
	void* baseA = module_addr + basea_offset;


	struct patch jump_patch =
	{
		jumpfcn_offset, 									// RelAddr (from ModuleBase) - "NoBabyJAOB1"  
		8,													// Size (bytes)
		{													// newcode
			0xE9, 0x00, 0x00, 0x00, 0x00,					// jmp [xxxx] - Note: Local Relative Jump (+5)
			0x90, 0x90, 0x90								// nop, nop, nop
		}, 	
		{													// origcode
			0x8b, 0xC8,										// mov ecx, eax
			0x8B, 0x42, 0x10,								// mov eax, [edx + 10]
			0x0F, 0x51, 0xC0								// sqrt xmm0, xmm0
		}, 	
	};


	// "NoBabyJ (full jump length)" code
	// Use true speed at offsets: baseA, 0x74, 0x1C4, 0xEE8/0xED8 (structure changes for V1.02)
	static char bbjcode_bytes[] =
	{
		// replicate overwritten bytes:
		0x8B, 0xC8,										// [0] mov ecx, eax
		0x8B, 0x42, 0x10, 								// [2] mov eax,[edx+10]
		0x0F, 0x51, 0xC0, 								// [5] sqrtps xmm0,xmm0

		// put robust speed into xmm0 instead:
		0x50, 											// [8] push eax
		0xA1, 0x00, 0x00, 0x00, 0x00, 					// [9] mov eax,[baseA]
		0x8B, 0x80, 0x74, 0x00, 0x00, 0x00,				// [14] mov eax, DWORD PTR [eax + 0x74]
		0x8B, 0x80, 0xC4, 0x01, 0x00, 0x00,				// [20] mov eax, DWORD PTR [eax + 0x1C4]
		0xF3, 0x0F, 0x10, 0x80,    0x00   , 0x0E, 0x00, 0x00, // [26] movss xmm0, DWORD PTR [eax + 0xEx8] (move and interpret as float)
		0x58, 											// [34] pop eax

		// return to end of inject:
		0xE9, 0x00, 0x00, 0x00, 0x00, 					// [35] jmp [xxxx]
	};

	// Get absolute addresses:
	void* jumpinjaddr = module_addr + jump_patch.rel_addr;
	void* jumpinj_ret = jumpinjaddr + jump_patch.size;
	
	// Update patch addresses:
	void* addr_bbjcode = bbjcode_bytes;
	void* bbjcode_fwd_offset = (void*)(addr_bbjcode - (jumpinjaddr + 5));
	memcpy(&jump_patch.patch[1], &bbjcode_fwd_offset, sizeof(bbjcode_fwd_offset));	// jmp fwd offset


	// Fill in addresses/offsets in bbjcode:
	memcpy(&bbjcode_bytes[30], &speed_final_offset, sizeof(speed_final_offset));
	memcpy(&bbjcode_bytes[10], &baseA, sizeof(baseA)); 					// mov rax,[baseA]

	// Fix inject return address
	void* jmpinj_ret_offset = (void*)(-(addr_bbjcode + 39 + 1 - jumpinj_ret)); // jmp backwards offset
	memcpy(&bbjcode_bytes[36], &jmpinj_ret_offset, sizeof(jmpinj_ret_offset)); 	// return to jumpinject (+offset)


	// Make bbjcode_bytes executable:
	DWORD op;
	VirtualProtect(bbjcode_bytes, sizeof(bbjcode_bytes),
		PAGE_EXECUTE_READWRITE, &op);

	// Edit memory:
	DWORD size = jump_patch.size;
	if (memcmp(jumpinjaddr, jump_patch.orig, size) == 0) {
		DWORD old;
		VirtualProtect(jumpinjaddr, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(jumpinjaddr, jump_patch.patch, size);
		VirtualProtect(jumpinjaddr, size, old, &old);
	}
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason,
		      LPVOID res)
{
    switch (reason) {
    case DLL_PROCESS_ATTACH:
	setup_d8proxy();
	attach_hook();
	break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
	break;
    }
    return TRUE;
}
