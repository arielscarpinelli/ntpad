//////////////////////////////////////////////////////////////////////
//
// ForceFeedbackImpl.cpp: implementation of the CForceFeedbackImpl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CForceFeedbackImpl.h"
#include "ForceFeedbackComDef.h"         // for __uuidof info

#include <math.h>
#include <memory.h>
#define DIDFT_GETINSTANCE(n) LOWORD((n) >> 8)

extern "C" { 
	BOOLEAN __stdcall HidD_GetFeature(HANDLE, PVOID, ULONG);

};
DWORD ThreadProc( DWORD dwThis );
COINIT DebugCoGetThreadingModel ( void );
HRESULT DIUtilGetHIDDevice(LPDIRECTINPUTDEVICE2 pdiDevice2, HANDLE *hDriver);
HRESULT DIUtilCreateDevice2FromJoyConfig(short nJoystickId, HWND hWnd,
                            LPDIRECTINPUT pdi,
                            LPDIRECTINPUTJOYCONFIG pdiJoyCfg,
                            LPDIRECTINPUTDEVICE2 *ppdiDevice2);
//////////////////////////////////////////////////////////////////////
// Construction/Destruction

CForceFeedbackImpl::CForceFeedbackImpl()
{
    int i;
	DWORD a;
	m_uRefCount = 0;
    g_uDllLockCount++;
	ZeroMemory(Efectos, sizeof(EFECTO)*10);
	for(i=0; i<10; i++)	Efectos[i].estado = EFECTO_VACIO;
	pausa = 0;
	working = 0;
	busy = 0;
	noactuators = 0;
	signaled = 0;
	hDriver = INVALID_HANDLE_VALUE;
	Ganancia = 10000;
	lastcall = GetTickCount();
    InitThunk((TMFP)TimerProc, this);
	hTimer = SetTimer(NULL, NULL, 1, (TIMERPROC) this->GetThunk());
	if(DebugCoGetThreadingModel() != 0xFF)
	{
	  hFinishEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	  hThread = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE) ThreadProc, this,
				 NULL, &a);
	};
	TRACE(">>> ForceFeedbackSvr: Constructing a CForceFeedbackImpl, DLL ref count = %d\n", g_uDllLockCount );
}
CForceFeedbackImpl::~CForceFeedbackImpl()
{
    g_uDllLockCount--;

    TRACE(">>> ForceFeedbackSvr: Destructing a CForceFeedbackImpl, DLL ref count = %d\n", g_uDllLockCount );
}


//////////////////////////////////////////////////////////////////////
// IUnknown methods

STDMETHODIMP_(ULONG) CForceFeedbackImpl::AddRef()
{
    TRACE(">>> ForceFeedbackSvr: CForceFeedbackImpl::AddRef(), new ref count = %d\n", m_uRefCount+1 );
    return ++m_uRefCount;               // Increment this object's reference count.
}

STDMETHODIMP_(ULONG) CForceFeedbackImpl::Release()
{
ULONG uRet = --m_uRefCount;             // Decrement this object's reference count.

    TRACE(">>> ForceFeedbackSvr: CForceFeedbackImpl::Release(), new ref count = %d\n", m_uRefCount );

	if ( 0 == m_uRefCount )             // Releasing last reference?
    {
		if(hTimer==0)
		{
	  	  SetEvent(hFinishEvent);
		  WaitForSingleObject(hThread, INFINITE);
		} else {
			KillTimer(NULL, hTimer);
		};
		CloseHandle(hDriver);
		delete this;
	};
    return uRet;
}

STDMETHODIMP CForceFeedbackImpl::QueryInterface ( REFIID riid, void** ppv )
{
HRESULT hrRet = S_OK;

    TRACE(">>> ForceFeedbackSvr: In CForceFeedbackImpl::QueryInterface()\n" );

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
    else if ( ::ATL::InlineIsEqualGUID ( riid, __uuidof(IForceFeedback) ))
        {
        *ppv = (IForceFeedback*) this;
        TRACE(">>> ForceFeedbackSvr: Client QIed for IForceFeedback\n" );
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
// IForceFeedback methods

STDMETHODIMP CForceFeedbackImpl::DeviceID(DWORD  dwDIVer, DWORD  dwExternalID,
										  DWORD  fBegin, DWORD  dwInternalId,
										  LPVOID  lpReserved)
{
    HRESULT hRes = S_OK;
    LPDIRECTINPUTDEVICE2    pdiDevice2 = NULL;
    LPDIRECTINPUTJOYCONFIG  pdiJoyCfg = NULL;
 	LPDIRECTINPUT pdi = NULL;


	TRACE(">>> ForceFeedbackSvr: Llamado a Device ID.\n");
    TRACE("                      - dwDIVer:      %i\n", dwDIVer);
	TRACE("	                     - dwExternalID: %i\n",dwExternalID);
	TRACE("	                     - fBegin:       %i\n", fBegin);
	TRACE("	                     - dwInteralID:  %i\n",dwInternalId); 
	TRACE("	                     - lpReserved:   %p\n", lpReserved);

	working = 1;
	if(signaled)
		CloseHandle(hDriver);
	hDriver = INVALID_HANDLE_VALUE;


    hRes = DirectInputCreate(g_hinstThisDll, 0x700, &pdi, NULL);
    if(FAILED(hRes))
    {
        TRACE(TEXT(">>> ForceFeedbackSvr: DirectInputCreate() failed!\n"));
        working = 0;
        return E_FAIL;
    }
    hRes = pdi->QueryInterface(IID_IDirectInputJoyConfig, (LPVOID*)&pdiJoyCfg);
    if (FAILED(hRes))
    {
        TRACE(TEXT(">>> ForceFeedbackSvr: Unable to create joyconfig\n"));
        goto ending;
    }
    hRes = DIUtilCreateDevice2FromJoyConfig(dwExternalID, NULL, pdi, pdiJoyCfg, &pdiDevice2);
    if (FAILED(hRes))
    {
        TRACE(TEXT(">>> ForceFeedbackSvr: Unable to create DirectInput Device\n"));
        goto ending;
    }
    hRes = DIUtilGetHIDDevice(pdiDevice2, &hDriver);
    if (FAILED(hRes))
    {
        TRACE(TEXT(">>> ForceFeedbackSvr: Failed to open HID Device\n"));
        goto ending;
    }
	if(hDriver == INVALID_HANDLE_VALUE) 
	{
		TRACE(">>> ForceFeedbackSvr: NO PUDE ABRIR EL ARCHIVO\n");
        goto ending;
	};
    hRes = S_OK;
	signaled = TRUE;
	Id = dwExternalID;
	TRACE(">>> ForceFeedbackSvr: Abri el archivo con handle %i\n", hDriver);

    if(pdiDevice2 != NULL)
        pdiDevice2->Release();
    if(pdiJoyCfg != NULL)
        pdiJoyCfg->Release();
    if(pdi != NULL)
        pdi->Release();
ending:
	working = 0;
	return hRes;
}
    
STDMETHODIMP CForceFeedbackImpl::GetVersions(LPDIDRIVERVERSIONS  pvers)
{
	HRESULT hRet = S_OK;
	TRACE(">>> ForceFeedbackSvr: Llamado a GetVersions.\n");
	pvers->dwFirmwareRevision = 0x0211;
	pvers->dwHardwareRevision = 0x0100;
	pvers->dwFFDriverVersion = 0x0211;


	return hRet;
}

STDMETHODIMP CForceFeedbackImpl::Escape(DWORD  dwID, DWORD  dwEffect,
										LPDIEFFESCAPE  pesc)
{
	HRESULT hRet = E_NOTIMPL;
	TRACE(">>> ForceFeedbackSvr: Llamado a Escape.\n");
	TRACE("                      Función no soportada.\n");
	return hRet;
}
    
STDMETHODIMP CForceFeedbackImpl::SetGain(DWORD  dwID, DWORD  dwGain)

{
	HRESULT hRet = S_OK;
	if(Id != dwID) DeviceID(0, dwID, 0,0,0);
	working = 1;
	TRACE(">>> ForceFeedbackSvr: Llamado a SetGain.\n");
    TRACE("                      - dwID:      %i\n", dwID);
    TRACE("                      - dwGain:    %i\n", dwGain);
	Ganancia = dwGain;
	working = 0;
	return hRet;
}
 
STDMETHODIMP CForceFeedbackImpl::SendForceFeedbackCommand(DWORD  dwID,
														  DWORD  dwCommand)
{
	int i;
	HRESULT hRet = S_OK;
	TRACE(">>> ForceFeedbackSvr: Llamado a SendForceFeedbackCommand.\n");
    TRACE("                      - dwID:      %i\n", dwID);
	if(Id != dwID) DeviceID(0, dwID, 0,0,0);
	working = 1;
	switch(dwCommand)
	{
		case DISFFC_RESET:
		    TRACE("                      - Comando: RESET\n");
			for(i=0; i<10; i++)	
			{
				Efectos[i].estado = EFECTO_VACIO;
				if(Efectos[i].custom)
				{
					VirtualUnlock(Efectos[i].custom, sizeof(LPLONG) * Efectos[i].size);
					VirtualFree(Efectos[i].custom, 0, MEM_RELEASE);
				}
				Efectos[i].elapsed = 0;
			};
			pausa = 0;
			noactuators = 0;
			break;
		case DISFFC_STOPALL:
		    TRACE("                      - Comando: PARAR TODOS\n");
			for(i=0; i<10; i++)	
				if(Efectos[i].estado == DIEGES_PLAYING) 
					Efectos[i].estado = 0;
			break;
		case DISFFC_PAUSE:
		    TRACE("                      - Comando: PAUSA\n");
			pausa = 1;
			break;
		case DISFFC_CONTINUE:
		    TRACE("                      - Comando: CONTINUAR\n");
			pausa = 0;
			break;
		case DISFFC_SETACTUATORSON:
		    TRACE("                      - Comando: SETACTUATORSON\n");
			noactuators = 0;
			break;
		case DISFFC_SETACTUATORSOFF:
		    TRACE("                      - Comando: SETACTUATORSOFF\n");
			noactuators = 1;
			break;

		default:
			hRet = E_NOTIMPL;
			break;
	};
	SetActuatorsState();
	working = 0;
	return hRet;
}
    
STDMETHODIMP CForceFeedbackImpl::GetForceFeedbackState(DWORD dwID,
													   LPDIDEVICESTATE  pds)
{
	HRESULT hRet = S_OK;
	int i;
	int empty = 0;
    int stop = 0;
	TRACE(">>> ForceFeedbackSvr: Llamado a GetForceFeedbackState.\n");
	if(Id != dwID) DeviceID(0, dwID, 0,0,0);
	working = 1;

	for(i=0; i<10; i++)	
	{
		switch(Efectos[i].estado)
		{
		   case EFECTO_VACIO:
				empty++;
		   case 0:
			    stop++;
		}
	}
	pds->dwState = 0;
	if(empty = 10)
	  pds->dwState |= DIGFFS_EMPTY;
	if(stop = 10)
	  pds->dwState |= DIGFFS_STOPPED;
    if(pausa)
	  pds->dwState |= DIGFFS_PAUSED;
	if(noactuators)
	  pds->dwState |= DIGFFS_ACTUATORSOFF;
    else
	  pds->dwState |= DIGFFS_ACTUATORSON;

	pds->dwLoad = empty * 10;

	working = 0;
	return hRet;
}

STDMETHODIMP CForceFeedbackImpl::DownloadEffect(DWORD  dwID, DWORD  dwEffectID,
												LPDWORD  pdwEffect, 
												LPCDIEFFECT  peff,
												DWORD  dwFlags)
{
	HRESULT hRet = S_OK;
	long nro, nro2, nro3, i;
	DWORD localgain; DWORD laststatus;
	DOUBLE lastphase;
	int found = 0;
	TRACE(">>> ForceFeedbackSvr: Llamado a DownloadEffect.\n");
    TRACE("                      - dwID:      %x\n", dwID);
    TRACE("                      - dwEffectID:%x\n", dwEffectID);
    TRACE("                      - *pdwEffect:%x\n", *pdwEffect);
    TRACE("                      - dwFlags   :%x\n", dwFlags);
	if(((dwFlags | DIEP_NODOWNLOAD) == dwFlags))
		return S_OK;
	if(Id != dwID) DeviceID(0, dwID, 0,0,0);
	working = 1;
	if (((*pdwEffect) == 0) | ((*pdwEffect) > 10))
	{
		for((*pdwEffect)=1; (*pdwEffect)<11; (*pdwEffect)++)
		{
			if (Efectos[(*pdwEffect) - 1].estado == EFECTO_VACIO)
			{
				found = 1;
				break;
			};
		};
		if (!found) {working = 0; return DIERR_DEVICEFULL;}; // Memoria llena
		Efectos[(*pdwEffect) - 1].estado = 0;
	};
    TRACE("                      - Id asignado:%i\n", *pdwEffect);
	lastphase = Efectos[(*pdwEffect) - 1].phase;
	laststatus = Efectos[(*pdwEffect) - 1].estado;
	ZeroMemory(&(Efectos[(*pdwEffect) - 1]), sizeof(EFECTO));
	Efectos[(*pdwEffect) - 1].estado = laststatus;
	Efectos[(*pdwEffect) - 1].phase = lastphase;
	localgain = Ganancia;
	if(peff->dwGain)
		localgain = (peff->dwGain * localgain) / 10000;
	if(peff->lpEnvelope != NULL)
	{
		Efectos[(*pdwEffect) - 1].isenveloped = 1;
		nro2 = ((DIENVELOPE*)peff->lpEnvelope)->dwAttackLevel; 
		nro2 = (localgain * nro2) / 10000;
		nro3 = ((DIENVELOPE*)peff->lpEnvelope)->dwFadeLevel; 				
		nro3 = (localgain * nro3) / 10000;
		Efectos[(*pdwEffect) - 1].attackduration = ((DIENVELOPE*)peff->lpEnvelope)->dwAttackTime / 1000;
		Efectos[(*pdwEffect) - 1].fadeduration = ((DIENVELOPE*)peff->lpEnvelope)->dwFadeTime/ 1000;
		Efectos[(*pdwEffect) - 1].attack = nro2;
		Efectos[(*pdwEffect) - 1].fade = nro3;
	};
	if(peff->dwTriggerButton != DIEB_NOTRIGGER)
		Efectos[(*pdwEffect) - 1].trigger = DIDFT_GETINSTANCE(peff->dwTriggerButton);
	Efectos[(*pdwEffect) - 1].duration = (peff->dwDuration != INFINITE)?(peff->dwDuration / 1000):INFINITE;
    TRACE("                      - dwDuration   :%d\n", peff->dwDuration);
	if ((Efectos[(*pdwEffect) - 1].duration < 1000) & (Efectos[(*pdwEffect) - 1].duration != INFINITE))
		Efectos[(*pdwEffect) - 1].duration = 1000;
	TRACE("                      - dwDuration Calc:%d\n", Efectos[(*pdwEffect) - 1].duration);
	Efectos[(*pdwEffect) - 1].tipo = dwEffectID;
	switch(dwEffectID)
	{
		case 1:
			TRACE(">>> ForceFeedbackSvr: Haciendo un efecto CONSTANT FORCE.\n");
			nro = ((DICONSTANTFORCE*)peff->lpvTypeSpecificParams)->lMagnitude; 
			nro = (localgain * nro) / 10000;
			Efectos[(*pdwEffect) - 1].gmin = nro;
			break;

		case 2:
			TRACE(">>> ForceFeedbackSvr: Haciendo un efecto RAMP FORCE\n");
			
			nro = ((DIRAMPFORCE*)peff->lpvTypeSpecificParams)->lStart; 
			nro2 = ((DIRAMPFORCE*)peff->lpvTypeSpecificParams)->lEnd; 			
			nro = (localgain * nro) / 10000;
			nro2 = (localgain * nro2) / 10000;
			if((nro >= 0) && (nro2 >= 0))
			{
				if(nro > nro2)
					nro2 = -10000 + nro2;
				else
					nro = -10000 + nro;
			};
			Efectos[(*pdwEffect) - 1].gmin = nro;
			Efectos[(*pdwEffect) - 1].gmax = nro2;
			break;
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
			nro = ((DIPERIODIC*)peff->lpvTypeSpecificParams)->lOffset; 			
			nro2 = ((DIPERIODIC*)peff->lpvTypeSpecificParams)->dwMagnitude; 
			nro = (localgain * nro) / 10000;
			nro2 = (localgain * nro2) / 10000;
			Efectos[(*pdwEffect) - 1].gcenter = nro;
			Efectos[(*pdwEffect) - 1].gmin = nro2;
			Efectos[(*pdwEffect) - 1].sample = 
				((DIPERIODIC*)peff->lpvTypeSpecificParams)->dwPeriod / 1000; 		
			if(Efectos[(*pdwEffect) - 1].sample == 0)
					Efectos[(*pdwEffect) - 1].sample = peff->dwSamplePeriod / 1000;
			if(Efectos[(*pdwEffect) - 1].sample < 2000)
					Efectos[(*pdwEffect) - 1].sample = 2000;
			if(found) // si es un efecto nuevo
			{
				Efectos[(*pdwEffect) - 1].phase =
					((DIPERIODIC*)peff->lpvTypeSpecificParams)->dwPhase; 		
			};
			break;
		case 8:
		case 9:
		case 10:
	 	case 11:
			TRACE(">>> ForceFeedbackSvr: Haciendo un efecto SPRING FORCE.\n");
			Efectos[(*pdwEffect) - 1].chico = 0;
			Efectos[(*pdwEffect) - 1].grande = 0;
			Efectos[(*pdwEffect) - 1].ejes = peff->cAxes;
			Efectos[(*pdwEffect) - 1].primereje =  DIDFT_GETINSTANCE(peff->rgdwAxes[0]);

			nro = 10000 + (((DICONDITION*)peff->lpvTypeSpecificParams)->lPositiveCoefficient); 
			nro = (localgain * nro2) / 10000;
			nro = (nro * 255) / 20000;
			if(nro < 2)
				nro = 2;

			nro2 = (((DICONDITION*)peff->lpvTypeSpecificParams)->dwPositiveSaturation); 
			nro2 = (localgain * nro2) / 10000;
			nro2 = (nro * 255) / 10000;
			if(nro2 == 0)
				nro2 = 255;

			Efectos[(*pdwEffect) - 1].gmin = nro;
			
			Efectos[(*pdwEffect) - 1].gmax = nro2;
			
			nro = 10000 + (((DICONDITION*)peff->lpvTypeSpecificParams)->lOffset); 
			nro = (nro * 255) / 20000;

			Efectos[(*pdwEffect) - 1].centro = nro;

			nro = (((DICONDITION*)peff->lpvTypeSpecificParams)->lDeadBand); 
			nro = (nro * 255) / 10000;			
			Efectos[(*pdwEffect) - 1].muerta = nro;

			break;
		case 12:
			nro = (((DICUSTOMFORCE*)peff->lpvTypeSpecificParams)->cSamples);
			nro2 = (((DICUSTOMFORCE*)peff->lpvTypeSpecificParams)->cChannels);			
			nro = nro / nro2;
			TRACE("Tamaño: %d bytes\n", nro);
			Efectos[(*pdwEffect) - 1].custom = (LPLONG) VirtualAlloc(0, nro*sizeof(LONG), MEM_COMMIT, PAGE_READWRITE);
			if(Efectos[(*pdwEffect) - 1].custom)
			{
				VirtualLock(Efectos[(*pdwEffect) - 1].custom, nro*sizeof(LONG));
				for(i=0; i<nro;i++)
				{
					Efectos[(*pdwEffect) - 1].custom[i] =
					    ((DICUSTOMFORCE*)peff->lpvTypeSpecificParams)->rglForceData[i*nro2];
				};
			} else {
				Efectos[(*pdwEffect) - 1].estado = EFECTO_VACIO;
				working = 0; 
				return DIERR_DEVICEFULL;
			};
			Efectos[(*pdwEffect) - 1].sample = 
				((DICUSTOMFORCE*)peff->lpvTypeSpecificParams)->dwSamplePeriod / 1000;
			if(Efectos[(*pdwEffect) - 1].sample == 0)
					Efectos[(*pdwEffect) - 1].sample = peff->dwSamplePeriod / 1000;
			if(Efectos[(*pdwEffect) - 1].sample == 0)
					Efectos[(*pdwEffect) - 1].sample = 5000;
			Efectos[(*pdwEffect) - 1].size = nro;
			break; 
	};
	CalculateForces((*pdwEffect) - 1);			
	if(((dwFlags | DIEP_START) == dwFlags))
	{
		Efectos[(*pdwEffect) - 1].estado = DIEGES_PLAYING;
		Efectos[(*pdwEffect) - 1].started = GetTickCount();
		Efectos[(*pdwEffect) - 1].veces = 1;
	};
	working = 0;
	Actualize();
	return hRet;    
}

STDMETHODIMP CForceFeedbackImpl::DestroyEffect(DWORD  dwID, DWORD  dwEffect)
{
	HRESULT hRet = S_OK;
	TRACE(">>> ForceFeedbackSvr: Llamado a DestroyEffect.\n");
    TRACE("                      - dwEffect:%x\n", dwEffect);
	if(Id != dwID) DeviceID(0, dwID, 0,0,0);
	working = 1;
	Efectos[dwEffect-1].estado = EFECTO_VACIO;
	if(Efectos[dwEffect-1].custom)
	{
		VirtualUnlock(Efectos[dwEffect-1].custom, sizeof(LPLONG) * Efectos[dwEffect-1].size);
		VirtualFree(Efectos[dwEffect-1].custom, 0, MEM_RELEASE);
	}; 
	Efectos[dwEffect-1].elapsed = 0;
	SetActuatorsState();
	working = 0;
	return hRet;
}

STDMETHODIMP CForceFeedbackImpl::StartEffect(DWORD  dwID, DWORD  dwEffect,
											 DWORD  dwMode, DWORD  dwCount)
{
	int i;
	HRESULT hRet = S_OK;
	TRACE(">>> ForceFeedbackSvr: Llamado a StartEffect.\n");
    TRACE("                      - dwID:      %i\n", dwID);
    TRACE("                      - dwEffect:  %i\n", dwEffect);
	TRACE("                      - dwMode:    %i\n", dwMode);
	TRACE("                      - dwCount:    %i\n", dwCount);
	if(Id != dwID) DeviceID(0, dwID, 0,0,0);
	working = 1;
	if(dwMode == DIES_SOLO)
	{
		for(i=0; i<10; i++)	
		{
			if(Efectos[dwEffect-1].estado == DIEGES_PLAYING)
			{
				Efectos[i].estado = 0;
			};
		};
	};
	Efectos[dwEffect-1].elapsed = 0;
	Efectos[dwEffect-1].started = GetTickCount();
	Efectos[dwEffect-1].estado = DIEGES_PLAYING;
	Efectos[dwEffect-1].veces = dwCount;
	working = 0;
	Actualize();
	return hRet;	
}

STDMETHODIMP CForceFeedbackImpl::StopEffect(DWORD  dwID, DWORD  dwEffect)
{
	HRESULT hRet = S_OK;
	TRACE(">>> ForceFeedbackSvr: Llamado a StopEffect.\n");
    TRACE("                      -dwID:      %x\n", dwID);
	TRACE(">>>                   -dwEffectID: %x\n", dwEffect);
	if(Id != dwID) DeviceID(0, dwID, 0,0,0);
	working = 1;
	Efectos[dwEffect-1].estado = 0;
	SetActuatorsState();
	working = 0;
	return hRet;
}
    
STDMETHODIMP CForceFeedbackImpl::GetEffectStatus(DWORD  dwID, DWORD  dwEffect,
												 LPDWORD  pdwStatus)
{
	HRESULT hRet = S_OK;
	TRACE(">>> ForceFeedbackSvr: Llamado a GetEffectStatus.\n");
	if(Id != dwID) DeviceID(0, dwID, 0,0,0);
	working = 1;
	*pdwStatus = 0;
	if(Efectos[dwEffect-1].estado == DIEGES_PLAYING)
		*pdwStatus = DIEGES_PLAYING;
	working = 0;
	return hRet;
}

// Metodos Propios
void CForceFeedbackImpl::SetActuatorsState()
{
	DWORD a;
	int i;
	UCHAR buffer[3] = {0,0,0};
	if ((!pausa) & (!noactuators))
	{
		for(i=0; i<10; i++)
		{
			if (Efectos[i].estado == DIEGES_PLAYING)
			{
				buffer[1] |= Efectos[i].chico;
				if(Efectos[i].grande > buffer[2])
				{
					buffer[2] = Efectos[i].grande;
				}
			};
		};
	};
	if((buffer[2] < 25) & (buffer[2] > 10)) buffer[2] = 25;
	WriteFile(hDriver, (VOID*) buffer, 3, &a, NULL);
#ifdef 	_DEBUG
	if((buffer[1] != 0) | (buffer[2] != 0))
	{
	TRACE(">>> ForceFeedbackSvr: Seteando los motores:\n");
	TRACE("                      - Chico: %i\n", buffer[1]);
	TRACE("                      - Grande: %i\n", buffer[2]);
	};
#endif
 }
VOID CForceFeedbackImpl::MatarTimer()
{
	KillTimer(NULL, hTimer);
	hTimer = 0;
};
VOID CForceFeedbackImpl::TimerProc(
  HWND hwnd,     // handle of window for timer messages
  UINT uMsg,     // WM_TIMER message
  UINT idEvent,  // timer identifier
  DWORD dwTime   // current system time
)
{
	Actualize();
};
VOID CForceFeedbackImpl::Actualize()
{
	int i, play = 0;
	int dif1, dif2;
	static UCHAR lastbuf[8] = {0,0,0,0,0,0,0,0};
	UCHAR buffer[8] = {0,0,0,0,0,0,0,0};
	UCHAR a = 0, b = 0, c1, c2;
	DWORD botones;
	DWORD dwTime;
	DOUBLE localsample;
	dwTime = GetTickCount();
	for(i=0; i<10; i++)
	{
			if (Efectos[i].estado == DIEGES_PLAYING) 
			{
				play = 1;
				break;
			}
	}
	if(busy | pausa | working| (!signaled) | (!play))
	{
		return;
	}
	busy = 1;
	HidD_GetFeature(hDriver, buffer, 8);
	botones = buffer[1];
	botones = (botones << 8) | buffer[2];

	for(i=0; i<10; i++)
	{
			if (Efectos[i].estado == DIEGES_PLAYING) 
			{
				Efectos[i].elapsed = dwTime - Efectos[i].started;
				if((Efectos[i].duration != INFINITE))
				{				
					if (Efectos[i].elapsed >= (Efectos[i].duration + Efectos[i].attackduration + Efectos[i].fadeduration))
					{
						if(Efectos[i].veces == INFINITE)
						{
							TRACE("se vencio el efecto %d. Se reinicia\n", i);
							Efectos[i].elapsed = 0;
							Efectos[i].started = dwTime;
						} else {
							if((--Efectos[i].veces) == 0)
							{
								TRACE("se vencio el efecto %d\n", i);
								Efectos[i].estado = 0;
							} else {
								TRACE("se vencio el efecto %d, quedan %d repeticiones\n", i, Efectos[i].veces);
								Efectos[i].elapsed = 0;
								Efectos[i].started = dwTime;
							}
						};

					}					
				};
				if((Efectos[i].tipo > 7) & (Efectos[i].tipo < 12))
				{					
					Efectos[i].chico = 0;
					Efectos[i].grande = 0;
					switch(Efectos[i].tipo)
					{
							case 8:
							case 9: // velocity and position are almost the same
								a = (abs(Efectos[i].centro - buffer[5]) * 255) / (255 - Efectos[i].centro);
								b = (abs(Efectos[i].centro - buffer[4]) * 255) / (255 - Efectos[i].centro);
								TRACE("a: %d b: %d", a, b);
								break;
							case 10:
								a = Efectos[i].gmin * abs(buffer[5] - lastbuf[5]);									
								b = Efectos[i].gmin * abs(buffer[4] - lastbuf[4]);									
								break;
							case 11:
            					dif1 = (Efectos[i].centro + Efectos[i].muerta);
			            		dif2 = (Efectos[i].centro - Efectos[i].muerta);
            					c1 = ((buffer[5]>= dif1)|(buffer[5]<= dif2))?1:0;						
			            		c2 = ((buffer[4]>= dif1)|(buffer[4]<=dif2))?1:0;
								if(Efectos[i].ejes == 1)
								{
									if(Efectos[i].primereje == 2)
										Efectos[i].chico = c1;
									else
										Efectos[i].chico = c2;								
								} else {						
									Efectos[i].chico = c1 | c2;
								}
								a = 0;
								b = 0;
								break;
					}
					if(Efectos[i].ejes == 1)
					{
						if(Efectos[i].primereje == 2)
						{
  							if(c1)
								Efectos[i].grande = a;
						}
						else
						{
							if(c2)
								Efectos[i].grande = b;								
							
						}
					} else {						
						if(c1|c2)
						{
						    if(a>b)
								Efectos[i].grande = a;
							else
								Efectos[i].grande = b;
						}
					}
					TRACE("%d", Efectos[i].gmax);
					if(Efectos[i].grande > Efectos[i].gmax)
						Efectos[i].grande = (UCHAR)Efectos[i].gmax;
				} else {
					if((Efectos[i].tipo > 2) & (Efectos[i].tipo < 8))
					{
						localsample = Efectos[i].sample;
						Efectos[i].phase = (36 / localsample); 
						localsample = Efectos[i].elapsed % Efectos[i].sample;
						Efectos[i].phase *= localsample;
						TRACE("Fase: %f\n",  Efectos[i].phase);
					};
					CalculateForces(i);			
				};
				if(Efectos[i].trigger != NULL)
				{
					if(!((botones >> (Efectos[i].trigger - 1)) & 1))
					{
						Efectos[i].chico = 0;
						Efectos[i].grande = 0;
					}
				};
			};
	};
	if(dwTime >= (lastcall + 500))
	{
		memcpy(lastbuf, buffer, 15);
		lastcall = dwTime;
	}
	SetActuatorsState();
	busy = 0;
	return;
};
void CForceFeedbackImpl::CalculateForces(int i)
{
	long l, a;
	double b;
				ApplyEnvelope(i);
				switch(Efectos[i].tipo)
				{
					case 1: // CONSTANT
						l = (labs(Efectos[i].actualmin) * 2) - 10000;
						break;
					case 2: // RAMP
						l = Efectos[i].actualmin +
							(((Efectos[i].actualmax - Efectos[i].actualmin) / 
							 (Efectos[i].duration + Efectos[i].attackduration + Efectos[i].fadeduration))
							 * Efectos[i].elapsed);
						break;
					case 3:
						l = -10000;
						Efectos[i].chico = OndaCuadrada(Efectos[i].phase);
						break;
					case 4:
						l = (LONG)(Efectos[i].gcenter + (Efectos[i].actualmin * sin(Efectos[i].phase * (3.1416 / 180))));
						break;
					case 5:
						l = (LONG)(Efectos[i].gcenter + (Efectos[i].actualmin * OndaTriangular(Efectos[i].phase)));
						break;
					case 6:
						l = (LONG)(Efectos[i].gcenter + (Efectos[i].actualmin * OndaSawtooth(Efectos[i].phase, 1)));
						break;
					case 7:
						l = (LONG)(Efectos[i].gcenter + (Efectos[i].actualmin * OndaSawtooth(Efectos[i].phase, 0)));
						break;
					case 12:
						b = ((double)Efectos[i].size) / ((double)Efectos[i].sample);
						b *= (double)(Efectos[i].elapsed % Efectos[i].sample);						
						a = ((long) b);
						l = Efectos[i].custom[a];
						break;
				};
				l = 10000 + l;
				TRACE("l: %d", l);
				l = (l * 255) / 20000;
				if(l>255) l = 255;
				Efectos[i].grande = (UCHAR) l;
};
void CForceFeedbackImpl::ApplyEnvelope(int i)
{
	if(Efectos[i].isenveloped)
	{
		if(Efectos[i].elapsed < Efectos[i].attackduration)
		{
			Efectos[i].actualmin = Efectos[i].gcenter + Efectos[i].attack +
									(((Efectos[i].gmin - Efectos[i].attack) / Efectos[i].attackduration) * Efectos[i].elapsed);
			Efectos[i].actualmax = Efectos[i].gcenter + Efectos[i].attack +
									(((Efectos[i].gmax - Efectos[i].attack) / Efectos[i].attackduration) * Efectos[i].elapsed);
		} else {
			if((Efectos[i].duration != INFINITE) & (Efectos[i].fadeduration != 0))
			{
				if(Efectos[i].attackduration > (Efectos[i].attackduration + Efectos[i].duration))
				{
					Efectos[i].actualmin = Efectos[i].gmin +
											(((Efectos[i].gcenter + Efectos[i].fade - Efectos[i].gmin) / Efectos[i].fadeduration) * (Efectos[i].elapsed-Efectos[i].attackduration-Efectos[i].duration));
					Efectos[i].actualmax = Efectos[i].gmax +
											(((Efectos[i].gcenter + Efectos[i].fade - Efectos[i].gmax) / Efectos[i].fadeduration) * (Efectos[i].elapsed-Efectos[i].attackduration-Efectos[i].duration));
				} else {
					 goto cont;
				};
			} else {
				goto cont;
			};
		};
	} else {
		cont:
			Efectos[i].actualmin = Efectos[i].gmin;
			Efectos[i].actualmax = Efectos[i].gmax;
	};
};
UCHAR CForceFeedbackImpl::OndaCuadrada(double base)
{
	if(base < 18)
		return 1;
	else
		return 0;
};

double CForceFeedbackImpl::OndaTriangular(double base)
{
	double i;
	double Onda = 0;
	double radbase;
	double signo = 1;
	radbase = base * (3.1416 / 18);
	for(i=1; i<=20; i+=2)
	{
       Onda += signo*(sin(i*radbase) / (i*i));
	   signo *= -1;
	};
	Onda *= 8 / (3.1416*3.1416); 
	return Onda;
};

double CForceFeedbackImpl::OndaSawtooth(double base, int direccion)
{
	double i;
	double Onda = 0;
	double radbase;
	double signo = 1;
	if(!direccion)
		signo = -1;
	radbase = base * (3.1416 / 18);
	for(i=1; i<=50; i++)
	{
       Onda += signo*(sin(i*radbase) / i);
	   signo *= -1;
	};
	Onda *= 2 / (3.1416);
	return Onda;
};

//===========================================================================
// DIUtilCreateDevice2FromJoyConfig
//
// Helper function to create a DirectInputDevice2 object from a 
//  DirectInputJoyConfig object.
//
// Parameters:
//  short                   nJoystickId     - joystick id for creation
//  HWND                    hWnd            - window handle
//  LPDIRECTINPUT           pdi             - ptr to base DInput object
//  LPDIRECTINPUTJOYCONFIG  pdiJoyCfg       - ptr to joyconfig object
//  LPDIRECTINPUTDEVICE     *ppdiDevice2    - ptr to device object ptr
//
// Returns: HRESULT
//
//===========================================================================
HRESULT DIUtilCreateDevice2FromJoyConfig(short nJoystickId, HWND hWnd,
                            LPDIRECTINPUT pdi,
                            LPDIRECTINPUTJOYCONFIG pdiJoyCfg,
                            LPDIRECTINPUTDEVICE2 *ppdiDevice2)
{
    HRESULT                 hRes        = E_NOTIMPL;
    LPDIRECTINPUTDEVICE     pdiDevTemp  = NULL;
    DIJOYCONFIG             dijc;
    
    // validate pointers
    if( (IsBadReadPtr((void*)pdi, sizeof(IDirectInput))) ||
        (IsBadWritePtr((void*)pdi, sizeof(IDirectInput))) )
    {
        TRACE(TEXT("DIUtilCreateDeviceFromJoyConfig() - invalid pdi\n"));
        return E_POINTER;
    }
    if( (IsBadReadPtr((void*)pdiJoyCfg, sizeof(IDirectInputJoyConfig))) ||
        (IsBadWritePtr((void*)pdiJoyCfg, sizeof(IDirectInputJoyConfig))) )
    {
        TRACE(TEXT("DIUtilCreateDeviceFromJoyConfig() - invalid pdiJoyCfg\n"));
        return E_POINTER;
    }
    if( (IsBadReadPtr((void*)ppdiDevice2, sizeof(LPDIRECTINPUTDEVICE2))) ||
        (IsBadWritePtr((void*)ppdiDevice2, sizeof(LPDIRECTINPUTDEVICE2))) )
    {
        TRACE(TEXT("DIUtilCreateDeviceFromJoyConfig() - invalid ppdiDevice2\n"));
        return E_POINTER;
    }

    // get the instance GUID for device configured as nJoystickId
    // 
    // GetConfig will provide this information
    dijc.dwSize = sizeof(DIJOYCONFIG);
    hRes = pdiJoyCfg->GetConfig(nJoystickId, &dijc, DIJC_GUIDINSTANCE);
    if(FAILED(hRes))
    {
        TRACE(TEXT("DIUtilCreateDeviceFromJoyConfig() - GetConfig() failed\n"));
        return hRes;
    }

    // create temporary device object
    //
    // use the instance GUID returned by GetConfig()
    hRes = pdi->CreateDevice(dijc.guidInstance, &pdiDevTemp, NULL);
    if(FAILED(hRes))
    {
        TRACE(TEXT("DIUtilCreateDeviceFromJoyConfig() - CreateDevice() failed\n"));
        return hRes;
    }

    // query for a device2 object
    hRes = pdiDevTemp->QueryInterface(IID_IDirectInputDevice2, (LPVOID*)ppdiDevice2);
    
    // release the temporary object
    pdiDevTemp->Release();

    if(FAILED(hRes))
    {
        TRACE(TEXT("DIUtilCreateDeviceFromJoyConfig() - QueryInterface(IDirectInputDevice2) failed\n"));
        return hRes;
    }

    // set the desired data format
    //
    // we want to be a joystick
    hRes = (*ppdiDevice2)->SetDataFormat(&c_dfDIJoystick);
    if(FAILED(hRes))
    {
        TRACE(TEXT("DIUtilCreateDeviceFromJoyConfig() - SetDataFormat(Joystick) failed\n"));
        return hRes;
    }
    return hRes;

} //*** end DIUtilCreateDevice2FromJoyConfig()
                            
//===========================================================================
// DIUtilGetHIDDevice
//
// Helper function to get the file handle of the related HID device
//  LPDIRECTINPUTDEVICE2    pdiDevice2  - ptr to device object
//  PHANDLE                 hDriver     - ptr to recieve device handle
// Returns: HRESULT
//
//===========================================================================
HRESULT DIUtilGetHIDDevice(LPDIRECTINPUTDEVICE2 pdiDevice2, HANDLE *hDriver)
{                         
    HRESULT hRes    = E_NOTIMPL;
    DIPROPGUIDANDPATH dipgp;

    // validate pointers
    if( (IsBadReadPtr((void*)pdiDevice2, sizeof(IDirectInputDevice2))) ||
        (IsBadWritePtr((void*)pdiDevice2, sizeof(IDirectInputDevice2))) )
    {
        TRACE(TEXT("DIUtilGetHIDDevice() - invalid pdiDevice2\n"));
        return E_POINTER;
    }

    // Initialize structure
    dipgp.diph.dwSize = sizeof(DIPROPGUIDANDPATH);
    dipgp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dipgp.diph.dwObj = 0;
    dipgp.diph.dwHow = DIPH_DEVICE;

    // Get the path
    hRes = pdiDevice2->GetProperty(DIPROP_GUIDANDPATH, &dipgp.diph);
    if(FAILED(hRes))
    {
        TRACE(TEXT("DIUtilGetHIDDevice() - GetProperty() failed\n"));
        return hRes;
    }

    *hDriver = CreateFileW(dipgp.wszPath,
                          GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(!hDriver)
    {
        TRACE(TEXT("DIUtilGetHIDDevice() - Can't open device\n"));
        return E_FAIL;
    }
    return hRes;

} //*** end DIUtilGetHIDDevice()
                            
