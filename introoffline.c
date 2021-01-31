/*
 *    build cmd with mingw32:
 *        gcc -Wall -Wl,--out-implib,libmessage.a -Wl,--enable-stdcall-fixup
 *            exports.DEF introoffline.c -shared -o DINPUT8.dll
 *
 */

#include <windows.h>
#include <string.h>

/*
 *    This patching mechanism taken from:
 *        https://github.com/bladecoding/DarkSouls3RemoveIntroScreens/blob/master/SoulsSkipIntroScreen/dllmain.cpp
 *
 *
 */
struct patch {
    DWORD rel_addr;
    DWORD size;
    char patch[50];
    char orig[50];
};

typedef HRESULT (WINAPI *dinp8crt_t)(HINSTANCE, DWORD, REFIID,
				     LPVOID *, LPUNKNOWN);
dinp8crt_t oDirectInput8Create;

__attribute__ ((dllexport))
HRESULT WINAPI DirectInput8Create(HINSTANCE inst, DWORD ver, REFIID id,
				  LPVOID *pout, LPUNKNOWN outer)
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
	
	
    void *base_addr = GetModuleHandle(NULL);
	
	//
	//  Add the "Open No Baby Jump Mod" code
	//
	
	static char spd_bytes[] =
	{
		// CE newmem:
		0x50, 										// push eax
		0x8B, 0xC6, 								// mov eax, esi
		0x3C, 0x10, 								// cmp al,10
		0x75, 0x0A, 								// jne code
		0x0F, 0x1F, 0x40, 0x00, 					// nop dword ptr [eax+00]
		0x89, 0x35, 0x00, 0x00, 0x00, 0x00,			// mov [speedBse_xxxx],esi
		0x58, 										// code: pop eax
		0x0F, 0x11, 0x96, 0xE0, 0x00, 0x00, 0x00, 	// movups [esi+000000E0],xmm2
		0xE9, 0x00, 0x00, 0x00, 0x00, 				// jmp return_xxxx
		// speedBse:
		0x00, 0x00, 0x00, 0x00, 
	};
	void *spdhook = spd_bytes;
	void *speedBse = &spd_bytes[30];
	
	struct patch spd_patch = 
	    {
			0x3AAB05, 										// "GetSpdPtrAOB"  
			7,
			{0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90,}, 	// jmp [xxxx], nop, nop
			{0x0F, 0x11, 0x96, 0xE0, 0x00, 0x00, 0x00,}, 	// movups [esi + 000000E0],xmm2
	    };
		
	void *spdinjaddr = base_addr + spd_patch.rel_addr;
	void *spdinj_ret = spdinjaddr + 7;
	
	// Calc and update addresses
	int jmpfw_offset 	= spdhook - (spdinjaddr + 5);
	int jmpret_offset 	= spdinj_ret - ((void*)&spd_bytes[25] + 5);
	
	memcpy(&spd_patch.patch[1],&jmpfw_offset,4); 		// jump to spd_bytes code
	memcpy(&spd_bytes[26],&jmpret_offset,4); 			// return to spdinject (+offset)
	//	
	memcpy(&spd_bytes[13],&speedBse,4); 			// fix mov [speedBse]
	
	
	//
	DWORD op;
	VirtualProtect(spd_bytes, sizeof(spd_bytes),
		       PAGE_EXECUTE_READWRITE, &op);
	
	// Edit memory:
	DWORD size = spd_patch.size;
	if (memcmp(spdinjaddr, spd_patch.orig, size) == 0) {
		DWORD old;
		VirtualProtect(spdinjaddr, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(spdinjaddr, spd_patch.patch, size);
		VirtualProtect(spdinjaddr, size, old, &old);
	}
	
	/*
	
	//	 Add the "no baby jump variant 1 (full)" code
	//
	static char jump_bytes[] =
	{
		// CE newmem:
		0x8B, 0xC8,										// mov ecx, eax
		0x8B, 0x42, 0x10, 								// mov eax,[edx+10]
		0x0F, 0x51, 0xC0, 								// sqrtps xmm0,xmm0
		0x50, 											// push eax
		0xA1, 0x00, 0x00, 0x00, 0x00, 					// mov eax,[speedBse_xxxx]
		0xF3, 0x0F, 0x10, 0x80, 0xE8, 0x00, 0x00, 0x00, // movss xmm0, [eax+E8]
		0x58, 											// pop eax
		0xE9, 0x00, 0x00, 0x00, 0x00, 					// jmp [xxxx]
	};
	void *jumphook = jump_bytes;
		
	struct patch jump_patch = 
	    {
			0x3A09C4, 											// "NoBabyJAOB1"  
			8,
			{0xE9, 0x00, 0x00, 0x00, 0x00, 0x90, 0x90, 0x90}, 	// jmp [xxxx], nop, nop, nop
			{0x8b, 0xC8, 0x8B, 0x42, 0x10, 0x0F, 0x51, 0xC0}, 	// movups [esi + 000000E0],xmm2
	    };
		
	void *bbjinjaddr = base_addr + jump_patch.rel_addr;
	char *nobbjinj_ret = (char*) bbjinjaddr + 8;
	
	// Calc and update addresses
	memcpy(&jump_patch.patch[1],&jumphook,4); 		// jump to jumphook_bytes code
	memcpy(&jump_bytes[10],&speedBse,4); 			// fix mov eax [speedBse]
	memcpy(&jump_bytes[24],&nobbjinj_ret,4); 		// return to bbjinject (+offset)
	
	// Edit memory:
	size = jump_patch.size;
	if (memcmp(bbjinjaddr, jump_patch.orig, size) == 0) {
		DWORD old;
		VirtualProtect(bbjinjaddr, size, PAGE_EXECUTE_READWRITE, &old);
		memcpy(bbjinjaddr, jump_patch.patch, size);
		VirtualProtect(bbjinjaddr, size, old, &old);
	}
	*/
	
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
