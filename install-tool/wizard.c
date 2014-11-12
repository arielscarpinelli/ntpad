/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    wizard.c

Abstract:

    This module implements the TOASTVA wizard that installs/updates toaster 
    drivers and allows the user to select additional value-added software.

--*/

#include "precomp.h"
#pragma hdrstop

//
// Constants
//
#define WMX_UPDATE_DRIVER_DONE (WM_USER + 500)

//
// Structures
//
WCHAR buffer[120] = {0};
typedef struct _SHAREDWIZDATA {
    HFONT   hTitleFont;          // Title font for the Welcome and Completion pages
    BOOL    HwInsertedFirst;     // Is the hardware already present?
    LPCWSTR MediaRootDirectory;  // Fully-qualified path to root of install media
    BOOL    DoDriverUpdatePage;  // Should Update Driver page do anything?
    BOOL    RebootRequired;      // Did we do anything that requires a reboot?
    HWND    hwndDlg;             // Handle to dialog notified by drv update thread
    HKEY    Padkey;
    int     uninstall;
    int     espa;
} SHAREDWIZDATA, *LPSHAREDWIZDATA;

//
// Function prototypes
//
int DoInstall(HWND hDlg);
VOID DoUninstall();
VOID DoReconfig();

INT_PTR
CALLBACK 
IntroDlgProc(
    IN HWND     hwndDlg,    
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    );

INT_PTR
CALLBACK 
SelectDoProc(
    IN HWND     hwndDlg,    
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    );

INT_PTR
CALLBACK
ConfigProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

INT_PTR
CALLBACK 
IntPage1DlgProc(
    IN HWND     hwndDlg,    
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    );

INT_PTR
CALLBACK 
EndDlgProc(
    IN HWND     hwndDlg,    
    IN UINT     uMsg,       
    IN WPARAM   wParam, 
    IN LPARAM   lParam  
    );

DWORD
WINAPI
UpdateDriverThreadProc(
    IN LPVOID ThreadData
    );

INT 
CALLBACK
WizardCallback(
    IN HWND   hwndDlg,
    IN UINT   uMsg,
    IN LPARAM lParam
    );

//
// Implementation
//

VOID
DoValueAddWizard(
    IN LPCWSTR MediaRootDirectory
    )

/*++

Routine Description:

    This routine displays a wizard that steps the user through the following
    actions:
    
    (a) Performs a "driver update" for any currently-present toasters
    (b) Installs the INF and CAT in case no toasters presently exist
    (c) Optionally, installs value-add software selected by the user (this
        wizard page is retrieved from the toaster co-installer, and is the same
        page the user gets if they do a "hardware-first" installation using our
        driver).

Arguments:

    MediaRootDirectory - Supplies the fully-qualified path to the root
        directory where the installation media is located.

Return Value:

    none

--*/

{
    PROPSHEETPAGE   psp =       {0}; //defines the property sheet pages
    HPROPSHEETPAGE  ahpsp[5] =  {0}; //an array to hold the page's HPROPSHEETPAGE handles
    PROPSHEETHEADER psh =       {0}; //defines the property sheet
    SHAREDWIZDATA wizdata =     {0}; //the shared data structure

    NONCLIENTMETRICS ncm = {0};
    LOGFONT TitleLogFont;
    HDC hdc;
    INT FontSize;
    INT index = 0;

    DWORD lCreated;
    HKEY hKey;
    if(RegCreateKeyEx(HKEY_LOCAL_MACHINE, (LPCWSTR) TEXT("System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\NTPAD"), 0, NULL,
                      0, KEY_ALL_ACCESS, NULL, &hKey, &lCreated) == ERROR_SUCCESS)
       wizdata.Padkey = hKey;

    //
    //Create the Wizard pages
    //
    // Intro page...
    //
    psp.dwSize =        sizeof(psp);
    psp.dwFlags =       PSP_DEFAULT|PSP_HIDEHEADER;
    psp.hInstance =     g_hInstance;
    psp.lParam =        (LPARAM) &wizdata; //The shared data structure
    psp.pfnDlgProc =    IntroDlgProc;
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_INTRO);

    ahpsp[index++] =          CreatePropertySheetPage(&psp);


    // Seleccione que hacer
    psp.dwFlags =           PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USETITLE;
    psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_TITLE3);
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_INICIAL);
    psp.pfnDlgProc =        SelectDoProc;

    ahpsp[index++] =              CreatePropertySheetPage(&psp);

    // Config
    psp.dwFlags =           PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USETITLE;
    psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_TITLE4);
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_CONFIG);
    psp.pfnDlgProc =        ConfigProc;

    ahpsp[index++] =              CreatePropertySheetPage(&psp);

    //
    // Updating drivers page...
    //
    psp.dwFlags =           PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE|PSP_USETITLE;
    psp.pszHeaderTitle =    MAKEINTRESOURCE(IDS_TITLE1);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_SUBTITLE1);
    psp.pszTemplate =       MAKEINTRESOURCE(IDD_INTERIOR1);
    psp.pfnDlgProc =        IntPage1DlgProc;

    ahpsp[index++] =              CreatePropertySheetPage(&psp);

    //
    // Finish page...
    //
    psp.dwFlags =       PSP_DEFAULT|PSP_HIDEHEADER;
    psp.pszTemplate =   MAKEINTRESOURCE(IDD_END);
    psp.pfnDlgProc =    EndDlgProc;

    ahpsp[index] =          CreatePropertySheetPage(&psp);

    //
    // Create the property sheet...
    //
    psh.dwSize =            sizeof(psh);
    psh.hInstance =         g_hInstance;
    psh.hwndParent =        NULL;
    psh.phpage =            ahpsp;
    psh.dwFlags =           PSH_WIZARD97|PSH_WATERMARK|PSH_HEADER|PSH_STRETCHWATERMARK|PSH_WIZARD|PSH_USECALLBACK|PSH_USEICONID;
    psh.pszIcon =           MAKEINTRESOURCE(IDI_ICON1);
    psh.pszbmWatermark =    MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader =       MAKEINTRESOURCE(IDB_BANNER);
    psh.nStartPage =        0;
    psh.nPages =            5;
    psh.pfnCallback =       WizardCallback;

    //
    // Set up the font for the titles on the intro and ending pages
    //
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    //
    // Create the intro/end title font
    //
    TitleLogFont = ncm.lfMessageFont;
    TitleLogFont.lfWeight = FW_BOLD;
    lstrcpy(TitleLogFont.lfFaceName, L"Verdana Bold");

    hdc = GetDC(NULL); //gets the screen DC
    FontSize = 12;
    TitleLogFont.lfHeight = 0 - GetDeviceCaps(hdc, LOGPIXELSY) * FontSize / 72;
    wizdata.hTitleFont = CreateFontIndirect(&TitleLogFont);
    ReleaseDC(NULL, hdc);
    wizdata.MediaRootDirectory = MediaRootDirectory;

    //
    // Display the wizard
    //
    PropertySheet(&psh);
    
    //
    // Destroy the fonts
    //
    DeleteObject(wizdata.hTitleFont);
         
    //
    // If we did anything that requires a reboot, prompt the user 
    // now.  Note that we need to do this regardle
    //
    RegCloseKey(hKey);
    if(wizdata.RebootRequired) {
        SetupPromptReboot(NULL, NULL, FALSE);
    }
}


INT_PTR
CALLBACK
IntroDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    This function is the dialog procedure for the Welcome page of the wizard.

Arguments:

    hwndDlg - Supplies a handle to the dialog box window

    uMsg - Supplies the message

    wParam - Supplies the first message parameter

    lParam - Supplies the second message parameter

Return Value:

    This dialog procedure always returns zero.

--*/

{
    LPSHAREDWIZDATA pdata;
    LPNMHDR lpnm;
    LCID locale;
    DWORD lCreated;
    DWORD Size = sizeof(DWORD);
    int i;
    DWORD langs[6] = {LANG_SPANISH, LANG_ENGLISH, LANG_PORTUGUESE, 
                      LANG_ITALIAN, LANG_JAPANESE, LANG_CHINESE};
    //
    // Retrieve the shared user data from GWL_USERDATA
    //
    pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg) {
    
        case WM_INITDIALOG :
            { 
                HWND hwndControl;

                //
                // Get the shared data from PROPSHEETPAGE lParam valueand load
                // it into GWL_USERDATA
                //
                pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;

                SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
                
                //
                // It's an intro/end page, so get the title font from the
                // shared data and use it for the title control
                //
                hwndControl = GetDlgItem(hwndDlg, IDC_WELLCOME);
                SetWindowFont(hwndControl,pdata->hTitleFont, TRUE);
                break;
            }

        case WM_NOTIFY :
            
            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code) {
                
                case PSN_SETACTIVE : 
                    //
                    // Enable the Next button
                    //
                    LoadString(g_hInstance, IDS_WELLCOME, buffer, sizeof(buffer)/ sizeof(WCHAR));
                    SetDlgItemText(hwndDlg, IDC_WELLCOME, buffer);
                    LoadString(g_hInstance, IDS_LANG, buffer, sizeof(buffer)/ sizeof(WCHAR));
                    SetDlgItemText(hwndDlg, IDC_LANG, buffer);
                    LoadString(g_hInstance, IDS_CHAMULLO, buffer, sizeof(buffer)/ sizeof(WCHAR));
                    SetDlgItemText(hwndDlg, IDC_CHAMULLO, buffer);

                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);

                    //
                    // When we're moving forward through the wizard, we want
                    // the driver update page to do its work.
                    //
                    locale = GetThreadLocale();
                    RegQueryValueEx(pdata->Padkey, (LPCWSTR)TEXT("Idioma"), 0, &lCreated, (LPBYTE)&locale, &Size);
                    CheckRadioButton(hwndDlg, IDC_ESP, IDC_CHI, IDC_ENG);
                    for(i=0;i<6;i++)
                      if(langs[i] == PRIMARYLANGID(LANGIDFROMLCID(locale)))
                           CheckRadioButton(hwndDlg, IDC_ESP, IDC_CHI, IDC_ESP+i);
                        
                    pdata->DoDriverUpdatePage = TRUE;
                    break;

                case PSN_WIZNEXT :
                    for(i=0;i<6;i++)
                    {
                      if(IsDlgButtonChecked(hwndDlg, IDC_ESP+i) == BST_CHECKED)
                      {
                        locale = MAKELCID(MAKELANGID((USHORT)langs[i], SUBLANG_NEUTRAL), SORT_DEFAULT);
                        SetThreadLocale(locale);
                        RegSetValueEx(pdata->Padkey, (LPCWSTR)TEXT("Idioma"), 0, REG_DWORD, (const BYTE *)&locale, Size);
                        if(langs[i] == LANG_SPANISH)
                          pdata->espa = 1;
                        else
                          pdata->espa = 0;
                      }
                    }
                    break;

                case PSN_RESET :
                    //Handle a Cancel button click, if necessary
                    break;

                default :
                    break;
            }
            break;

        default:
            break;
    }

    return 0;
}

INT_PTR
CALLBACK
SelectDoProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    This function is the dialog procedure for the Welcome page of the wizard.

Arguments:

    hwndDlg - Supplies a handle to the dialog box window

    uMsg - Supplies the message

    wParam - Supplies the first message parameter

    lParam - Supplies the second message parameter

Return Value:

    This dialog procedure always returns zero.

--*/

{
    LPSHAREDWIZDATA pdata;
    LPNMHDR lpnm;

    //
    // Retrieve the shared user data from GWL_USERDATA
    //
    pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg) {
    
        case WM_INITDIALOG :
            {
                //
                // Get the shared data from PROPSHEETPAGE lParam valueand load
                // it into GWL_USERDATA
                //
                pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;

                SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
                
                CheckRadioButton(hwndDlg, IDC_INSTALL, IDC_UNINSTALL, IDC_INSTALL);
                // It's an intro/end page, so get the title font from the
                //
                // shared data and use it for the title control
                //
                break;
            }

        case WM_NOTIFY :
            
            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code) {
                
                case PSN_SETACTIVE : 
                    //
                    // Enable the Next button
                    //

                    //
                    // When we're moving forward through the wizard, we want
                    // the driver update page to do its work.
                    //
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);

                    LoadString(g_hInstance, IDS_INSTALL, buffer, sizeof(buffer)/ sizeof(WCHAR));
                    SetDlgItemText(hwndDlg, IDC_INSTALL, buffer);
                    LoadString(g_hInstance, IDS_RECONFIGURE, buffer, sizeof(buffer)/ sizeof(WCHAR));
                    SetDlgItemText(hwndDlg, IDC_RECONFIG, buffer);
                    LoadString(g_hInstance, IDS_DOUNINSTALL, buffer, sizeof(buffer)/ sizeof(WCHAR));
                    SetDlgItemText(hwndDlg, IDC_UNINSTALL, buffer);
                    pdata->DoDriverUpdatePage = TRUE;
                    break;

                case PSN_WIZNEXT :
                    if(BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_UNINSTALL))
                    {
                      pdata->uninstall = TRUE;
                      SetWindowLong(hwndDlg, DWL_MSGRESULT, IDD_INTERIOR1);
                    } else if(BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RECONFIG))
                    {
                      pdata->uninstall = 2;
                    } else {
                        pdata->uninstall = 0;
                    };
                    //Handle a Next button click here
                    return TRUE;
                    break;

                case PSN_RESET :
                    //Handle a Cancel button click, if necessary
                    break;

                default :
                    break;
            }
            break;

        default:
            break;
    }

    return 0;
}

INT_PTR
CALLBACK
ConfigProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    This function is the dialog procedure for the Welcome page of the wizard.

Arguments:

    hwndDlg - Supplies a handle to the dialog box window

    uMsg - Supplies the message

    wParam - Supplies the first message parameter

    lParam - Supplies the second message parameter

Return Value:

    This dialog procedure always returns zero.

--*/

{
    LPSHAREDWIZDATA pdata;
    LPNMHDR lpnm;
    DWORD lCreated;
    DWORD Type = 0;
    DWORD Puerto = 0x378;
    DWORD Id = 1;
    DWORD Size = sizeof(DWORD);
    DWORD Cant = 0;
    unsigned int i, x;
    UCHAR map[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    HKEY hKey;

    //
    // Retrieve the shared user data from GWL_USERDATA
    //
    pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg) {
    
        case WM_INITDIALOG :
            {
                //
                // Get the shared data from PROPSHEETPAGE lParam valueand load
                // it into GWL_USERDATA
                //
                pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;

                SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);

                for(i=0;i<5;i++)
                {
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("PSX Original (DirectPad If.)"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("PSX Megatap (5 pads max.)"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("PSX Multitap"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("Super Nintendo"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("Nintendo"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("Atari"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("Genesis (Original)"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("Genesis New Interface"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("Saturn Digital"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("Saturn Analog"));
                SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_ADDSTRING, 0,
                                                (LPARAM)TEXT("Nintendo 64"));
                }

                for(x=0;x<5;x++)
                  for(i=1;i<6;i++)
                  {
                    _itow(i, buffer, 10);
                    SendMessage(GetDlgItem(hwndDlg, IDC_ID+x), CB_ADDSTRING, 0,
                               (LPARAM)buffer);
                  };

                for(i=1;i<6;i++)
                {
                  _itow(i, buffer, 10);
                  SendMessage(GetDlgItem(hwndDlg, IDC_COMBO1), CB_ADDSTRING, 0,
                                         (LPARAM) buffer);
                };

                //
                // It's an intro/end page, so get the title font from the
                // shared data and use it for the title control
                //
                break;
            }

        case WM_NOTIFY :
            
            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code) {
                
                case PSN_SETACTIVE : 
                    //
                    // Enable the Next button
                    //

                     PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
                     RegQueryValueEx(pdata->Padkey, (LPCWSTR)TEXT("Cantidad"), 0, &lCreated, (LPBYTE)&Cant, &Size);
                     if(Cant == 0)
                        Cant++;
                     SendDlgItemMessage(hwndDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM) Cant - 1, 0);
                     SendMessage(hwndDlg, WM_COMMAND, MAKEWPARAM(IDC_COMBO1, CBN_SELCHANGE), 0);

                     LoadString(g_hInstance, IDS_HOWMUCH, buffer, sizeof(buffer)/ sizeof(WCHAR));
                     SetDlgItemText(hwndDlg, IDC_HOWMUCH, buffer);
                     LoadString(g_hInstance, IDS_NOW, buffer, sizeof(buffer)/ sizeof(WCHAR));
                     SetDlgItemText(hwndDlg, IDC_NOW, buffer);
                     LoadString(g_hInstance, IDS_TYPE, buffer, sizeof(buffer)/ sizeof(WCHAR));
                     SetDlgItemText(hwndDlg, IDC_TYPEBOX, buffer);
                     LoadString(g_hInstance, IDS_PORT, buffer, sizeof(buffer)/ sizeof(WCHAR));
                     SetDlgItemText(hwndDlg, IDC_PORTBOX, buffer);

                     for(i=0;i<5;i++)
                     {
                        Type = 0;
                        Puerto = 0x378;
                        Id = i+1;

                        wsprintf(buffer, TEXT("%d"), i+1);
                        if (RegOpenKeyEx(pdata->Padkey, buffer, 0, KEY_ALL_ACCESS, &hKey)
                        == ERROR_SUCCESS)
			{
                            RegQueryValueEx(hKey, TEXT("Tipo"), 0, &lCreated, (LPBYTE)&Type, &Size);
                            RegQueryValueEx(hKey, TEXT("Puerto"), 0, &lCreated, (LPBYTE)&Puerto, &Size);
                            if(Puerto<0x10)
                              Puerto = 0x378;
                            RegQueryValueEx(hKey, TEXT("Id"), 0, &lCreated, (LPBYTE)&Id, &Size);
                            RegCloseKey(hKey);
			}

                        SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_SETCURSEL, (WPARAM) Type, 0);
                        wsprintf(buffer, TEXT("0x%x"), Puerto);
                        SetDlgItemText(hwndDlg, IDC_PORT+i, buffer);                        
                        SendMessage(GetDlgItem(hwndDlg, IDC_ID+i), CB_SETCURSEL, (WPARAM) Id-1, 0);
                    }
                    //
                    // When we're moving forward through the wizard, we want
                    // the driver update page to do its work.
                    //
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);

                    pdata->DoDriverUpdatePage = TRUE;
                    break;

                case PSN_WIZNEXT :
                case PSN_WIZBACK :

                    Cant = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO1), CB_GETCURSEL, 0, 0) + 1; 
                    RegSetValueEx(pdata->Padkey, (LPCWSTR)TEXT("Cantidad"), 0, REG_DWORD, (const BYTE *)&Cant, Size);

                    for(i=0;i<Cant;i++)
                    {
                        Type = SendMessage(GetDlgItem(hwndDlg, IDC_TIPO+i), CB_GETCURSEL, 0, 0);
                        GetDlgItemText(hwndDlg, IDC_PORT+i, buffer, sizeof(buffer) / sizeof(WCHAR));
                        if(!swscanf(buffer, TEXT("0x%x"), &Puerto))
                            Puerto = 0x378;
                        Id = SendMessage(GetDlgItem(hwndDlg, IDC_ID+i), CB_GETCURSEL, 0, 0) + 1; 
                
                        wsprintf(buffer, TEXT("%d"), i+1);
                        if (RegCreateKeyEx(pdata->Padkey, buffer, 0, NULL,
                             0, KEY_ALL_ACCESS, NULL, &hKey, &lCreated)
                              == ERROR_SUCCESS)
                        {
                            RegSetValueEx(hKey, TEXT("Tipo"), 0, REG_DWORD, (const BYTE *)&Type, Size);
                            RegSetValueEx(hKey, TEXT("Puerto"), 0, REG_DWORD, (const BYTE *)&Puerto, Size);
                            RegSetValueEx(hKey, TEXT("Id"), 0, REG_DWORD, (const BYTE *)&Id, Size);
                            Size = 16;
                            RegQueryValueEx(hKey, TEXT("Map"), 0, &lCreated, map, &Size);
                            RegSetValueEx(hKey, TEXT("Map"), 0, REG_BINARY, (const BYTE *)map, Size);
                            Size = 4;
                            RegCloseKey(hKey);
                        }
                    };
                    return 0;

                case PSN_RESET :
                    //Handle a Cancel button click, if necessary
                    break;

                default :
                    break;
            }
            break;

        case WM_COMMAND:
            switch( LOWORD(wParam) )
            {
               case IDC_COMBO1:
                 if(HIWORD(wParam) == CBN_SELCHANGE)
                 {
                    Cant = SendMessage(GetDlgItem(hwndDlg, IDC_COMBO1), CB_GETCURSEL, 0, 0);
                    for(i=0;i<5;i++)
                    {

                       EnableWindow(GetDlgItem(hwndDlg, IDC_TIPO+i), (i<=Cant));
                       EnableWindow(GetDlgItem(hwndDlg, IDC_PORT+i), (i<=Cant));
                       EnableWindow(GetDlgItem(hwndDlg, IDC_ID+i), (i<=Cant));
                    }
                 }              
            };                            
        default:
            break;
    }

    return 0;
}


INT_PTR
CALLBACK
IntPage1DlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    This function is the dialog procedure for the first interior wizard page.
    This page updates the drivers for any existing (present) devices, or
    installs the INF if there aren't any present devices.

Arguments:

    hwndDlg - Supplies a handle to the dialog box window

    uMsg - Supplies the message

    wParam - Supplies the first message parameter

    lParam - Supplies the second message parameter

Return Value:

    This dialog procedure always returns zero.

--*/

{
    LPSHAREDWIZDATA pdata;
    LPNMHDR lpnm;
    HANDLE hThread;
    HKEY hKey;
    DWORD UserPrompted;

    //
    // Retrieve the shared user data from GWL_USERDATA
    //
    pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg) {

        case WM_INITDIALOG :
            //
            // Get the PROPSHEETPAGE lParam value and load it into GWL_USERDATA
            //
            pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
            break;

        case WM_NOTIFY :

            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code) {

                case PSN_SETACTIVE :
                    //
                    // If we're coming here from the intro page, then disable 
                    // the Back and Next buttons (we're going to be busy for a
                    // little bit updating drivers).
                    //
                    // If we're coming to this page from anywhere else, 
                    // immediately jump to the intro page.
                    //
                    if(!pdata->DoDriverUpdatePage)
                    {
                            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK);
                            PropSheet_PressButton(GetParent(hwndDlg), PSBTN_BACK);
                    }
                    else
                    {
                        PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
                        EnableWindow(GetDlgItem(GetParent(hwndDlg), IDCANCEL), FALSE);

                        //
                        // Show "searching" animation...
                        //
                        ShowWindow(GetDlgItem(hwndDlg, IDC_ANIMATE1), SW_SHOW);
                        Animate_Open(GetDlgItem(hwndDlg, IDC_ANIMATE1), MAKEINTRESOURCE(IDA_SEARCHING));
                        Animate_Play(GetDlgItem(hwndDlg, IDC_ANIMATE1), 0, -1, -1);

                        //
                        // Create a thread to do the work of updating the 
                        // driver, etc.
                        //
                        pdata->hwndDlg = hwndDlg;

                        hThread = CreateThread(NULL,
                                               0,
                                               UpdateDriverThreadProc,
                                               pdata,
                                               0,
                                               NULL
                                              );

                        if(hThread) {
                            //
                            // Thread launched successfully--close the handle, 
                            // then just wait to be notified of thread's 
                            // completion.
                            //
                            CloseHandle(hThread);

                        } else {
                           // Es una pena, no tenemos lamparita, pero tenemos que laburar igual
                           if(pdata->uninstall==1)
                              DoUninstall();
                           else if(pdata->uninstall==2)
                              DoReconfig();
                           else
                              DoInstall(hwndDlg);
                            pdata->DoDriverUpdatePage = FALSE;
                            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                            PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
                        }
                    }
                    break;

                case PSN_WIZNEXT :
                    //Handle a Next button click, if necessary
                    break;

                case PSN_WIZBACK :
                    //Handle a Back button click, if necessary
                    break;

                case PSN_RESET :
                    //Handle a Cancel button click, if necessary
                    break;

                default :
                    break;
            }
            break;

        case WMX_UPDATE_DRIVER_DONE :
            //
            // Stop "searching" animation...
            //
            Animate_Stop(GetDlgItem(hwndDlg, IDC_ANIMATE1));
            ShowWindow(GetDlgItem(hwndDlg, IDC_ANIMATE1), SW_HIDE);

            pdata->DoDriverUpdatePage = FALSE;

            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
            EnableWindow(GetDlgItem(GetParent(hwndDlg), IDCANCEL), TRUE);

            PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);

            break;

        default:
            break;
    }

    return 0;
}


INT_PTR
CALLBACK
EndDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )

/*++

Routine Description:

    This function is the dialog procedure for the Finish page of the wizard.

Arguments:

    hwndDlg - Supplies a handle to the dialog box window

    uMsg - Supplies the message

    wParam - Supplies the first message parameter

    lParam - Supplies the second message parameter

Return Value:

    This dialog procedure always returns zero.

--*/

{
    LPSHAREDWIZDATA pdata;
    LPNMHDR lpnm;

    //
    // Retrieve the shared user data from GWL_USERDATA
    //
    pdata = (LPSHAREDWIZDATA) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    switch(uMsg) {
        
        case WM_INITDIALOG :
            { 
                HWND hwndControl;

                //
                // Get the shared data from PROPSHEETPAGE lParam value and load
                // it into GWL_USERDATA
                //
                pdata = (LPSHAREDWIZDATA) ((LPPROPSHEETPAGE) lParam) -> lParam;
                SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR) pdata);
                
                //
                // It's an intro/end page, so get the title font from userdata
                // and use it on the title control
                //
                hwndControl = GetDlgItem(hwndDlg, IDC_TITLE);
                SetWindowFont(hwndControl,pdata->hTitleFont, TRUE);
                break;
            }

        case WM_NOTIFY :

            lpnm = (LPNMHDR)lParam;

            switch(lpnm->code) {
                
                case PSN_SETACTIVE : 
                    //
                    // Enable the correct buttons for the active page
                    //
                    LoadString(g_hInstance, IDS_TITLE2, buffer, sizeof(buffer)/ sizeof(WCHAR));
                    SetDlgItemText(hwndDlg, IDC_TITLE, buffer);
                    LoadString(g_hInstance, IDS_README, buffer, sizeof(buffer)/ sizeof(WCHAR));
                    SetDlgItemText(hwndDlg, IDC_README, buffer);

                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_FINISH);

                    //
                    // Doesn't make sense to have Cancel button enabled here
                    //
                    EnableWindow(GetDlgItem(GetParent(hwndDlg),  IDCANCEL), FALSE);

                    //                                                             
                    // Si desinstalamos, lo puteamos...
                    //
                    switch(pdata->uninstall)
                    {
                       case 1:
                           LoadString(g_hInstance, IDS_UNINSTALL, buffer, sizeof(buffer)/ sizeof(WCHAR));
                           break;
                       case 2:
                           LoadString(g_hInstance, IDS_RECONFIG, buffer, sizeof(buffer)/ sizeof(WCHAR));
                           break;
                       default:
                           LoadString(g_hInstance, IDS_END1, buffer, sizeof(buffer)/ sizeof(WCHAR));
                           break;
                    }

                    SetDlgItemText(hwndDlg, IDC_FINISH_TEXT, buffer);
                    pdata->DoDriverUpdatePage = FALSE;
                    break;

                case PSN_WIZBACK :
                    //
                    // Jumping back from this page, so turn Cancel button back on.
                    //
                    EnableWindow(GetDlgItem(GetParent(hwndDlg),  IDCANCEL), TRUE);
                    break;

                default :
                    break;
            }
            break;
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
              case IDC_README:
                GetCurrentDirectory(120, buffer);
                if(pdata->espa)
                  ShellExecute(hwndDlg, NULL, TEXT("readme.htm"), NULL, buffer, SW_SHOW);
                else
                  ShellExecute(hwndDlg, NULL, TEXT("readmeeng.htm"), NULL, buffer, SW_SHOW);
                break;
            };
            break;
        default:
            break;
    }

    return 0;
}

DWORD
WINAPI
UpdateDriverThreadProc(
    IN LPVOID ThreadData
    )

/*++

Routine Description:

    This function updates the drivers for any existing toasters.  If there are
    no toasters currently connected to the computer, it installs the INF/CAT so
    that the system will be ready to automatically install toasters that are
    plugged in later.  This routine will also mark any non-present (aka, 
    "phantom") toasters as needs-reinstall, so that they'll be updated to the
    new driver if they're ever plugged in again.

Arguments:

    ThreadData - Supplies a pointer to a SHAREDWIZDATA structure that's used
        both by this thread, and by the wizard in the main thread.

Return Value:

    If successful, the function returns NO_ERROR.
    
    Otherwise, the function returns a Win32 error code indicating the cause of
    failure.

--*/

{
    LPSHAREDWIZDATA pdata;   

    pdata = (LPSHAREDWIZDATA)ThreadData;
                           if(pdata->uninstall==1)
                              DoUninstall();
                           else if(pdata->uninstall==2)
                              DoReconfig();
                           else
                              DoInstall(pdata->hwndDlg);
    PostMessage(pdata->hwndDlg, WMX_UPDATE_DRIVER_DONE, 0, 0);

    return NO_ERROR;
}


INT 
CALLBACK
WizardCallback(
    IN HWND   hwndDlg,
    IN UINT   uMsg,
    IN LPARAM lParam
    )

/*++

Routine Description:

    Call back used to remove the "X" and  "?" from the wizard page.

Arguments:

    hwndDlg - Handle to the property sheet dialog box.

    uMsg - Identifies the message being received. This parameter is one of the
        following values:

        PSCB_INITIALIZED - Indicates that the property sheet is being
                           initialized. The lParam value is zero for this 
                           message.

        PSCB_PRECREATE   - Indicates that the property sheet is about to be
                           created. The hwndDlg parameter is NULL and the 
                           lParam parameter is a pointer to a dialog template 
                           in memory. This template is in the form of a 
                           DLGTEMPLATE structure followed by one or more
                           DLGITEMTEMPLATE structures.

    lParam - Specifies additional information about the message. The
        meaning of this value depends on the uMsg parameter.

Return Value:

    The function returns zero.

--*/

{
    DLGTEMPLATE *pDlgTemplate;

    switch(uMsg) {

        case PSCB_PRECREATE:
            if(lParam){
                //
                // This is done to hide the X and ? at the top of the wizard
                //
                pDlgTemplate = (DLGTEMPLATE *)lParam;
                pDlgTemplate->style &= ~(DS_CONTEXTHELP | WS_SYSMENU);
            }
            break;

        default:
            break;
    }

    return 0;
}

