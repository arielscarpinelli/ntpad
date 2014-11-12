//===========================================================================
// CPLSVR1.H
//
//===========================================================================

//===========================================================================
// (C) Copyright 1998 Microsoft Corp.  All rights reserved.
//
// You have a royalty-free right to use, modify, reproduce and
// distribute the Sample Files (and/or any modified version) in
// any way you find useful, provided that you agree that
// Microsoft has no warranty obligations or liability for any
// Sample Application Files which are modified.
//===========================================================================

#pragma pack(8)

//---------------------------------------------------------------------------

#ifndef _CPLSVR1_H
#define _CPLSVR1_H
#define ISOLATION_AWARE_ENABLED 1
#define UNICODE
//---------------------------------------------------------------------------
#define INC_OLE2

#ifndef PPVOID
#define PPVOID  LPVOID*
#endif

// included headers
#define STRICT
#include <windows.h>
#include <objbase.h>
#define DIRECTINPUT_VERSION 0x700
#include <dinput.h>
#include <dinputd.h>
#include <dicpl.h>
extern "C" {
   #include <hidsdi.h>
};
#include <setupapi.h>
#include <assert.h>
#include <prsht.h>
#include "resrc1.h"
#include "resource.h"

// symbolic constants
#define ID_POLLTIMER    50
#define NUMPAGES        5

#define STR_LEN_32		32
#define STR_LEN_64		64
#define STR_LEN_128		128

// data structures
typedef struct _CPLPAGEINFO
{
    WCHAR  name[25];
    UINT strnameid;
    LPWSTR  lpwszDlgTemplate;
    DLGPROC fpPageProc;
} CPLPAGEINFO;

// global variables
extern HINSTANCE            ghInst;
extern CRITICAL_SECTION     gcritsect;


// prototypes
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv);
STDAPI DllCanUnloadNow(void);
extern BOOL RegisterCustomButtonClass();
void SetButtonState(HWND hDlg, BYTE nButton, BOOL bState );

// GUIDs used by our CPL
// CPLCLSID="{BBB7F0C0-6673-4f74-A408-726EDCA8AF82}"
const GUID CLSID_CplSvr1 = 
{ 0xBBB7F0C0, 0x6673, 0x4f74, { 0xa4, 0x08, 0x72, 0x6e, 0xdc, 0xa8, 0xaf, 0x82 } };
//* NULL_GUID {00000000-0000-0000-0000-000000000000}
const GUID NULL_GUID = 
{ 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 } };

// derive a new class from CDIGameCntrlPropSheet
//
// we want to store some additional data here
class CDIGameCntrlPropSheet_X : public IDIGameCntrlPropSheet
{
    private:
    short				    m_cProperty_refcount;
	short				    m_nID;

    public:
    CDIGameCntrlPropSheet_X(void);
    ~CDIGameCntrlPropSheet_X(void);
    // IUnknown methods
	virtual STDMETHODIMP            QueryInterface(REFIID, PPVOID);
	virtual STDMETHODIMP_(ULONG)    AddRef(void);
	virtual STDMETHODIMP_(ULONG)    Release(void);
    // IDIGameCntrlPropSheet methods		
	virtual STDMETHODIMP			GetSheetInfo(LPDIGCSHEETINFO *ppSheetInfo);
	virtual STDMETHODIMP			GetPageInfo (LPDIGCPAGEINFO  *ppPageInfo );
	virtual STDMETHODIMP			SetID(USHORT nID);
	virtual STDMETHODIMP_(USHORT)	GetID(void)			{return m_nID;}
    virtual STDMETHODIMP            Initialize(void);
    virtual STDMETHODIMP            SetDevice(LPDIRECTINPUTDEVICE2 pdiDevice2);
    virtual STDMETHODIMP            GetDevice(LPDIRECTINPUTDEVICE2 *ppdiDevice2);
    virtual STDMETHODIMP            SetJoyConfig(LPDIRECTINPUTJOYCONFIG pdiJoyCfg);
    virtual STDMETHODIMP            GetJoyConfig(LPDIRECTINPUTJOYCONFIG *ppdiJoyCfg);
    virtual STDMETHODIMP            SetHIDHandle(HANDLE h);
    virtual STDMETHODIMP            GetHIDHandle(HANDLE *h);
    virtual STDMETHODIMP            GetConfighKey(HKEY *h);

    protected:
    DIGCSHEETINFO           m_digcSheetInfo;
    DIGCPAGEINFO            *m_pdigcPageInfo;
    LPDIRECTINPUTDEVICE2    m_pdiDevice2;
    LPDIRECTINPUTJOYCONFIG  m_pdiJoyCfg;
    WCHAR                   *m_wszSheetCaption;
    HANDLE                  m_hHID;
    HKEY                    m_ConfighKey;

};

//---------------------------------------------------------------------------
#endif // _CPLSVR1_H

























