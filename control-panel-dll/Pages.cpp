//===========================================================================
//
// CPLSVR1 property pages implementation.
//
// Functions:
//  CPLSVR1_Page1_DlgProc()
//  CPLSVR1_Page2_DlgProc()
//  CPLSVR1_Page3_DlgProc()
//  InitDInput()
//  EnumDeviceObjectsProc()
//  EnumEffectsProc()
//  DisplayEffectList()
//  DisplayJoystickState()
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

#include "cplsvr1.h"
#include "pages.h"
#include "dicputil.h"
#include "resrc1.h"
#include <prsht.h>
HBRUSH g_hbrBackground;
LPDIRECTINPUTEFFECT  g_lpdiEffect = NULL;  // global effect object
LONG fza[4] = {DI_FFNOMINALMAX,DI_FFNOMINALMAX,0, 0};
TCHAR  lptszBuf[64];
int isanalog = 0;
void ActualizeMaps(HWND hWnd, PUCHAR map);

//===========================================================================
// CPLSVR1_Page1_DlgProc
//
// Callback proceedure CPLSVR1 property page #1 (Main).
//
// Parameters:
//  HWND    hWnd    - handle to dialog window
//  UINT    uMsg    - dialog message
//  WPARAM  wParam  - message specific data
//  LPARAM  lParam  - message specific data
//
// Returns: BOOL
//
//===========================================================================
INT_PTR CALLBACK CPLSVR1_Page1_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static CDIGameCntrlPropSheet_X *pdiCpl;
   static LPDIRECTINPUTDEVICE2    pdiDevice2;
   static LPDIJOYSTATE  pdijs;

   switch(uMsg)
   {
      case WM_INITDIALOG:
         {
         // perform any required initialization... 

         // get ptr to our object
         pdiCpl = (CDIGameCntrlPropSheet_X*)((PROPSHEETPAGE *)lParam)->lParam;
         
         // Save our pointer so we can access it later
         SetWindowLongPtr(hWnd, DWLP_USER, (LPARAM)pdiCpl);

         // initialize DirectInput
         if (FAILED(InitDInput(GetParent(hWnd), pdiCpl)))
         {
#ifdef _DEBUG
            OutputDebugString(TEXT("CPLSvr1 Page 4 - InitDInput failed!\n"));
#endif //_DEBUG
            return TRUE;
         }
		   g_hbrBackground = CreateSolidBrush(RGB(51, 51, 51));
		   pdijs = new (DIJOYSTATE);
			assert (pdijs);
          for(int i=0;i<32;i++)
          {
          TCHAR text[3];
          wsprintf(text, TEXT("%d"), i+1);
          SetDlgItemText(hWnd,IDC_TESTJOYBTNICON+i,text);
          }
          LoadString(ghInst, IDS_CALIBRATE, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_CALIBRATE, lptszBuf);
          SendDlgItemMessage(hWnd, IDC_CALIBRATE, WM_USER+1, 0,0);
          LoadString(ghInst, IDS_ANALOGPRESURE, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_ANALOGPRESURE, lptszBuf);

          UCHAR data[8];
          HANDLE hHID;
          pdiCpl->GetHIDHandle(&hHID);
          ZeroMemory(data, 8);
          HidD_GetFeature(hHID, data, 8);
          wsprintf(lptszBuf, TEXT("Joystick ID: %d"), data[3]);
          SetDlgItemText(hWnd, IDC_JOYSTICKID, lptszBuf);
         }
         break;

         case WM_ACTIVATE:
            pdiCpl->GetDevice(&pdiDevice2);
            if(LOWORD(wParam) == WA_INACTIVE)
                pdiDevice2->Unacquire();
            else
                pdiDevice2->Acquire();
            break;  
             
         case WM_DESTROY:
		   if (pdijs)
				delete (pdijs);
                        DeleteObject(g_hbrBackground);
                        pdiCpl->GetDevice(&pdiDevice2);
                        pdiDevice2->Unacquire();

			break;
         case WM_COMMAND:
           if(LOWORD(wParam) == IDC_CALIBRATE)
           {
               LoadString(ghInst, IDS_CALIBRATEMSG, lptszBuf, STR_LEN_64);                      
               MessageBox(hWnd, lptszBuf, TEXT("NTPAD"), MB_ICONINFORMATION | MB_OK);       
               pdiCpl->GetDevice(&pdiDevice2);

               DIPROPDWORD dipdw;
               dipdw.diph.dwSize = sizeof(DIPROPDWORD);
               dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
               dipdw.diph.dwObj = 0;
               dipdw.diph.dwHow = DIPH_DEVICE;
               dipdw.dwData = DIPROPCALIBRATIONMODE_RAW;
               pdiDevice2->SetProperty(DIPROP_CALIBRATIONMODE, (LPDIPROPHEADER)&dipdw);

               if(SUCCEEDED(DIUtilPollJoystick(pdiDevice2, pdijs)))
               {
                  PLONG state = (PLONG) pdijs;

                  DIPROPCAL dipcal;
                  dipcal.diph.dwSize = sizeof(DIPROPCAL);
                  dipcal.diph.dwHeaderSize = sizeof(DIPROPHEADER);
                  dipcal.diph.dwHow = DIPH_BYUSAGE;

                  for(int i=0x30;i<=0x37;i++)
                  {
                    dipcal.diph.dwObj = DIMAKEUSAGEDWORD(1,i);
                    dipcal.lCenter = state[i-0x30];
                    dipcal.lMin = 0;
                    if((i<=0x32)  || (i==0x35))
                      dipcal.lMax = 255;
                    else
                      dipcal.lMax = 511;
                    pdiDevice2->SetProperty(DIPROP_CALIBRATION, (LPDIPROPHEADER)&dipcal);
                  }
               };
               dipdw.dwData = DIPROPCALIBRATIONMODE_COOKED;
               pdiDevice2->SetProperty(DIPROP_CALIBRATIONMODE, (LPDIPROPHEADER)&dipdw);
               
           }
           break;
         case WM_NOTIFY:
         {
            // perform any WM_NOTIFY processing...

            switch(((LPNMHDR)lParam)->code)
            {
               case PSN_SETACTIVE:
               LoadString(ghInst, IDS_BUTTONS, lptszBuf, STR_LEN_64);                      
               SetDlgItemText(hWnd, IDC_BUTTONS, lptszBuf);

		   	      // get the device object
	   	         pdiCpl->GetDevice(&pdiDevice2);

                  // we're the active page now
                  // try to acquire the device
                  if (SUCCEEDED(pdiDevice2->Acquire()))
                  {                            
                     SetTimer(hWnd, ID_POLLTIMER, 50, NULL);
                  }
                    else 
                  {
                      LoadString(ghInst, IDS_UNABLE, lptszBuf, STR_LEN_64);                      
                      MessageBox(hWnd, lptszBuf, TEXT("Error"), MB_ICONSTOP | MB_OK);
                      KillTimer(hWnd, ID_POLLTIMER);
                  }
                  break;

               case PSN_KILLACTIVE:
                  // we're no longer the active page
                  // 
                  // we need to unacquire now
                  pdiCpl->GetDevice(&pdiDevice2);
                  pdiDevice2->Unacquire();

                  // we're done with our timer
                  KillTimer(hWnd, ID_POLLTIMER);
                  break;
            }
         }
         return TRUE;

	case WM_CTLCOLORDLG:
       		return (LONG)g_hbrBackground;
        case WM_CTLCOLORSTATIC:
	    {
	        HDC hdcStatic = (HDC)wParam;
        	SetTextColor(hdcStatic, RGB(0, 255, 0));
	        SetBkMode(hdcStatic, TRANSPARENT);
        	return (LONG)g_hbrBackground;
	    }
	    break;

         case WM_TIMER:
			   // get the device object
            //pdiCpl->GetDevice(&pdiDevice2);
               
            ZeroMemory(pdijs, sizeof(DIJOYSTATE));
      
            if(SUCCEEDED(DIUtilPollJoystick(pdiDevice2, pdijs)))
               DisplayJoystickState(hWnd, pdijs);
            break;
    }
    
    return FALSE;
} //*** end CPLSVR1_Page4_DlgProc()

//===========================================================================
// CPLSVR1_Page2_DlgProc
//
// Callback proceedure CPLSVR1 property page #2 (Map).
//
// Parameters:
//  HWND    hWnd    - handle to dialog window
//  UINT    uMsg    - dialog message
//  WPARAM  wParam  - message specific data
//  LPARAM  lParam  - message specific data
//
// Returns: BOOL
//
//===========================================================================
INT_PTR CALLBACK CPLSVR1_Page2_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   static CDIGameCntrlPropSheet_X *pdiCpl;
   static LPDIRECTINPUTDEVICE2    pdiDevice2;
   static LPDIJOYSTATE  pdijs;
   static HANDLE hHID;
   const UCHAR resetmap[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
   static UCHAR orgmap[16];
   static UCHAR map[16];
   static unsigned char data[8];
   static HKEY hKeyNTPAD;
   static HKEY hKey;
   DWORD a;
   DWORD Size = 16;
   DWORD lCreated;
   static int last = 0xff;
   PUSHORT buttons;
   switch(uMsg)
   {
      case WM_INITDIALOG:
      {
         // perform any required initialization... 

         // get ptr to our object
         pdiCpl = (CDIGameCntrlPropSheet_X*)((PROPSHEETPAGE *)lParam)->lParam;
         
         // Save our pointer so we can access it later
         SetWindowLongPtr(hWnd, DWLP_USER, (LPARAM)pdiCpl);

         // initialize DirectInput
         if (FAILED(InitDInput(GetParent(hWnd), pdiCpl)))
         {
#ifdef _DEBUG
            OutputDebugString(TEXT("CPLSvr1 Page 4 - InitDInput failed!\n"));
#endif //_DEBUG
            return TRUE;
         }
		   pdijs = new (DIJOYSTATE);
			assert (pdijs);
          for(int i=0;i<32;i++)
          {
            TCHAR text[3];
            wsprintf(text, TEXT("%d"), i+1);
            SetDlgItemText(hWnd,IDC_TESTJOYBTNICON+i,text);
          }
          pdiCpl->GetHIDHandle(&hHID);
          ZeroMemory(data, 8);
          HidD_GetFeature(hHID, data, 8);
          TCHAR id[4];
          wsprintf(id, TEXT("%d"), data[3]);
          pdiCpl->GetConfighKey(&hKeyNTPAD);
          memcpy(orgmap, resetmap, 16);
          if (RegCreateKeyEx(hKeyNTPAD, id, 0, NULL, 0,
              KEY_ALL_ACCESS, NULL, &hKey, &lCreated)
                        == ERROR_SUCCESS)
          {
             RegQueryValueEx(hKey, TEXT("Map"), 0, &lCreated, orgmap, &Size);
             RegQueryValueEx(hKey, TEXT("Psx2"), 0, &lCreated, (LPBYTE)&isanalog, &Size);
          }
          memcpy(map, orgmap, 16);
          ActualizeMaps(hWnd, map);
          LoadString(ghInst, IDS_PRESSANDKEEP, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_LINEA1, lptszBuf);
          LoadString(ghInst, IDS_THEN, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_LINEA2, lptszBuf);
          LoadString(ghInst, IDS_FINALLY, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_LINEA3, lptszBuf);
          LoadString(ghInst, IDS_UNMAPPED, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_UNMAPPED, lptszBuf);
          LoadString(ghInst, IDS_MAPPEDAS, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_MAPPEDASTEXT, lptszBuf);
          for(int i=0;i<4;i++)
          {
            LoadString(ghInst, IDS_ANALOG, lptszBuf, STR_LEN_64);
            wsprintf(lptszBuf, TEXT("%s %d"), lptszBuf, i+1);
            SetDlgItemText(hWnd, IDC_ANALOG1+i, lptszBuf);
          }
          break;
      }
      case WM_DESTROY:
		   if (pdijs)
				delete (pdijs);
                   if (hKey)
                      RegCloseKey(hKey);
                      break;


      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
           case IDC_RESET:
              memcpy(map, resetmap, 16);
              ActualizeMaps(hWnd, map);
              PropSheet_Changed(GetParent(hWnd), hWnd);
              break;
           case IDC_UNMAPPED:
               if(last != 0xff)
               {
                  map[last] = 0xff;
                  PropSheet_Changed(GetParent(hWnd), hWnd);
                  ActualizeMaps(hWnd, map);
                }
                break;
           default:
             if((wParam>= IDC_1) && (wParam <= IDC_16))
             {
               if(last != 0xff)
               { 
                  map[last] = wParam - IDC_1;
                  ActualizeMaps(hWnd, map);
                  PropSheet_Changed(GetParent(hWnd), hWnd);
               }
             }                
             if((wParam>= IDC_ANALOG1) && (wParam <= IDC_ANALOG4))
             {
               if(last != 0xff)
               { 
                  map[last] = wParam - IDC_ANALOG1 + 16;
                  ActualizeMaps(hWnd, map);
                  PropSheet_Changed(GetParent(hWnd), hWnd);
               }
             }
             break;
         }

         break;
      case WM_NOTIFY:
         switch(((LPNMHDR)lParam)->code)
         {
               case PSN_SETACTIVE:
                  for(int i=0;i<4;i++)
                    EnableWindow(GetDlgItem(hWnd, IDC_ANALOG1+i), isanalog);
                  SetTimer(hWnd, ID_POLLTIMER, 100, NULL);
                  break;

               case PSN_KILLACTIVE:
                  KillTimer(hWnd, ID_POLLTIMER);
                  break;
               case PSN_RESET:
                  memcpy(map, orgmap, 16);
               case PSN_APPLY:
                  ActualizeMaps(hWnd, map);                  
                  RegSetValueEx(hKey, TEXT("Map"), 0, REG_BINARY, map, Size);
                  data[1] = 255;
                  WriteFile(hHID, (VOID*) data, 3, &a, NULL);
                  break;
           }
           return TRUE;
           break;
      case WM_TIMER:
           last = 0xff;
           ZeroMemory(data, 8);
           data[1] = 2;
           WriteFile(hHID, (VOID*) data, 3, &a, NULL);
           ZeroMemory(data, 8);
           HidD_GetFeature(hHID, data, 8);
           buttons = (PUSHORT) &data[1];
           for(int i = 0;i<16;i++)
           {
             if((*buttons >> i)&1)
             {
                last=i;
                SetButtonState(hWnd, (BYTE)i, TRUE );
             } else {
                SetButtonState(hWnd, (BYTE)i, FALSE );
             };
           };
           break;
    }
    
    return FALSE;
} //*** end CPLSVR1_Page4_DlgProc()

//===========================================================================
// CPLSVR1_Page3_DlgProc
//
// Callback proceedure CPLSVR1 property page #3 (Effects).
//
// Parameters:
//  HWND    hWnd    - handle to dialog window
//  UINT    uMsg    - dialog message
//  WPARAM  wParam  - message specific data
//  LPARAM  lParam  - message specific data
//
// Returns: BOOL
//
//===========================================================================
INT_PTR CALLBACK CPLSVR1_Page3_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
   CDIGameCntrlPropSheet_X *pdiCpl = (CDIGameCntrlPropSheet_X*)GetWindowLongPtr(hWnd, DWLP_USER);
   LPDIRECTINPUTDEVICE2    pdiDevice2 = NULL;
   static int mbig;
   static int msmall;
   static unsigned char data[15];
   static HANDLE hHID;
   static EFFECTLIST  *pel;
   static DIEFFECT diEffect;
   static DIENVELOPE sobre;
   static DICONSTANTFORCE constantforce;
   static DIRAMPFORCE rampforce;
   static DICONDITION condition;
   static DIPERIODIC periodic;
   static DICUSTOMFORCE customforce;
   static DWORD    dwAxes[2] = { DIJOFS_X, DIJOFS_Y };
   static LONG     lDirection[2] = { 1, 0 };

   DWORD a;
   switch(uMsg)
   {
      case WM_INITDIALOG:
         {
         // perform any required initialization... 

         // get ptr to our object
         pel = (EFFECTLIST *) LocalAlloc( 0, sizeof(EFFECTLIST) );
         pdiCpl = (CDIGameCntrlPropSheet_X*)((PROPSHEETPAGE *)lParam)->lParam;
         
         // Save our pointer so we can access it later
         SetWindowLongPtr(hWnd, DWLP_USER, (LPARAM)pdiCpl);

         // initialize DirectInput
         if (FAILED(InitDInput(GetParent(hWnd), pdiCpl)))
         {
#ifdef _DEBUG
             OutputDebugString(TEXT("CPLSvr1 Page 3 - InitDInput failed!\n"));
#endif //_DEBUG
             return TRUE;
         }

         // get the device object
         pdiCpl->GetDevice(&pdiDevice2);

         // display the supported effects
         if (FAILED(DisplayEffectList(hWnd, IDC_EFFECTLIST, pdiDevice2, pel)))
         {
             LoadString(ghInst, IDS_ERRORFF, lptszBuf, STR_LEN_64);

             SendDlgItemMessage(hWnd, IDC_EFFECTLIST, LB_ADDSTRING, 0, (LPARAM)lptszBuf);
             EnableWindow(GetDlgItem(hWnd, IDC_EFFECTLIST), 0);
         } else 
           SendDlgItemMessage(hWnd, IDC_EFFECTLIST, LB_SETCURSEL, 0, (LPARAM)0);

          LoadString(ghInst, IDS_SUPPORTED, lptszBuf, STR_LEN_64);
          SetDlgItemText(hWnd, IDC_SUPPORTED, lptszBuf);
          LoadString(ghInst, IDS_TESTEFFECT, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_TESTEFFECT, lptszBuf);
          LoadString(ghInst, IDS_MOTORS, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_MOTORSTEST, lptszBuf);
          LoadString(ghInst, IDS_SMALL, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_SMALL, lptszBuf);
          LoadString(ghInst, IDS_BIG, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_BIGTEXT, lptszBuf);
          SendDlgItemMessage(hWnd, IDC_BIG, TBM_SETRANGE,(WPARAM) TRUE,(LPARAM) MAKELONG(0, 255));
          LoadString(ghInst, IDS_STOP, lptszBuf, STR_LEN_64);                      
          SetDlgItemText(hWnd, IDC_STOP, lptszBuf);
         }
         break;
      case WM_HSCROLL:
         mbig = SendDlgItemMessage(hWnd, IDC_BIG, TBM_GETPOS, 0,0);
         pdiCpl->GetHIDHandle(&hHID);
         ZeroMemory(data, 8);
         data[1] = (char)msmall;
         data[2] = (char)mbig;
         WriteFile(hHID, (VOID*) data, 3, &a, NULL);
         return TRUE;
         break;
      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
           case IDC_SMALL:
             msmall = (SendDlgItemMessage(hWnd, IDC_SMALL, BM_GETCHECK, 0, 0) ==BST_CHECKED)?1:0;
             pdiCpl->GetHIDHandle(&hHID);
             ZeroMemory(data, 8);
             data[1] = (char)msmall;
             data[2] = (char)mbig;
             WriteFile(hHID, (VOID*) data, 3, &a, NULL);
             return TRUE;
           case IDC_TESTEFFECT:
              pdiCpl->GetDevice(&pdiDevice2);              
              sobre.dwSize = sizeof(sobre);
              sobre.dwAttackLevel = 0;
              sobre.dwAttackTime = (DWORD) DI_SECONDS;
              sobre.dwFadeLevel = 0;
              sobre.dwFadeTime = 0;
              ZeroMemory(&diEffect, sizeof(DIEFFECT));
              diEffect.dwSize = sizeof(DIEFFECT);
              diEffect.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
              diEffect.dwDuration = INFINITE;
              diEffect.dwSamplePeriod = 0;               // = default
              diEffect.dwGain = DI_FFNOMINALMAX;         // no scaling
              diEffect.dwTriggerButton = DIEB_NOTRIGGER;
              diEffect.dwTriggerRepeatInterval = 0;
              diEffect.cAxes = 2;
              diEffect.rgdwAxes = &dwAxes[0];
              diEffect.rglDirection = &lDirection[0];
              diEffect.lpEnvelope = &sobre;
              diEffect.dwStartDelay = 0;

              a = SendDlgItemMessage(hWnd, IDC_EFFECTLIST, LB_GETCURSEL, 0, 0);
              if(a != -1)
              {
                 
                 switch(DIEFT_GETTYPE(pel->rgdwEffType[a]))
                 {
                    case DIEFT_CONSTANTFORCE:
                        constantforce.lMagnitude = DI_FFNOMINALMAX;
                        diEffect.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
                        diEffect.lpvTypeSpecificParams = &constantforce; 
                        break;
                    case DIEFT_RAMPFORCE:
                        rampforce.lStart = 0;
                        rampforce.lEnd = DI_FFNOMINALMAX;
                        diEffect.cbTypeSpecificParams = sizeof(DIRAMPFORCE);
                        diEffect.lpvTypeSpecificParams = &rampforce;
                        diEffect.dwDuration = (DWORD)(2 * DI_SECONDS);
                        break;
                    case DIEFT_CONDITION:
                          condition.lOffset = 0;
                          condition.lPositiveCoefficient = 1000;
                          condition.lNegativeCoefficient = 0;
                          condition.dwPositiveSaturation = DI_FFNOMINALMAX;
                          condition.dwNegativeSaturation = 0;
                          condition.lDeadBand = 2000;
                          LoadString(ghInst, IDS_CONDITION, lptszBuf, STR_LEN_64);                      
                          MessageBox(hWnd, lptszBuf, TEXT("NTPAD"), MB_ICONINFORMATION | MB_OK);
                        diEffect.cbTypeSpecificParams = sizeof(DICONDITION);
                        diEffect.lpvTypeSpecificParams = &condition;
                        break;
                    case DIEFT_PERIODIC:
                        periodic.dwMagnitude = DI_FFNOMINALMAX;
                        periodic.lOffset = 0;
                        periodic.dwPhase = 0;
                        periodic.dwPeriod = 2 * DI_SECONDS;
                        diEffect.cbTypeSpecificParams = sizeof(DIPERIODIC);
                        diEffect.lpvTypeSpecificParams = &periodic;
                        break;
                    case DIEFT_CUSTOMFORCE:
                        customforce.cChannels = 2;
                        customforce.dwSamplePeriod = 2 * DI_SECONDS;
                        customforce.cSamples = 4;
                        customforce.rglForceData = fza;
                        diEffect.cbTypeSpecificParams = sizeof(DICUSTOMFORCE);
                        diEffect.lpvTypeSpecificParams = &customforce;
                        break;

                 };        
                 if(FAILED(a = pdiDevice2->CreateEffect(pel->rgGuid[a], &diEffect, &g_lpdiEffect, NULL)))
                 {
                          wsprintf(lptszBuf, TEXT("Error: %x"), a);
                          MessageBox(hWnd, lptszBuf, TEXT("Error"), MB_ICONSTOP | MB_OK);
                          g_lpdiEffect = NULL;
                          break;
                 }
                 pdiDevice2->Acquire();
                 g_lpdiEffect->Start(INFINITE,0);
                 EnableWindow(GetDlgItem(hWnd, IDC_STOP), 1);
                 EnableWindow(GetDlgItem(hWnd, IDC_TESTEFFECT), 0);

              };
              break;
           case IDC_STOP:
              EnableWindow(GetDlgItem(hWnd, IDC_STOP), 0);
              EnableWindow(GetDlgItem(hWnd, IDC_TESTEFFECT), 1);
              if(g_lpdiEffect != NULL)
              {
                 g_lpdiEffect->Stop();
                 g_lpdiEffect->Release();
                 g_lpdiEffect = NULL;
                 pdiCpl->GetDevice(&pdiDevice2);              
                 pdiDevice2->Unacquire();
              };
              break;
         }
         break;
      case WM_DESTROY:
          if(g_lpdiEffect != NULL)
          {
             g_lpdiEffect->Stop();
             g_lpdiEffect->Release();
             g_lpdiEffect = NULL;
             pdiCpl->GetDevice(&pdiDevice2);              
             pdiDevice2->Unacquire();
          };
          LocalFree((PVOID) pel);
          break;
      case WM_NOTIFY:
         {
            // perform any WM_NOTIFY processing...

            switch(((LPNMHDR)lParam)->code)
            {
               case PSN_SETACTIVE:
                  // get the device object
                  SetTimer(hWnd, ID_POLLTIMER, 100, NULL);
                  data[1] = (char)msmall;
                  data[2] = (char)mbig;
                  WriteFile(hHID, (VOID*) data, 3, &a, NULL);
                  EnableWindow(GetDlgItem(hWnd, IDC_STOP), 0);
                  EnableWindow(GetDlgItem(hWnd, IDC_TESTEFFECT), 1);
                  return TRUE;
                  break;

               case PSN_KILLACTIVE:
                  KillTimer(hWnd, ID_POLLTIMER);
                  if(g_lpdiEffect != NULL)
                  {
                     g_lpdiEffect->Stop();
                     g_lpdiEffect->Release();
                     g_lpdiEffect = NULL;
                     pdiCpl->GetDevice(&pdiDevice2);              
                     pdiDevice2->Unacquire();
                  };
                  ZeroMemory(data, 8);
                  WriteFile(hHID, (VOID*) data, 3, &a, NULL);
                  return TRUE;
                  break;
            }
         }
         // perform any WM_NOTIFY processing, but there is none...
         // return TRUE if you handled the notification (and have set
         // the result code in SetWindowLongPtr(hWnd, DWLP_MSGRESULT, lResult)
         // if you want to return a nonzero notify result)
         // or FALSE if you want default processing of the notification.
     case WM_TIMER:
        pdiCpl->GetHIDHandle(&hHID);
        ZeroMemory(data, 15);
        ReadFile(hHID, data, 15, &a, NULL);
        return TRUE;
    }
    
    return FALSE;
} //*** end CPLSVR1_Page3_DlgProc()

//===========================================================================
// CPLSVR1_Page4_DlgProc
//
// Callback proceedure CPLSVR1 property page #4.
//
// Parameters:
//  HWND    hWnd    - handle to dialog window
//  UINT    uMsg    - dialog message
//  WPARAM  wParam  - message specific data
//  LPARAM  lParam  - message specific data
//
// Returns: BOOL
//
//===========================================================================
INT_PTR CALLBACK CPLSVR1_Page4_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
   CDIGameCntrlPropSheet_X *pdiCpl = (CDIGameCntrlPropSheet_X*)GetWindowLongPtr(hWnd, DWLP_USER);

   static HKEY hKeyNTPAD;
   static HKEY hKey;
   static HKEY hKey2;
   static LPDIRECTINPUTJOYCONFIG pdiCfg;

   DWORD a;
   DWORD Size = 4;
   DWORD lCreated;
   DWORD NoFF = 0;
   DWORD NoAx = 0;
   DWORD allanalog = 0;
   DWORD scanmode = 0;
   DWORD delay = 4;
   DWORD death = 10;
   DWORD Ejes = 0x03020100;
   PUCHAR axismap = (PUCHAR) &Ejes;
   static unsigned char data[8];
   HANDLE hHID;
   switch(uMsg)
   {
      case WM_INITDIALOG:
         {
         // perform any required initialization... 

         // get ptr to our object
         pdiCpl = (CDIGameCntrlPropSheet_X*)((PROPSHEETPAGE *)lParam)->lParam;
         
         // Save our pointer so we can access it later
         SetWindowLongPtr(hWnd, DWLP_USER, (LPARAM)pdiCpl);

         // initialize DirectInput
         if (FAILED(InitDInput(GetParent(hWnd), pdiCpl)))
         {
#ifdef _DEBUG
            OutputDebugString(TEXT("CPLSvr1 Page 4 - InitDInput failed!\n"));
#endif //_DEBUG
            return TRUE;
         }

         pdiCpl->GetHIDHandle(&hHID);
         ZeroMemory(data, 8);
         HidD_GetFeature(hHID, data, 8);
         TCHAR id[4];
         wsprintf(id, TEXT("%d"), data[3]);
         pdiCpl->GetConfighKey(&hKeyNTPAD);
         if (RegCreateKeyEx(hKeyNTPAD, id, 0, NULL, 0,
              KEY_ALL_ACCESS, NULL, &hKey, &lCreated)
                        == ERROR_SUCCESS)
         {
             RegQueryValueEx(hKey, TEXT("noAx"), 0, &lCreated, (LPBYTE)&NoAx, &Size);
             RegQueryValueEx(hKey, TEXT("NoFF"), 0, &lCreated, (LPBYTE)&NoFF, &Size);
             RegQueryValueEx(hKey, TEXT("allanalog"), 0, &lCreated, (LPBYTE)&allanalog, &Size);
             RegQueryValueEx(hKey, TEXT("Psx2"), 0, &lCreated, (LPBYTE)&isanalog, &Size);
             RegQueryValueEx(hKey, TEXT("Delay"), 0, &lCreated, (LPBYTE)&delay, &Size);
             RegQueryValueEx(hKey, TEXT("ScanMode"), 0, &lCreated, (LPBYTE)&scanmode, &Size);
             RegQueryValueEx(hKey, TEXT("Ejes"), 0, &lCreated, (LPBYTE)&Ejes, &Size);
             RegQueryValueEx(hKey, TEXT("Death"), 0, &lCreated, (LPBYTE)&death, &Size);
         }
         #define CHECK(a)   (a)?BST_CHECKED:BST_UNCHECKED
         LoadString(ghInst, IDS_DISABLEFF, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_DISABLEFF, lptszBuf);
         SendDlgItemMessage(hWnd, IDC_DISABLEFF, BM_SETCHECK, (WPARAM) CHECK(NoFF), 0);

         LoadString(ghInst, IDS_DISABLEAX, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_DISABLEAX, lptszBuf);
         SendDlgItemMessage(hWnd, IDC_DISABLEAX, BM_SETCHECK, (WPARAM) CHECK(NoAx), 0);

         LoadString(ghInst, IDS_ALLWAYSANALOG, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_ALLWAYSANALOG, lptszBuf);
         SendDlgItemMessage(hWnd, IDC_ALLWAYSANALOG, BM_SETCHECK, (WPARAM) CHECK(allanalog), 0);

         LoadString(ghInst, IDS_DSHOCK2, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_DSHOCK2, lptszBuf);
         SendDlgItemMessage(hWnd, IDC_DSHOCK2, BM_SETCHECK, (WPARAM) CHECK(isanalog), 0);

         LoadString(ghInst, IDS_SCAN, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_SCAN, lptszBuf);
         SetDlgItemInt(hWnd, IDC_DELAY, delay, FALSE);

         LoadString(ghInst, IDS_AXES, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_AXIS, lptszBuf);

         LoadString(ghInst, IDS_ANALOGREMAP, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_ANALOGREMAP, lptszBuf);

         LoadString(ghInst, IDS_SCANMODE, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_SCANMODE, lptszBuf);

         LoadString(ghInst, IDS_SPEED, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_SPEED, lptszBuf);
         LoadString(ghInst, IDS_RESPONSE, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_RESPONSE, lptszBuf);
         CheckRadioButton(hWnd, IDC_SPEED, IDC_RESPONSE, IDC_SPEED+scanmode);

         if((Ejes == 0) || (Ejes == 1))
           Ejes = 0x03020100;

         for(int i =0; i<4;i++)
         {
            for(int j=0;j<4;j++)
            {
               LoadString(ghInst, IDS_X+j, lptszBuf, STR_LEN_64);
               SendDlgItemMessage(hWnd, IDC_EJE1+i, CB_ADDSTRING, 0, (LPARAM) lptszBuf);
            }
            SendDlgItemMessage(hWnd, IDC_EJE1+i, CB_SETCURSEL, (WPARAM)axismap[i], 0);
         }

         LoadString(ghInst, IDS_DEATH, lptszBuf, STR_LEN_64);
         SetDlgItemText(hWnd, IDC_DEATH, lptszBuf);
         SetDlgItemInt(hWnd, IDC_DEATHNUM, death, FALSE);


         }
         break;
         case WM_DESTROY:
                        if (hKey)
                           RegCloseKey(hKey);
			break;
         case WM_COMMAND:
                  PropSheet_Changed(GetParent(hWnd), hWnd);
                  break;

         case WM_NOTIFY:
         {
            // perform any WM_NOTIFY processing...

            switch(((LPNMHDR)lParam)->code)
            {
               case PSN_SETACTIVE:
                  break;
               case PSN_KILLACTIVE:
                  isanalog = (BST_CHECKED == SendDlgItemMessage(hWnd,IDC_DSHOCK2, BM_GETCHECK, 0, 0));
                  break;
               case PSN_APPLY:
                  NoFF = (BST_CHECKED == SendDlgItemMessage(hWnd,IDC_DISABLEFF, BM_GETCHECK, 0, 0));
                  NoAx = (BST_CHECKED == SendDlgItemMessage(hWnd,IDC_DISABLEAX, BM_GETCHECK, 0, 0));
                  isanalog = (BST_CHECKED == SendDlgItemMessage(hWnd,IDC_DSHOCK2, BM_GETCHECK, 0, 0));
                  allanalog = (BST_CHECKED == SendDlgItemMessage(hWnd,IDC_ALLWAYSANALOG, BM_GETCHECK, 0, 0));
                  delay = GetDlgItemInt(hWnd, IDC_DELAY, 0,0);
                  death = GetDlgItemInt(hWnd, IDC_DEATHNUM, 0,0);
                  scanmode = IsDlgButtonChecked(hWnd, IDC_RESPONSE);
                  for(int i=0; i<4;i++)
                    axismap[i] = (UCHAR) SendDlgItemMessage(hWnd, IDC_EJE1+i, CB_GETCURSEL, 0, 0);

                  RegSetValueEx(hKey, TEXT("NoFF"), 0, REG_DWORD, (const BYTE *)&NoFF, Size);
                  RegSetValueEx(hKey, TEXT("noAx"), 0, REG_DWORD, (const BYTE *)&NoAx, Size);
                  RegSetValueEx(hKey, TEXT("Allanalog"), 0, REG_DWORD, (const BYTE *)&allanalog, Size);
                  RegSetValueEx(hKey, TEXT("Psx2"), 0, REG_DWORD, (const BYTE *)&isanalog, Size);                  
                  RegSetValueEx(hKey, TEXT("Delay"), 0, REG_DWORD, (const BYTE *)&delay, Size);                  
                  RegSetValueEx(hKey, TEXT("ScanMode"), 0, REG_DWORD, (const BYTE *)&scanmode, Size);
                  RegSetValueEx(hKey, TEXT("Ejes"), 0, REG_DWORD, (const BYTE *)axismap, Size);
                  RegSetValueEx(hKey, TEXT("Death"), 0, REG_DWORD, (const BYTE *)&death, Size);

                  data[1] = 255;
                  pdiCpl->GetHIDHandle(&hHID);
                  WriteFile(hHID, (VOID*) data, 3, &a, NULL);
                  PropSheet_CancelToClose(GetParent(hWnd));
                  break;
            }
         }
         return TRUE;

    }
    

    return FALSE;
} //*** end CPLSVR1_Page4_DlgProc()


//===========================================================================
// CPLSVR1_Page5_DlgProc
//
// Callback proceedure CPLSVR1 property page #5.
//
// Parameters:
//  HWND    hWnd    - handle to dialog window
//  UINT    uMsg    - dialog message
//  WPARAM  wParam  - message specific data
//  LPARAM  lParam  - message specific data
//
// Returns: BOOL
//
//===========================================================================
INT_PTR CALLBACK CPLSVR1_Page5_DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                    LPARAM lParam)
{
   CDIGameCntrlPropSheet_X *pdiCpl = (CDIGameCntrlPropSheet_X*)GetWindowLongPtr(hWnd, DWLP_USER);

    
    return FALSE;
} //*** end CPLSVR1_Page5_DlgProc()



//===========================================================================
// InitDInput
//
// Initializes DirectInput objects
//
// Parameters:
//  HWND                    hWnd    - handle of caller's window
//  CDIGameCntrlPropSheet_X *pdiCpl - pointer to Game Controllers property
//                                      sheet object
//
// Returns: HRESULT
//
//===========================================================================
HRESULT InitDInput(HWND hWnd, CDIGameCntrlPropSheet_X *pdiCpl)
{
   // protect ourselves from multithreading problems
   EnterCriticalSection(&gcritsect);
   HRESULT       hRes;

   // validate pdiCpl
   if ((IsBadReadPtr((void*)pdiCpl, sizeof(CDIGameCntrlPropSheet_X))) ||
       (IsBadWritePtr((void*)pdiCpl, sizeof(CDIGameCntrlPropSheet_X))) )
   {
#ifdef _DEBUG
      OutputDebugString(TEXT("InitDInput() - bogus pointer!\n"));
#endif // _DEBUG
      hRes = E_POINTER;
      goto exitinit;
   }

   LPDIRECTINPUTDEVICE2    pdiDevice2;

   // retrieve the current device object
   pdiCpl->GetDevice(&pdiDevice2);   

   LPDIRECTINPUTJOYCONFIG  pdiJoyCfg ;

   // retrieve the current joyconfig object
   pdiCpl->GetJoyConfig(&pdiJoyCfg);

   HANDLE hHIDObject;

   pdiCpl->GetHIDHandle(&hHIDObject);

   LPDIRECTINPUT pdi;

   // have we already initialized DirectInput?
   if ((NULL == pdiDevice2) || (NULL == pdiJoyCfg) || (NULL == hHIDObject))
   {
      // no, create a base DirectInput object
      hRes = DirectInputCreate(ghInst, DIRECTINPUT_VERSION, &pdi, NULL);
      if(FAILED(hRes))
      {
#ifdef _DEBUG
         OutputDebugString(TEXT("DirectInputCreate() failed!\n"));
#endif //_DEBUG
         goto exitinit;
      }
   }
   else
   {
      hRes = S_FALSE;
      goto exitinit;
   }


   // have we already created a joyconfig object?
   if (NULL == pdiJoyCfg)
   {
      // no, create a joyconfig object
      hRes = pdi->QueryInterface(IID_IDirectInputJoyConfig, (LPVOID*)&pdiJoyCfg);
      if (SUCCEEDED(hRes))
      {
         // store the joyconfig object
         pdiCpl->SetJoyConfig(pdiJoyCfg);
      }
      else
      {
#ifdef _DEBUG
         OutputDebugString(TEXT("Unable to create joyconfig!\n"));
#endif // _DEBUG
         goto exitinit;
      }
   } else hRes = S_FALSE;

   // have we already created a device object?
   if (NULL == pdiDevice2)
   {
      // no, create a device object

      if (NULL != pdiJoyCfg)
      {
         hRes = DIUtilCreateDevice2FromJoyConfig(pdiCpl->GetID(), hWnd, pdi, pdiJoyCfg, &pdiDevice2);
         if (SUCCEEDED(hRes))
         {
            // store the device object
            pdiCpl->SetDevice(pdiDevice2);
         }
         else
         {
#ifdef _DEBUG
            OutputDebugString(TEXT("DIUtilCreateDevice2FromJoyConfig() failed!\n"));
#endif //_DEBUG
            goto exitinit;
         }
      }
      else
      {
#ifdef _DEBUG
         OutputDebugString(TEXT("No joyconfig object... cannot create device!\n"));
#endif // _DEBUG
         hRes = E_FAIL;
         goto exitinit;
      }
   } else hRes = S_FALSE;

   if (NULL == hHIDObject)
   {
      hRes = DIUtilGetHIDDevice(pdiDevice2, &hHIDObject);
      if (SUCCEEDED(hRes))
      {
         // store the joyconfig object
         pdiCpl->SetHIDHandle(hHIDObject);
      }
      else
      {
#ifdef _DEBUG
         OutputDebugString(TEXT("Unable to open HID device!\n"));
#endif // _DEBUG
         goto exitinit;
      }
   } else hRes = S_FALSE;

   if (NULL != pdi)
   {
      // release he base DirectInput object
      pdi->Release();
   }

exitinit:
   // we're done
   LeaveCriticalSection(&gcritsect);
   return hRes;
} //*** end InitDInput()


//===========================================================================
// EnumDeviceObjectsProc
//
// Enumerates DirectInput device objects (i.e. Axes, Buttons, etc)
//
// Parameters:
//  LPCDIDEVICEOBJECTINSTANCE   pdidoi  - ptr to device object instance data
//  LPVOID                      pv      - ptr to caller defined data
//
// Returns:
//  BOOL - DIENUM_STOP or DIENUM_CONTINUE
//
//===========================================================================
BOOL CALLBACK EnumDeviceObjectsProc(LPCDIDEVICEOBJECTINSTANCE pdidoi, LPVOID pv)
{
   // validate pointers
   if( (IsBadReadPtr((void*)pdidoi,sizeof(DIDEVICEINSTANCE))) )
   {
#ifdef _DEBUG
       OutputDebugString(TEXT("EnumDeviceObjectsProc() - invalid pdidoi!\n"));
#endif //_DEBUG
       return E_POINTER;
   }
   if( (IsBadReadPtr((void*)pv, 1)) ||
       (IsBadWritePtr((void*)pv, 1)) )
   {
#ifdef _DEBUG
       OutputDebugString(TEXT("EnumDeviceObjectsProc() - invalid pv!\n"));
#endif // _DEBUG
       return E_POINTER;
   }
    
	OBJECTINFO *poi = (OBJECTINFO*)pv;

	// store information about each object
   poi->rgGuidType[poi->nCount]    = pdidoi->guidType;
   poi->rgdwOfs[poi->nCount]       = pdidoi->dwOfs;
   poi->rgdwType[poi->nCount]      = pdidoi->dwType;
   lstrcpy(poi->rgtszName[poi->nCount], pdidoi->tszName);
    
   // increment the number of enum'd objects
   poi->nCount++;

   if (poi->nCount >= MAX_OBJECTS)
   {
      // we cannot handle any more objects...
      //
      // tell DirectInput to stop enumerating
      return DIENUM_STOP;
   }

   // let DInput enumerate until all the objects have been found
   return DIENUM_CONTINUE;
} //*** end EnumDeviceObjectsProc()


//===========================================================================
// EnumEffectsProc
//
// Enumerates DirectInput effect types
//
// Parameters:
//  LPCDIEFFECTINFO   pei   - ptr to effect information
//  LPVOID            pv    - ptr to caller defined data
//
// Returns:
//  BOOL - DIENUM_STOP or DIENUM_CONTINUE
//
//===========================================================================
BOOL CALLBACK EnumEffectsProc(LPCDIEFFECTINFO pei, LPVOID pv)
{
   // validate pointers
   if ((IsBadReadPtr((void*)pei,sizeof(DIEFFECTINFO))) )
   {
#ifdef _DEBUG
      OutputDebugString(TEXT("EnumEffectsProc() - invalid pei!\n"));
#endif //_DEBUG
      return E_POINTER;
   }
   if ((IsBadReadPtr ((void*)pv, 1)) ||
       (IsBadWritePtr((void*)pv, 1)) )
   {
#ifdef _DEBUG
      OutputDebugString(TEXT("EnumEffectsProc() - invalid pv!\n"));
#endif //_DEBUG
      return E_POINTER;
   }

   EFFECTLIST  *peList = (EFFECTLIST*)pv;

	// store information about each effect
   peList->rgGuid[peList->nCount]              = pei->guid;
   peList->rgdwEffType[peList->nCount]         = pei->dwEffType;
   peList->rgdwStaticParams[peList->nCount]    = pei->dwStaticParams;
   peList->rgdwDynamicParams[peList->nCount]   = pei->dwDynamicParams;
   lstrcpy(peList->rgtszName[peList->nCount], pei->tszName);
    
   // increment the number of enum'd effects
   peList->nCount++;

   if (peList->nCount >= MAX_OBJECTS)
   {
      // we cannot handle any more effects...
      //
      // tell DirectInput to stop enumerating
      return DIENUM_STOP;
   }

   // let DInput enumerate until all the objects have been found
   return DIENUM_CONTINUE;
} //*** end EnumEffectsProc()
//===========================================================================
// DisplayEffectList
//
// Displays the names of the device effects in the provided dialog
//  control.
//
// Parameters:
//  HWND                    hWnd        - window handle
//  UINT                    uListCtrlId - id of list box control
//  LPDIRECTINPUTDEVICE2    pdiDevice2  - ptr to device object
//
// Returns:
//
//===========================================================================
HRESULT DisplayEffectList(HWND hWnd, UINT uListCtrlId,
                          LPDIRECTINPUTDEVICE2 pdiDevice2,
                          EFFECTLIST *pel)
{
   // valdiate pointer(s)
   if ((IsBadReadPtr((void*)pdiDevice2, sizeof(IDirectInputDevice2))) ||
       (IsBadWritePtr((void*)pdiDevice2, sizeof(IDirectInputDevice2))) )
   {
#ifdef _DEBUG
        OutputDebugString(TEXT("DisplayEffectList() - invalid pdiDevice2!\n"));
#endif // _DEBUG
        return E_POINTER;
   }

   DIDEVCAPS   didc;

   // check to see if our device supports force feedback
   didc.dwSize = sizeof(DIDEVCAPS);

   HRESULT hRes = pdiDevice2->GetCapabilities(&didc);
   if(FAILED(hRes))
   {
#ifdef _DEBUG
      OutputDebugString(TEXT("DisplayEffectList() - GetCapabilities() failed!\n"));
#endif // _DEBUG
      return hRes;
   }

   if (!(didc.dwFlags & DIDC_FORCEFEEDBACK))
   {
      LoadString(ghInst, IDS_NOFORCEFEEDBACK, lptszBuf, STR_LEN_64);
      SendDlgItemMessage(hWnd, IDC_EFFECTLIST, LB_ADDSTRING, 0, (LPARAM)lptszBuf);
      return S_FALSE;
   }

   // enumerate effects

    if( pel )
    {
       pel->nCount = 0;
       hRes = pdiDevice2->EnumEffects((LPDIENUMEFFECTSCALLBACK)EnumEffectsProc, (LPVOID*)pel, DIEFT_ALL);

       if (SUCCEEDED(hRes))
       {
	       // display them to the list
               for(int i=0;i<pel->nCount;i++)
                       SendDlgItemMessage(hWnd, IDC_EFFECTLIST, LB_ADDSTRING, 0, (LPARAM)pel->rgtszName[i]);

       }
    #ifdef _DEBUG
       else OutputDebugString(TEXT("DisplayEffectList() - EnumEffects() failed!\n"));
    #endif // _DEBUG

    }
    else
    {
        hRes = E_OUTOFMEMORY;
    }

   // done
   return hRes;
} //*** end DisplayEffectList()

void DrawTelevisor(HWND hWnd, int recurso, int x, int y)
{
   HWND hVent;
   HDC hDC;
   HBITMAP hBmp;
   HBITMAP hBmp2;
   BITMAP bmp;
   HDC hbDC;
   HGDIOBJ hOld;
   hVent = GetDlgItem(hWnd, recurso);
   hDC = GetDC(hVent);
   hbDC = CreateCompatibleDC(hDC);
   hBmp = LoadBitmap(ghInst, MAKEINTRESOURCE(IDB_TELE));
   hOld   = SelectObject(hbDC, hBmp);
   hBmp2 = Load32bppTga(MAKEINTRESOURCE(IDB_PUNTITO), 1);
   GetObject(hBmp2, sizeof(BITMAP), &bmp);

   AlphaDraw(hbDC, ((90 * x) / 65535) + 11,
                   ((90 * y)/ 65535),
                   bmp.bmWidth, bmp.bmHeight, hBmp2);
   DeleteObject(hBmp2);
   GetObject(hBmp, sizeof(BITMAP), &bmp);
   BitBlt(hDC, 0,0,bmp.bmWidth, bmp.bmHeight, hbDC, 0,0,SRCCOPY);
   SelectObject(hbDC, hOld);
   DeleteObject(hBmp);
   DeleteDC(hbDC);
   ReleaseDC(hVent, hDC);
};
//===========================================================================
// DisplayJoystickState
//
// Displays the current joystick state in the provided dialog.
//
// Parameters:
//  HWND        hWnd    - window handle
//  DIJOYSTATE  *pdijs  - ptr to joystick state information
//
// Returns:
//
//===========================================================================
HRESULT DisplayJoystickState(HWND hWnd, DIJOYSTATE *pdijs)
{
   TCHAR   tszBtn[5];
   // validate pointer(s)
   if( (IsBadReadPtr((void*)pdijs, sizeof(DIJOYSTATE))) )
   {
#ifdef _DEBUG
       OutputDebugString(TEXT("DisplayJoystickState() - invalid pdijs!\n"));
#endif // _DEBUG
       return E_POINTER;
   }

   // x-axis
   wsprintf(lptszBuf, TEXT("%d"), pdijs->lX);
   SetDlgItemText(hWnd, IDC_XDATA, lptszBuf);

   // y-axis
   wsprintf(lptszBuf, TEXT("%d"), pdijs->lY);
   SetDlgItemText(hWnd, IDC_YDATA, lptszBuf);
   DrawTelevisor(hWnd, IDC_TELE1, pdijs->lX, pdijs->lY);

   // z-axis
   wsprintf(lptszBuf, TEXT("%d"), pdijs->lZ);
   SetDlgItemText(hWnd, IDC_ZDATA, lptszBuf);

   // rz-axis
   wsprintf(lptszBuf, TEXT("%d"), pdijs->lRz);
   SetDlgItemText(hWnd, IDC_RZDATA, lptszBuf);
   DrawTelevisor(hWnd, IDC_TELE2, pdijs->lRz, pdijs->lZ);

   // rx-axis
   wsprintf(lptszBuf, TEXT("%d"), pdijs->lRx - 32768);
   SetDlgItemText(hWnd, IDC_RXDATA2, lptszBuf);

   // ry-axis
   wsprintf(lptszBuf, TEXT("%d"), pdijs->lRy - 32768);
   SetDlgItemText(hWnd, IDC_RYDATA2, lptszBuf);

   // s1-axis
   wsprintf(lptszBuf, TEXT("%d"), pdijs->rglSlider[0] - 32768);
   SetDlgItemText(hWnd, IDC_S1DATA2, lptszBuf);

   // s2-axis
   wsprintf(lptszBuf, TEXT("%d"), pdijs->rglSlider[1] - 32768);
   SetDlgItemText(hWnd, IDC_S2DATA2, lptszBuf);

   for (BYTE i = 0; i < 32; i++)
   {
       if (pdijs->rgbButtons[i] & 0x80)
       {
          SetButtonState(hWnd, i, TRUE );
       } else {
          SetButtonState(hWnd, i, FALSE );
       }
   }

   // done
   return S_OK;
} //*** end DisplayJoystickState()

void ActualizeMaps(HWND hWnd, PUCHAR map)
{
  int i;
  TCHAR num[20];
  for(i=0;i<16;i++)
  {
    if(map[i]==0xFF)
      LoadString(ghInst, IDS_UNMAPPED, num, 20);                      
    else if(map[i]>15)
    {
      LoadString(ghInst, IDS_ANALOG, num, 20);
      wsprintf(num, TEXT("%s %d"), num, map[i]-15);
    }
    else
      wsprintf(num, TEXT("%d"), map[i]+1);
    SetDlgItemText(hWnd, IDC_MAPEDAS + i, num);
  };
}


































