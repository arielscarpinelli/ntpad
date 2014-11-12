//////////////////////////////////////////////////////////////////////
//
// IForceFeedback interface definition
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISIMPLEMSG_H__E66A448D_57A8_448B_B78D_E86E8A66F098__INCLUDED_)
#define AFX_ISIMPLEMSG_H__E66A448D_57A8_448B_B78D_E86E8A66F098__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "dinput.h"
#include "dinputd.h"

struct IForceFeedback : public IUnknown
{
    // IUnknown
    STDMETHOD_(ULONG, AddRef)() PURE;
    STDMETHOD_(ULONG, Release)() PURE;
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv) PURE;

    // IForceFeedback
    STDMETHOD(DeviceID)(THIS_ DWORD,DWORD,DWORD,DWORD,LPVOID) PURE;
    STDMETHOD(GetVersions)(THIS_ LPDIDRIVERVERSIONS) PURE;
    STDMETHOD(Escape)(THIS_ DWORD,DWORD,LPDIEFFESCAPE) PURE;
    STDMETHOD(SetGain)(THIS_ DWORD,DWORD) PURE;
    STDMETHOD(SendForceFeedbackCommand)(THIS_ DWORD,DWORD) PURE;
    STDMETHOD(GetForceFeedbackState)(THIS_ DWORD,LPDIDEVICESTATE) PURE;
    STDMETHOD(DownloadEffect)(THIS_ DWORD,DWORD,LPDWORD,LPCDIEFFECT,DWORD) PURE;
    STDMETHOD(DestroyEffect)(THIS_ DWORD,DWORD) PURE;
    STDMETHOD(StartEffect)(THIS_ DWORD,DWORD,DWORD,DWORD) PURE;
    STDMETHOD(StopEffect)(THIS_ DWORD,DWORD) PURE;
    STDMETHOD(GetEffectStatus)(THIS_ DWORD,DWORD,LPDWORD) PURE;
};

#endif // defined(AFX_ISIMPLEMSG_H__E66A448D_57A8_448B_B78D_E86E8A66F098__INCLUDED_)