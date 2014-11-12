#include "precomp.h"
#pragma hdrstop
VOID CleanInf(void)
{
  HANDLE h;
  WIN32_FIND_DATA FindFileData;
  TCHAR wininf[MAX_PATH];
  TCHAR ntpadinf[MAX_PATH];
  TCHAR busca[MAX_PATH];
  FILE *archivo;
  PUCHAR buffer;
  CHAR *del = 0;
  TCHAR *pnp;

  buffer = (PUCHAR) malloc(4096);
  if(!buffer)
    return;

  GetWindowsDirectory(wininf, sizeof(wininf));
  wcscat(wininf, TEXT("\\inf\\"));

  wcscpy(ntpadinf, wininf);
  wcscat(ntpadinf, TEXT("ntpad.inf"));
  DeleteFile(ntpadinf);

  wcscpy(ntpadinf, wininf);
  wcscat(ntpadinf, TEXT("padenum.inf"));
  DeleteFile(ntpadinf);

  wcscpy(busca, wininf);
  wcscat(busca, TEXT("oem*.inf"));
  h = FindFirstFile(busca, &FindFileData);
  if (h != INVALID_HANDLE_VALUE)
  {
	  do {
                wcscpy(ntpadinf, wininf);
                wcscat(ntpadinf, FindFileData.cFileName);
                archivo = _wfopen(ntpadinf, TEXT("r"));                          
		if (archivo)
		{
                    fread(buffer, 1, 4096, archivo);
                    del = strstr(buffer, "PID_2222");
		    fclose(archivo);
		    if (del != NULL)
			{
		    	DeleteFile(ntpadinf);
                        pnp = wcsstr(ntpadinf, TEXT(".inf"));
                        pnp[1] = 'p';
		     	DeleteFile(ntpadinf);	  
			};
		}
	  } while (FindNextFile(h, &FindFileData));
	  FindClose(h);
  };
  free(buffer);
};

BOOL StateChange(DWORD NewState, PSP_DEVINFO_DATA DeviceInfoData,
                 HDEVINFO hDevInfo)
{
    SP_PROPCHANGE_PARAMS PropChangeParams = {sizeof(SP_CLASSINSTALL_HEADER)};

    //
    // Set the PropChangeParams structure.
    //
    PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    PropChangeParams.Scope = DICS_FLAG_GLOBAL;
    PropChangeParams.StateChange = NewState; 

    if (!SetupDiSetClassInstallParams(hDevInfo,
        DeviceInfoData,
        (SP_CLASSINSTALL_HEADER *)&PropChangeParams,
        sizeof(PropChangeParams)))
    {
        return FALSE;
    }

    //
    // Call the ClassInstaller and perform the change.
    //
    if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
        hDevInfo,
        DeviceInfoData))
    {
        return FALSE;
    }

    return TRUE;
}

int RemoveHardware(WCHAR * Id, int remove)
{
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD i,err, del;
    DWORD DataT;
    LPTSTR p,buffer = NULL;
    DWORD buffersize = 0;
    del = 0;
    //
    // Create a Device Information Set with all present devices.
    //
    DeviceInfoSet = SetupDiGetClassDevs(NULL, // All Classes
        0,
        0, 
        DIGCF_ALLCLASSES | DIGCF_PRESENT ); // All devices present on system
    if (DeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        return 0;
    }
    
    //
    //  Enumerate through all Devices.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i=0;SetupDiEnumDeviceInfo(DeviceInfoSet,i,&DeviceInfoData);i++)
    {
        
        //
        // We won't know the size of the HardwareID buffer until we call
        // this function. So call it with a null to begin with, and then 
        // use the required buffer size to Alloc the nessicary space.
        // Keep calling we have success or an unknown failure.
        //
        while (!SetupDiGetDeviceRegistryProperty(
            DeviceInfoSet,
            &DeviceInfoData,
            SPDRP_HARDWAREID,
            &DataT,
            (PBYTE)buffer,
            buffersize,
            &buffersize))
        {
            if (GetLastError() == ERROR_INVALID_DATA)
            {
                //
                // May be a Legacy Device with no HardwareID. Continue.
                //
                break;
            }
            else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                //
                // We need to change the buffer size.
                //
                if (buffer) 
                    free(buffer);
                buffer = malloc(buffersize);
                if(buffer) ZeroMemory(buffer, buffersize);
            }
            else
            {
                //
                // Unknown Failure.
                //
                goto cleanup_DeviceInfo;
            }            
        }

        if (GetLastError() == ERROR_INVALID_DATA) 
            continue;
        
        //
        // Compare each entry in the buffer multi-sz list with our HardwareID.
        //
        for (p=buffer;*p&&(p<&buffer[buffersize]);p+=lstrlen(p)+sizeof(TCHAR))
        {

            if (!wcscmp(Id,p))
            {

                //
                // Worker function to remove device.
                //
                if(remove)
                {
                  if (SetupDiCallClassInstaller(DIF_REMOVE,
                      DeviceInfoSet,
                      &DeviceInfoData))
                  {
                     del = 1;
                  } 
                  break;
                } else {
                  if(StateChange(DICS_PROPCHANGE, &DeviceInfoData, DeviceInfoSet))
                  {
                     del = 1;
                  }
                  break;
                }
            }
        }

        if (buffer) free(buffer);
        buffer = NULL;
        buffersize = 0;
    }

    
    //
    //  Cleanup.
    //    
cleanup_DeviceInfo:
    err = GetLastError();
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    
    return (del); 
}


BOOL InstallRootEnumeratedDriver(IN  WCHAR * HardwareId,
    IN  WCHAR * INFFile,
    OUT PBOOL  RebootRequired  OPTIONAL,
    HWND hDlg
    )
/*++

Routine Description:

    This routine creates and installs a new root-enumerated devnode.

Arguments:

    HardwareIdList - Supplies a multi-sz list containing one or more hardware
    IDs to be associated with the device.  These are necessary in order 
    to match up with an INF driver node when we go to do the device 
    installation.

    InfFile - Supplies the full path to the INF File to be used when 
    installing this device.

    RebootRequired - Optionally, supplies the address of a boolean that is 
    set, upon successful return, to indicate whether or not a reboot is 
    required to bring the newly-installed device on-line.

Return Value:

    The function returns TRUE if it is successful. 

    Otherwise it returns FALSE and the logged error can be retrieved 
    with a call to GetLastError. 

--*/
{
    HDEVINFO DeviceInfoSet = 0;
    SP_DEVINFO_DATA DeviceInfoData;
    GUID ClassGUID;
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD err;
    
    //
    // Use the INF File to extract the Class GUID. 
    //
    if (!SetupDiGetINFClass(INFFile,&ClassGUID,ClassName,sizeof(ClassName),0))
    {
        return 0;
    }
    
    //
    // Create the container for the to-be-created Device Information Element.
    //
    DeviceInfoSet = SetupDiCreateDeviceInfoList(&ClassGUID,0);
    if(DeviceInfoSet == INVALID_HANDLE_VALUE) 
    {
        return 0;
    }
    
    // 
    // Now create the element. 
    // Use the Class GUID and Name from the INF file.
    //
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfo(DeviceInfoSet,
        ClassName,
        &ClassGUID,
        NULL,
        0,
        DICD_GENERATE_ID,
        &DeviceInfoData))
    {
        goto cleanup_DeviceInfo;
    }
    
    //
    // Add the HardwareID to the Device's HardwareID property.
    //
    if(!SetupDiSetDeviceRegistryProperty(DeviceInfoSet,
        &DeviceInfoData,
        SPDRP_HARDWAREID,
        (LPBYTE)HardwareId,
        (lstrlen(HardwareId)+1+1)*sizeof(TCHAR))) 
    {
        goto cleanup_DeviceInfo;
    }
    
    //
    // Transform the registry element into an actual devnode 
    // in the PnP HW tree.
    //
    if (!SetupDiCallClassInstaller(DIF_REGISTERDEVICE,
        DeviceInfoSet,
        &DeviceInfoData))
    {
        goto cleanup_DeviceInfo;
    }
    
    //
    // The element is now registered. We must explicitly remove the 
    // device using DIF_REMOVE, if we encounter any failure from now on.
    //
    
    //
    // Install the Driver.
    //
    if (!UpdateDriverForPlugAndPlayDevices(hDlg,
        HardwareId,
        INFFile,
        INSTALLFLAG_FORCE,
        RebootRequired))
    {
        DWORD err = GetLastError();
        
        if (!SetupDiCallClassInstaller(
            DIF_REMOVE,
            DeviceInfoSet,
            &DeviceInfoData))
        {
        }
        SetLastError(err);
    }
    
    //
    //  Cleanup.
    //    
cleanup_DeviceInfo:
    err = GetLastError();
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    SetLastError(err);
    
    return err == NO_ERROR;
}

VOID UninstallOldVersions(VOID)
{
    HKEY  hKey, hKey2, hKey3, hKey4;
    DWORD lCreated;
    DWORD link;
    DWORD created;
    DWORD oword;
    DWORD doscinco = 1024;
    BOOL  ret = 0;
    BOOL rem = 0;
    TCHAR  KeyName[MAX_PATH];
    DWORD tam;
    DWORD i;
    TCHAR GamePort[] = TEXT("GamePort\\VID_045E&PID_2222");
    PWCHAR p, pos;
    UCHAR zero[1] = {0};
    rem = RemoveHardware(GamePort, 1);
	{
          p = (PWCHAR) malloc(doscinco);
          if(!p) return;
          if(RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                TEXT("SYSTEM\\CurrentControlSet\\Control\\DeviceClasses\\{cae56030-684a-11d0-d6f6-00a0c90f57da}"),
		  			    0, NULL, 0, KEY_ALL_ACCESS, NULL, &hKey, &lCreated) == ERROR_SUCCESS)
	{	
		RegQueryInfoKey(hKey, NULL, NULL, NULL, &created, NULL, NULL, NULL, NULL,
			            NULL, NULL, NULL);
		for(i = 0;(i < (created + 1)) & (! ret);i++)
		{
		  if(RegEnumKey(hKey, i, KeyName, MAX_PATH) == ERROR_SUCCESS) 
		  {
			  if(RegOpenKeyEx(hKey, KeyName, 0, KEY_ALL_ACCESS, &hKey2) == ERROR_SUCCESS)
			  {
                                 if(RegOpenKeyEx(hKey2, TEXT("#\\Control"), 0, KEY_ALL_ACCESS, &hKey3) == ERROR_SUCCESS)
				 {
					link = 0;
					tam = sizeof(link);
                                        RegQueryValueEx(hKey3, TEXT("Linked"), 0, &oword, (LPBYTE) &link, &tam);
					if(link)
					{
                                          if(RegOpenKeyEx(hKey2, TEXT("#\\Device Parameters\\DirectX"), 0, KEY_ALL_ACCESS, &hKey4) == ERROR_SUCCESS)
                                          {
                                            doscinco = 1024;
                                            RegQueryValueEx(hKey4, TEXT("Config"), 0, &lCreated, (LPBYTE) p, &doscinco);
                                            pos = wcsstr(p, GamePort);
                                            if(((pos != NULL) | rem))
                                              RegSetValueEx(hKey4, TEXT("Config"), 0, REG_BINARY, zero, 0);
                                            RegFlushKey(hKey4);
                                            RegCloseKey(hKey4);
					    ret = 1;
					  }
					};
					RegCloseKey(hKey3);
				 };
			     RegCloseKey(hKey2);
			  };
		  };
		};
		RegCloseKey(hKey);
	};
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                TEXT("SYSTEM\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\DirectInput\\Gameports"), 0, KEY_READ || KEY_QUERY_VALUE ||KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
                if(RegOpenKeyEx(hKey, TEXT("0"), 0, KEY_SET_VALUE || KEY_QUERY_VALUE, &hKey2) == ERROR_SUCCESS)
		{
                                            doscinco = 1024;
                                            RegQueryValueEx(hKey2, TEXT("Config"), 0, &lCreated,(LPBYTE) p, &doscinco);
                                            pos = wcsstr(p, GamePort);
                                            if(((pos != NULL) | rem))
                                              RegSetValueEx(hKey2, TEXT("Config"), 0, REG_BINARY, zero, 0);
                                            RegCloseKey(hKey2);
                                            ret = 1;
		};
                RegCloseKey(hKey);
	};
       free(p);
    };
    RegDeleteKey(HKEY_LOCAL_MACHINE, TEXT("Software\\NTPAD"));
}
int DoInstall(HWND hDlg)
{
  TCHAR dir[MAX_PATH];
  TCHAR source[MAX_PATH];
  WCHAR Pad[] = TEXT("NTPAD\\VID_045E&PID_2222");
  WCHAR bus[] = TEXT("root\\padenum\0\0");
  BOOL RebootRequired;
  UninstallOldVersions();
  CleanInf();
  GetCurrentDirectory(sizeof(source), source);
  wcscpy(dir, source);
  wcscat(dir, TEXT("\\ntpad.inf"));
  if (! SetupCopyOEMInf(dir, source,SPOST_PATH, 0, NULL, 0, NULL, NULL))
  {
    	if (GetLastError() == ERROR_FILE_EXISTS)
	    	SetupCopyOEMInf(dir, source,SPOST_PATH, SP_COPY_REPLACEONLY, NULL, 0, NULL, NULL);
    	else {
                MessageBox(NULL, TEXT("Error insperado"), TEXT("Error"), MB_ICONSTOP | MB_OK);
		    return 0;
		};
  }; 
  wcscat(source, TEXT("\\padenum.inf"));

  if (!UpdateDriverForPlugAndPlayDevices(GetParent(hDlg), // No Window Handle
            bus, // Hardware ID
            source, 
            INSTALLFLAG_FORCE,
            &RebootRequired))
    {
        if (GetLastError()!= ERROR_NO_SUCH_DEVINST)
        {
            //
            // Otro error que no es que no exista
            //
            return 2; // Install Failure
        }
        
        // 
        // Driver Does not exist, Create and call the API.
        // HardwareID must be a multi-sz string, which argv[2] is.
        //
        if (!InstallRootEnumeratedDriver(bus, // HardwareID
            source, // FileName
            &RebootRequired, GetParent(hDlg)))
        {
            return 2; // Install Failure
        }
    } else {
      // Ya estaba instalado, updatemos el otro tambien
      if (!UpdateDriverForPlugAndPlayDevices(GetParent(hDlg), // No Window Handle
            Pad, // Hardware ID
            dir, 
            INSTALLFLAG_FORCE,
            &RebootRequired))
          return 2;
    };

  return 0;
}
VOID DoUninstall(VOID)
{
  WCHAR Pad[] = TEXT("NTPAD\\VID_045E&PID_2222");
  WCHAR bus[] = TEXT("root\\padenum");
  unsigned int rb = 0;
  UninstallOldVersions();
  CleanInf();
  if(RemoveHardware(Pad, 1))
  {
    if(RemoveHardware(bus, 1))
    {
    TCHAR system[MAX_PATH];

    GetSystemDirectory(system, sizeof(system));
    wcscat(system, TEXT("\\drivers\\ntpad.sys"));
    if(!DeleteFile(system))
    {
        MoveFileEx(system, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        rb = 1;
    }

    GetSystemDirectory(system, sizeof(system));
    wcscat(system, TEXT("\\ntpad.dll"));
    if(!DeleteFile(system))
    {
        MoveFileEx(system, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        rb = 1;
    }

    GetSystemDirectory(system, sizeof(system));
    wcscat(system, TEXT("\\ntpadcpl.dll"));
    if(!DeleteFile(system))
    {
        MoveFileEx(system, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        rb = 1;
    }

    GetSystemDirectory(system, sizeof(system));
    wcscat(system, TEXT("\\drivers\\padenum.sys"));
    if(!DeleteFile(system))
    {
        MoveFileEx(system, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        rb = 1;
    }
    };
  }
  return;
}

VOID DoReconfig(VOID)
{
  WCHAR Pad[] = TEXT("NTPAD\\VID_045E&PID_2222");
  if(!RemoveHardware(Pad, 0))
      MessageBox(NULL, TEXT("No se encuentra instalado el NTPAD en esta PC"), TEXT("Error"), MB_ICONSTOP | MB_OK);
  return;
}
