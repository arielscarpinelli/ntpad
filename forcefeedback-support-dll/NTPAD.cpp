//////////////////////////////////////////////////////////////////////
//
// NTPAD.cpp : Defines the entry point for the DLL application.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CForceFeedbackClassFactory.h"
#include "CForceFeedbackImpl.h"
#include "ForceFeedbackComDef.h"
#include "stdio.h"


HINSTANCE g_hinstThisDll = NULL;        // Our DLL's module handle
UINT      g_uDllLockCount = 0;          // # of COM objects in existence
WINOLEAPI  CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);

//////////////////////////////////////////////////////////////////////////////
// Global functions

// DllMain() is our DLL's entry point.  Windows calls this when a process or thread
// loads and unloads the DLL.
BOOL APIENTRY DllMain ( HINSTANCE hModule, 
                        DWORD     ul_reason_for_call, 
                        LPVOID    lpReserved )
{
	switch ( ul_reason_for_call )
        {
        case DLL_PROCESS_ATTACH:
            {
            g_hinstThisDll = hModule;

            TRACE(">>> ForceFeedbackSvr: Loading server DLL.\n");

            // Calling DisableThreadLibraryCalls() prevents DllMain() from 
            // getting called for every thread that attaches/detaches from
            // our DLL.

            DisableThreadLibraryCalls ( hModule );
            }
        break;

        case DLL_PROCESS_DETACH:
            {
            TRACE(">>> ForceFeedbackSvr: Unloading server DLL.\n");
            return g_uDllLockCount > 0 ? FALSE : TRUE;
			}
        break;
        }

    return TRUE;
}

// DllGetClassObject() is called when COM needs to get a class factory.
STDAPI DllGetClassObject ( REFCLSID rclsid, REFIID riid, void** ppv )
{
HRESULT hrRet;
CForceFeedbackClassFactory* pFactory;

    TRACE(">>> ForceFeedbackSvr: In DllGetClassObject()\n");

    // Check that the client is asking for the CForceFeedbackImpl factory.

    if ( !::ATL::InlineIsEqualGUID ( rclsid, __uuidof(CLSIDForceFeedbackImpl) ))
        return CLASS_E_CLASSNOTAVAILABLE;

    // Check that ppv really points to a void*.

    if ( IsBadWritePtr ( ppv, sizeof(void*) ))
        return E_POINTER;

    *ppv = NULL;

    // Construct a new class factory object.

    pFactory = new CForceFeedbackClassFactory;

    if ( NULL == pFactory )
        return E_OUTOFMEMORY;

    // AddRef() the factory since we're using it.

    pFactory->AddRef();

    // QI() the factory for the interface the client wants.

    hrRet = pFactory->QueryInterface ( riid, ppv );
    
    // We're done with the factory, so Release() it.

    pFactory->Release();

    return hrRet;
}

// DllCanUnloadNow() is called when COM wants to unload our DLL from memory.
// We check our lock count, which will be nonzero if there are any COM
// objects still in memory.
// Return S_FALSE to prevent the DLL from being unloaded, or S_OK to let it
// be unloaded.
STDAPI DllCanUnloadNow()
{
    TRACE(">>> ForceFeedbackSvr: In DllCanUnloadNow(), ref count = %d\n", g_uDllLockCount );

    return g_uDllLockCount > 0 ? S_FALSE : S_OK;
}

// DllRegisterServer() creates the registy entries that tells COM where our 
// server is located and its threading model.
STDAPI DllRegisterServer()
{
HKEY  hCLSIDKey = NULL, hInProcSvrKey = NULL;
LONG  lRet;
TCHAR szModulePath [MAX_PATH];
TCHAR szClassDescription[] = _T("NTPAD ForceFeeback DLL");
TCHAR szThreadingModel[]   = _T("Both");

__try
    {
    // Create a key under CLSID for our COM server.

    lRet = RegCreateKeyEx ( HKEY_CLASSES_ROOT, _T("CLSID\\{5B153A12-D1BD-4F3D-A5C7-EE16D5A1186C}"),
                            0, NULL, REG_OPTION_NON_VOLATILE, KEY_SET_VALUE | KEY_CREATE_SUB_KEY,
                            NULL, &hCLSIDKey, NULL );
    
    if ( ERROR_SUCCESS != lRet ) 
        return HRESULT_FROM_WIN32(lRet);

    // The default value of the key is a human-readable description of the coclass.

    lRet = RegSetValueEx ( hCLSIDKey, NULL, 0, REG_SZ, (const BYTE*) szClassDescription,
                           sizeof(szClassDescription) );

    if ( ERROR_SUCCESS != lRet )
        return HRESULT_FROM_WIN32(lRet);
    
    // Create the InProcServer32 key, which holds info about our coclass.

    lRet = RegCreateKeyEx ( hCLSIDKey, _T("InProcServer32"), 0, NULL, REG_OPTION_NON_VOLATILE,
                            KEY_SET_VALUE, NULL, &hInProcSvrKey, NULL );

    if ( ERROR_SUCCESS != lRet )
        return HRESULT_FROM_WIN32(lRet);

    // The default value of the InProcServer32 key holds the full path to our DLL.

    GetModuleFileName ( g_hinstThisDll, szModulePath, MAX_PATH );

    lRet = RegSetValueEx ( hInProcSvrKey, NULL, 0, REG_SZ, (const BYTE*) szModulePath, 
                           sizeof(TCHAR) * (lstrlen(szModulePath)+1) );

    if ( ERROR_SUCCESS != lRet )
        return HRESULT_FROM_WIN32(lRet);

    // The ThreadingModel value tells COM how it should handle threads in our DLL.
    // The concept of apartments is beyond the scope of this article, but for
    // simple, single-threaded DLLs, use Apartment.

    lRet = RegSetValueEx ( hInProcSvrKey, _T("ThreadingModel"), 0, REG_SZ,
                           (const BYTE*) szThreadingModel, 
                           sizeof(szThreadingModel) );

    if ( ERROR_SUCCESS != lRet )
        return HRESULT_FROM_WIN32(lRet);
    }   
__finally
    {
    if ( NULL != hCLSIDKey )
        RegCloseKey ( hCLSIDKey );

    if ( NULL != hInProcSvrKey )
        RegCloseKey ( hInProcSvrKey );
    }

    return S_OK;
}

// DllUnregisterServer() deleted the registy entries that DllRegisterServer() created.
STDAPI DllUnregisterServer()
{
    // Delete our registry entries.  Note that you must delete from the deepest
    // key and work upwards, because on NT/2K, RegDeleteKey() doesn't delete 
    // keys that have subkeys on NT/2K.

    RegDeleteKey ( HKEY_CLASSES_ROOT, _T("CLSID\\{5B153A12-D1BD-4F3D-A5C7-EE16D5A1186C}\\InProcServer32") );
    RegDeleteKey ( HKEY_CLASSES_ROOT, _T("CLSID\\{5B153A12-D1BD-4F3D-A5C7-EE16D5A1186C}") );

    return S_OK;
}

DWORD ThreadProc( DWORD dwThis )
{

	if(CoInitializeEx(NULL, 0x0)==S_OK) // enter the MTA
	{
        TRACE("Entre en modo multihread");
		if (true) // scope for smart ptr
        {
            CComPtr<CForceFeedbackImpl> foo = reinterpret_cast<CForceFeedbackImpl*>(dwThis);
			foo->MatarTimer();
            while ( WaitForSingleObject( foo->hFinishEvent, 1 ) == WAIT_TIMEOUT )
                foo->Actualize();
        }

        CoUninitialize();

		ExitThread(0);
	};
	return 0;
}

#define OLE_APT_MASK    0x080
#define OLE_FREE_MASK   0x140
/*----------------------------------------------------------------------
FUNCTION:   DebugCoGetThreadingModel
DISCUSSION: Returns the COM threading model for the current thread.
PARAMETERS: None.
RETURNS:
    COINIT_APARTMENTTHREADED - The thread was initialized with the
                               apartment threading model.
    COINIT_MULTITHREADED     - The thread was initialized with the
                               free threading model.
    0xFF                     - The thread was not initialized for COM.
----------------------------------------------------------------------*/
COINIT DebugCoGetThreadingModel ( void )
{
    DWORD dwOLETLS ;    // Get the OLE thread structure out of the TEB block.    // Get the TEB.
    __asm MOV EAX , FS:018h
    // Get the COM TLS structure from its offset.
    __asm MOV EAX , [EAX + 0F80h]
    __asm MOV dwOLETLS , EAX    // If the value is zero, OLE has not been initialized.
    if ( 0 == dwOLETLS )
    {
        return ( (COINIT)0xFF ) ;
    }    COINIT coRet = (COINIT)0xFF ;
    __try
    {        // Get the threading model flag, which is 0xC bytes into the
        // structure.
        DWORD dwFlags = *(LPDWORD)((dwOLETLS + 0xC)) ;        if ( OLE_APT_MASK == ( dwFlags & OLE_APT_MASK ) )
        {
            coRet = (COINIT)0x2 ;
        }
        if ( OLE_FREE_MASK == ( dwFlags & OLE_FREE_MASK ) )
        {
            coRet = (COINIT)0x0 ;
        }
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        coRet = (COINIT)0xFF ;
    }
    return ( coRet ) ;
}

#ifdef _DEBUG 
//////////////////////////////////////////////////////////////////////////////
// Diagnostic trace stolen shamelessly from MFC :)

#include <stdio.h>

void TRACE (LPCTSTR lpszFormat, ...)
{
FILE* tracer;
va_list args;
int     nBuf;
TCHAR   szBuffer[512];

    va_start(args, lpszFormat);

    nBuf = _vsntprintf(szBuffer, 512, lpszFormat, args);

	OutputDebugString ( szBuffer );
	tracer = fopen("C:\\ntpaddll.log", "a+");
	if(tracer)
	{
		fprintf(tracer, "%s", szBuffer);
		fclose(tracer);
	};

    va_end(args);
}
#endif // ifdef _DEBUG

