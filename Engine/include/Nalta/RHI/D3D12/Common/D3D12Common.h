#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOGDI
#define NOGDI
#endif

#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxcapi.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

#ifndef N_SHIPPING
#define N_D3D12_SET_NAME(aObject, aName) aObject->SetName(L##aName)
#define N_D3D12_SET_NAME_W(aObject, aName) aObject->SetName(aName)
#else
#define N_D3D12_SET_NAME(aObject, aName) ((void)0)
#define N_D3D12_SET_NAME_W(aObject, aName) ((void)0)
#endif

#ifndef N_SHIPPING
#define NL_DX_VERIFY(hr) \
    do { HRESULT _hr = (hr); N_CORE_ASSERT(SUCCEEDED(_hr), "D3D12 call failed"); } while(0)
#else
#define NL_DX_VERIFY(hr) (hr)
#endif

template <class T> void SafeRelease(T& aPPT)
{
    if (aPPT)
    {
        aPPT->Release();
        aPPT = nullptr;
    }
}