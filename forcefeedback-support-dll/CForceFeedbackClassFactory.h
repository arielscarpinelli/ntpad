//////////////////////////////////////////////////////////////////////
//
// ForceFeedbackClassFactory.h: interface for the CForceFeedbackClassFactory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ForceFeedbackCLASSFACTORY_H__E66A448C_57A8_448B_B78D_E86E8A66F098__INCLUDED_)
#define AFX_ForceFeedbackCLASSFACTORY_H__E66A448C_57A8_448B_B78D_E86E8A66F098__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CForceFeedbackClassFactory : public IClassFactory
{
public:
    CForceFeedbackClassFactory();
    virtual ~CForceFeedbackClassFactory();

    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

    // IClassFactory
    STDMETHOD(CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppv);
    STDMETHOD(LockServer)(BOOL fLock);

protected:
    ULONG m_uRefCount;
};

#endif // !defined(AFX_ForceFeedbackCLASSFACTORY_H__E66A448C_57A8_448B_B78D_E86E8A66F098__INCLUDED_)
