#include <windows.h>
#include <stdint.h>

typedef long RwFixed;
typedef int  RwInt32;
typedef unsigned int RwUInt32;
typedef short RwInt16;
typedef unsigned short RwUInt16;
typedef unsigned char RwUInt8;
typedef signed char RwInt8;
typedef char RwChar;
typedef float RwReal;
typedef RwInt32 RwBool;

#include "rwd3d9.h"
#include <initguid.h>
#include <d3d9.h>
#include <d3d9types.h>
#include "MemoryMgr.h"

#define INVALID ((void*)0xFFFFFFFF)

IDirect3DDevice9 *d3d9device = NULL;
RwUInt32 &LastPixelShader = *AddressByVersion<RwUInt32*>(0x619478, 0x618B40, 0x625ACC, 0x6DDE2C, 0x6DDE1C, 0x6DCDC4);
RwUInt32 &Last8VertexShader = *AddressByVersion<RwUInt32*>(0x619474, 0x618B3C, 0x625AD0, 0x6DDE28, 0x6DDE18, 0x6DCDC0);
void *Last9VertexShader = INVALID;
RwUInt32 LastFVF = 0xFFFFFFFF;
void *LastVertexDecl = INVALID;

IUnknown *&RwD3DDevice = *AddressByVersion<IUnknown**>(0x662EF0, 0x662EF0, 0x67342C, 0x7897A8, 0x7897B0, 0x7887B0);

static uint32_t D3D8DeviceSystemStart_A = AddressByVersion<uint32_t>(0x5B7A50, 0x5B7D10, 0x5BC470, 0x65BFC0, 0x65C010, 0x65AF70); 
static uint32_t RwD3D8GetCurrentD3DDevice_A = AddressByVersion<uint32_t>(0x5BA590, 0x5BA850, 0, 0x65F140, 0x65F190, 0x65E0F0); 
WRAPPER int D3D8DeviceSystemStart(void) { VARJMP(D3D8DeviceSystemStart_A); }



// override Im2d pixel shader
static uint32_t RwD3D8SetPixelShader_A = AddressByVersion<uint32_t>(0x5BAFD0, 0x5BB290, 0x5BF4A0, 0x65F330, 0x65F380, 0x65E2E0);
WRAPPER RwBool RwD3D8SetPixelShader(RwUInt32 handle) { VARJMP(RwD3D8SetPixelShader_A); }

static void *overridePS = NULL;

static RwBool
RwD3D8SetPixelShader_hook(RwUInt32 handle)
{
	if(overridePS){
		RwD3D9SetPixelShader(overridePS);
		return 1;
	}
	return RwD3D8SetPixelShader(handle);
}

static void
hookPixelShader(void)
{
	if(isVC()){
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0, 0, 0, 0x6666B8, 0x666708, 0x665668), RwD3D8SetPixelShader_hook);
		MemoryVP::InjectHook(AddressByVersion<uint32_t>(0, 0, 0, 0x666928, 0x666978, 0x6658D8), RwD3D8SetPixelShader_hook);
	}else{
		if (gtaversion == III_STEAM)
			MemoryVP::InjectHook(0x5C353A, RwD3D8SetPixelShader_hook);
		else{
			MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5BF9D6, 0x5BFC96, 0, 0, 0, 0), RwD3D8SetPixelShader_hook);
			MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5BFBD3, 0x5BFE93, 0, 0, 0, 0), RwD3D8SetPixelShader_hook);
		}
	}
}

void
RwD3D9SetIm2DPixelShader(void *ps)
{
	overridePS = ps;
}


// Lazy stuff =P
void *RwD3D8GetCurrentD3DDevice_Steam()
{
	auto v0 = *(DWORD*)0x672F94;
	*(DWORD*)0x672F8C = 0;
	if ( *(DWORD*)0x672F94 )
	{
		DWORD v1 = 0;
		/*just*/ do /*it*/
		{
			int v2 = *(int *)((char *)*(DWORD*)0x672F98 + v1);
			v1 += 4;
			if ( v2 )
			{
				(*(void (__cdecl **)(int, int))(*(DWORD*)0x671248 + 324))(v0, v2);
				*(int *)((char *)0x672F94 + v1) = 0;
				v0 = *(DWORD*)0x672F94;
			}
		}
		while ( v1 < 0x410 );
		(*(void (__cdecl **)(int))0x5C69C0)(v0); //sub_5C69C0(v0);
	}
	else
	{
		*(DWORD*)0x672F94 = (*(int (__cdecl **)(int, int, int))0x5C6710)(64, 15, 16); //sub_5C6710(64, 15, 16);
		memset((void*)*(DWORD*)0x672F98, 0, 0x410u);
	}
	*(DWORD*)0x673398 = (*(int (__cdecl **)(int))(*(DWORD*)0x671248 + 320))(*(DWORD*)0x672F94);
	memcpy((void*)*(DWORD*)0x673398, (void*)0x62B8E0, 0x40u);
	return (void *)*(DWORD*)0x673398;
}

WRAPPER void *RwD3D8GetCurrentD3DDevice(void)
{
	if (gtaversion != III_STEAM)
		VARJMP(RwD3D8GetCurrentD3DDevice_A);
	__asm
	{
		call RwD3D8GetCurrentD3DDevice_Steam
		retn
	}
}

// invalidate d3d9 cache when d3d8 shader changes
static uint32_t setshaderhook_R = AddressByVersion<uint32_t>(0x5BAF9A, 0x5BB25A, 0x5BF451, 0x65F2FA, 0x65F34A, 0x65E2AA); 
void __declspec(naked)
setshaderhook(void)
{
	if (gtaversion == III_STEAM) // 0x5BF449
	{
		_asm
		{
			mov	Last9VertexShader, 0xFFFFFFFF
			mov	LastFVF, 0xFFFFFFFF
			mov	LastVertexDecl, 0xFFFFFFFF
			mov     eax, Last8VertexShader
			mov     ebx, [ebp+8]
			push	setshaderhook_R
			retn
		}

	}

	_asm{
		mov	Last9VertexShader, 0xFFFFFFFF
		mov	LastFVF, 0xFFFFFFFF
		mov	LastVertexDecl, 0xFFFFFFFF
		push	esi
		mov	esi, [esp+8]
		push	setshaderhook_R
		retn
	}
}


// get d3d9 device when d3d8 device is created
int
D3D8DeviceSystemStart_hook(void)
{
	if(D3D8DeviceSystemStart() == 0)
		return 0;
	RwD3DDevice->QueryInterface(IID_IDirect3DDevice9, (void**)&d3d9device);
	return 1;
}

BOOL
d3d9attach(void)
{
	MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5B76B0, 0x5B7970, 0x5BD700, 0x65BB1F, 0x65BB6F, 0x65AACF), D3D8DeviceSystemStart_hook);
	if(RwD3DDevice)
		RwD3DDevice->QueryInterface(IID_IDirect3DDevice9, (void**)&d3d9device);

	if (gtaversion == III_STEAM)
		MemoryVP::InjectHook(0x5BF449, setshaderhook, PATCH_JUMP);
	else
		MemoryVP::InjectHook(setshaderhook_R-5, setshaderhook, PATCH_JUMP);

	hookPixelShader();
	return TRUE;
}

#ifdef    __cplusplus
extern "C"
{
#endif

RwBool
RwD3D9Supported(void)
{
	return d3d9device != NULL;
}

void*
RwD3D9GetCurrentD3DDevice(void)
{
	// just to clear the cache
	RwD3D8GetCurrentD3DDevice();
	return d3d9device;
}

RwBool
RwD3D9CreateVertexShader(const RwUInt32 *function, void **shader)
{
	HRESULT res = d3d9device->CreateVertexShader((DWORD*)function, (IDirect3DVertexShader9**)shader);
	return res >= 0;
}

RwBool
RwD3D9CreatePixelShader(const RwUInt32 *function, void **shader)
{
	HRESULT res = d3d9device->CreatePixelShader((DWORD*)function, (IDirect3DPixelShader9**)shader);
	return res >= 0;
}

void
RwD3D9SetVertexShader(void *shader)
{
	if(Last9VertexShader != shader){
		Last8VertexShader = 0xFFFFFFFF;
		if(d3d9device->SetVertexShader((IDirect3DVertexShader9*)shader) < 0)
			Last9VertexShader = INVALID;
		else
			Last9VertexShader = shader;
	}
}

void
RwD3D9SetPixelShader(void *shader)
{
	if(LastPixelShader != (RwUInt32)shader){
		if(d3d9device->SetPixelShader((IDirect3DPixelShader9*)shader) < 0)
			LastPixelShader = 0xFFFFFFFF;
		else
			LastPixelShader = (RwUInt32)shader;
	}
}

void
RwD3D9SetFVF(RwUInt32 fvf)
{
	if(LastFVF != fvf){
		Last8VertexShader = 0xFFFFFFFF;
		if(d3d9device->SetFVF(fvf) < 0)
			LastFVF = 0xFFFFFFFF;
		else
			LastFVF = fvf;
	}
}

void
RwD3D9SetVertexDeclaration(void *vertexDeclaration)
{
	if(LastVertexDecl != vertexDeclaration){
		Last8VertexShader = 0xFFFFFFFF;
		if(d3d9device->SetVertexDeclaration((IDirect3DVertexDeclaration9*)vertexDeclaration) < 0)
			LastVertexDecl = INVALID;
		else
			LastVertexDecl = vertexDeclaration;
	}
}

void
RwD3D9SetVertexShaderConstant(RwUInt32 registerAddress, const void *constantData, RwUInt32 constantCount)
{
	d3d9device->SetVertexShaderConstantF(registerAddress, (const float*)constantData, constantCount);
}

void
RwD3D9SetPixelShaderConstant(RwUInt32 registerAddress, const void *constantData, RwUInt32 constantCount)
{
	d3d9device->SetPixelShaderConstantF(registerAddress, (const float*)constantData, constantCount);
}

void
RwD3D9SetVertexPixelShaderConstant(RwUInt32 registerAddress, const void *constantData, RwUInt32 constantCount)
{
	RwD3D9SetVertexShaderConstant(registerAddress, constantData, constantCount);
	RwD3D9SetPixelShaderConstant(registerAddress, constantData, constantCount);
}

#ifdef    __cplusplus
}
#endif

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH){
		AddressByVersion<uint32_t>(0, 0, 0, 0, 0, 0);
		if(gtaversion != -1)
			return d3d9attach();
	}

	return TRUE;
}
