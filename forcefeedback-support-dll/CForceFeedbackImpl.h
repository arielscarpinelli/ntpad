//////////////////////////////////////////////////////////////////////
//
// ForceFeedbackImpl.h: interface for the CForceFeedbackImpl class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ForceFeedbackIMPL_H__44BE02AE_9619_47AF_8070_4EDA06A1E45C__INCLUDED_)
#define AFX_ForceFeedbackIMPL_H__44BE02AE_9619_47AF_8070_4EDA06A1E45C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IForceFeedback.h"           // interface definition
struct EFECTO
{
	// General
	DWORD estado;
	DWORD tipo;
	UCHAR grande;
	UCHAR chico;
	LONG sample;
	LONG elapsed;
	LONG duration;
	DWORD started;
	DWORD trigger;
	DWORD veces;


	LONG gmin; // constant o ramp
	LONG gmax; // ramp
	LONG gcenter; // periodic
	
	// Envelope
	BOOL isenveloped;
	LONG attack;
	LONG actualmax;
	LONG attackduration;
	LONG fade;
	LONG actualmin;
	LONG fadeduration;

	// periodic
	DOUBLE phase;
    // custom
	LPLONG custom;
	DWORD size;
	int ejes; // condition
	int primereje;
	int muerta;
	int centro;
};
#define EFECTO_VACIO 2
#pragma pack(push, 1)

template <class T>
class CAuxThunk
{
  BYTE    m_mov;          // mov ecx, %pThis
  DWORD   m_this;         //
  BYTE    m_jmp;          // jmp func
  DWORD   m_relproc;      // relative jmp
public:
  typedef void (T::*TMFP)();
  void InitThunk(TMFP method, const T* pThis)
  {
    union { DWORD func; TMFP method; } addr;
    addr.method = (TMFP)method;
    m_mov = 0xB9;
    m_this = (DWORD)pThis;
    m_jmp = 0xE9;
    m_relproc = addr.func - (DWORD)(this+1);
    FlushInstructionCache(GetCurrentProcess(), this, sizeof(*this));
  }
  FARPROC GetThunk() const {
    _ASSERTE(m_mov == 0xB9);
    return (FARPROC)this; }
};
#pragma pack(pop)

class CForceFeedbackImpl : CAuxThunk<CForceFeedbackImpl>, public IForceFeedback  
{
public:
	CForceFeedbackImpl();
	virtual ~CForceFeedbackImpl();

    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID riid, void** ppv);

    // IForceFeedback
    STDMETHOD(DeviceID)(THIS_ DWORD,DWORD,DWORD,DWORD,LPVOID);
    STDMETHOD(GetVersions)(THIS_ LPDIDRIVERVERSIONS);
    STDMETHOD(Escape)(THIS_ DWORD,DWORD,LPDIEFFESCAPE);
    STDMETHOD(SetGain)(THIS_ DWORD,DWORD);
    STDMETHOD(SendForceFeedbackCommand)(THIS_ DWORD,DWORD);
    STDMETHOD(GetForceFeedbackState)(THIS_ DWORD,LPDIDEVICESTATE);
    STDMETHOD(DownloadEffect)(THIS_ DWORD,DWORD,LPDWORD,LPCDIEFFECT,DWORD);
    STDMETHOD(DestroyEffect)(THIS_ DWORD,DWORD);
    STDMETHOD(StartEffect)(THIS_ DWORD,DWORD,DWORD,DWORD);
    STDMETHOD(StopEffect)(THIS_ DWORD,DWORD);
    STDMETHOD(GetEffectStatus)(THIS_ DWORD,DWORD,LPDWORD);
	VOID TimerProc(HWND, UINT, UINT, DWORD);
	VOID Actualize();
	VOID MatarTimer();
	HANDLE hFinishEvent;



protected:
    ULONG m_uRefCount;
private:
	void ApplyEnvelope(int EffectId);
	UCHAR OndaCuadrada(double);
	double OndaTriangular(double);
	double OndaSawtooth(double, int);
	void CalculateForces(int i);
	void SetActuatorsState();
	HANDLE hDriver;
	HANDLE hThread;
	DWORD Ganancia;
	EFECTO Efectos[10];
	BOOLEAN pausa;
	BOOLEAN noactuators;
	BOOLEAN signaled;
	BOOLEAN working;
	BOOLEAN busy;
	DWORD lastcall;
	int oned;
	DWORD Id;
	UINT hTimer;
};

#endif // !defined(AFX_ForceFeedbackIMPL_H__44BE02AE_9619_47AF_8070_4EDA06A1E45C__INCLUDED_)
