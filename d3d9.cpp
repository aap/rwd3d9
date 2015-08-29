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
RwUInt32 &LastPixelShader = *AddressByVersion<RwUInt32*>(0x619478, 0x6DDE2C);
RwUInt32 &Last8VertexShader = *AddressByVersion<RwUInt32*>(0x619474, 0x6DDE28);
void *Last9VertexShader = INVALID;
RwUInt32 LastFVF = 0xFFFFFFFF;
void *LastVertexDecl = INVALID;

IUnknown *&RwD3DDevice = *AddressByVersion<IUnknown**>(0x662EF0, 0x7897A8);

static uint32_t D3D8DeviceSystemStart_A = AddressByVersion<uint32_t>(0x5B7A50, 0x65BFC0); 
static uint32_t RwD3D8GetCurrentD3DDevice_A = AddressByVersion<uint32_t>(0x5BA590, 0x65F140); 
WRAPPER int D3D8DeviceSystemStart(void) { VARJMP(D3D8DeviceSystemStart_A); }
WRAPPER void *RwD3D8GetCurrentD3DDevice(void) { VARJMP(RwD3D8GetCurrentD3DDevice_A); }

// invalidate d3d9 cache when d3d8 shader changes
static uint32_t setshaderhook_R = AddressByVersion<uint32_t>(0x5BAF9A, 0x65F2FA); 
void __declspec(naked)
setshaderhook(void)
{
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
	MemoryVP::InjectHook(AddressByVersion<uint32_t>(0x5B76B0, 0x65BB1F), D3D8DeviceSystemStart_hook);
	if(RwD3DDevice)
		RwD3DDevice->QueryInterface(IID_IDirect3DDevice9, (void**)&d3d9device);

	MemoryVP::InjectHook(setshaderhook_R-5, setshaderhook, PATCH_JUMP);
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
	if(reason == DLL_PROCESS_ATTACH)
		return d3d9attach();

	return TRUE;
}
