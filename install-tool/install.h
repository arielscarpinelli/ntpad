#include <windows.h>
#include <windowsx.h>
#include <string.h>
#include <shellapi.h>
#include <prsht.h>
#include <windef.h>
#include <regstr.h>
#include <cfgmgr32.h>
#include <setupapi.h>
#include <newdev.h>
#include "resource.h"
#include <stdlib.h>
#include <stdio.h>
//
// String constants
//

//
// global variables
//
extern HINSTANCE g_hInstance;

//
// utility routines
//

VOID
DoValueAddWizard(
    IN LPCWSTR MediaRootDirectory
    );

