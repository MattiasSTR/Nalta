#pragma once

#ifndef N_SHIPPING
    #define DX12_SET_NAME(aObject, aName) aObject->SetName(L##aName)
    #define DX12_SET_NAME_W(aObject, aName) aObject->SetName(aName)
#else
    #define DX12_SET_NAME(aObject, aName) ((void)0)
    #define DX12_SET_NAME_W(aObject, aName) ((void)0)
#endif