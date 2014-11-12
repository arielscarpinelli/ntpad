//////////////////////////////////////////////////////////////////////
//
// ForceFeedbackClassFactory.cpp: implementation of the CForceFeedbackClassFactory class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CForceFeedbackClassFactory.h"
#include "CForceFeedbackImpl.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction

CForceFeedbackClassFactory::CForceFeedbackClassFactory()
{
    m_uRefCount = 0;
    g_uDllLockCount++;

    TRACE(">>> ForceFeedbackSvr: Constructing a CForceFeedbackClassFactory, DLL ref count = %d\n", g_uDllLockCount );
}

CForceFeedbackClassFactory::~CForceFeedbackClassFactory()
{
    g_uDllLockCount--;

    TRACE(">>> ForceFeedbackSvr: Destructing a CForceFeedbackClassFactory, DLL ref count = %d\n", g_uDllLockCount );
}


//////////////////////////////////////////////////////////////////////
// IUnknown methods

STDMETHODIMP_(ULONG) CForceFeedbackClassFactory::AddRef()
{
    TRACE(">>> ForceFeedbackSvr: CForceFeedbackClassFactory::AddRef(), new ref count = %d\n", m_uRefCount+1 );

    return ++m_uRefCount;               // Increment this object's reference count.
}

STDMETHODIMP_(ULONG) CForceFeedbackClassFactory::Release()
{
ULONG uRet = --m_uRefCount;             // Decrement this object's reference count.
    
    TRACE(">>> ForceFeedbackSvr: CForceFeedbackClassFactory::Release(), new ref count = %d\n", m_uRefCount );

    if ( 0 == m_uRefCount )             // Releasing last reference?
        delete this;

    return uRet;
}

STDMETHODIMP CForceFeedbackClassFactory::QueryInterface ( REFIID riid, void** ppv )
{
HRESULT hrRet = S_OK;

    TRACE(">>> ForceFeedbackSvr: In CForceFeedbackClassFactory::QueryInterface()\n" );

    // Check that ppv really points to a void*.

    if ( IsBadWritePtr ( ppv, sizeof(void*) ))
        return E_POINTER;

    // Standard QI initialization - set *ppv to NULL.

    *ppv = NULL;

    // If the client is requesting an interface we support, set *ppv.

    if ( ::ATL::InlineIsEqualGUID ( riid, IID_IUnknown ))
        {
        *ppv = (IUnknown*) this;
        TRACE(">>> ForceFeedbackSvr: Client QIed for IUnknown\n" );
        }
    else if ( ::ATL::InlineIsEqualGUID ( riid, IID_IClassFactory ))
        {
        *ppv = (IClassFactory*) this;
        TRACE(">>> ForceFeedbackSvr: Client QIed for IClassFactory\n" );
        }
    else
        {
        hrRet = E_NOINTERFACE;
        TRACE(">>> ForceFeedbackSvr: Client QIed for unsupported interface\n" );
        }

    // If we're returning an interface pointer, AddRef() it.

    if ( S_OK == hrRet )
        {
        ((IUnknown*) *ppv)->AddRef();
        }

    return hrRet;
}


//////////////////////////////////////////////////////////////////////
// IClassFactory methods

STDMETHODIMP CForceFeedbackClassFactory::CreateInstance ( IUnknown* pUnkOuter,
                                                         REFIID    riid,
                                                         void**    ppv )
{
HRESULT hrRet;
CForceFeedbackImpl* pMsgbox;

    TRACE(">>> ForceFeedbackSvr: In CForceFeedbackClassFactory::CreateInstance()\n" );

    // We don't support aggregation, so pUnkOuter must be NULL.

    if ( NULL != pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    // Check that ppv really points to a void*.

    if ( IsBadWritePtr ( ppv, sizeof(void*) ))
        return E_POINTER;

    *ppv = NULL;

    // Create a new COM object!

    pMsgbox = new CForceFeedbackImpl;

    if ( NULL == pMsgbox )
        return E_OUTOFMEMORY;

    // QI the object for the interface the client is requesting.

    hrRet = pMsgbox->QueryInterface ( riid, ppv );

    // If the QI failed, delete the COM object since the client isn't able
    // to use it (the client doesn't have any interface pointers on the object).

    if ( FAILED(hrRet) )
        delete pMsgbox;

    return hrRet;
}

STDMETHODIMP CForceFeedbackClassFactory::LockServer ( BOOL fLock )
{
    // Increase/decrease the DLL ref count, according to the fLock param.

    fLock ? g_uDllLockCount++ : g_uDllLockCount--;

    TRACE(">>> ForceFeedbackSvr: In CForceFeedbackClassFactory::LockServer(), new DLL ref count = %d\n", g_uDllLockCount );
    
    return S_OK;
}
