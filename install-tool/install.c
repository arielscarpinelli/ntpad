#include "precomp.h"
#pragma hdrstop


//
// Constants
//
#if defined(_IA64_)
    #define TOASTVA_PLATFORM_SUBDIRECTORY L"ia64"
#elif defined(_X86_)
    #define TOASTVA_PLATFORM_SUBDIRECTORY L"i386"
#else
#error Unsupported platform
#endif

#define TOASTVA_PLATFORM_SUBDIRECTORY_SIZE (sizeof(TOASTVA_PLATFORM_SUBDIRECTORY) / sizeof(WCHAR))

//
// Globals
//
HINSTANCE g_hInstance;


//
// Implementation
//

INT
WINAPI
WinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE hPrevInstance,
    IN LPSTR     lpszCmdLine,
    IN INT       nCmdShow
    )
{
    LPWSTR *ArgList;
    INT NumArgs;
    WCHAR MediaRootDirectory[MAX_PATH];
    LPWSTR FileNamePart;
    DWORD DirPathLength;
    
    g_hInstance = hInstance;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszCmdLine);
    UNREFERENCED_PARAMETER(nCmdShow);

    InitCommonControls();
    //
    // Retrieve the full directory path from which our setup program was
    // invoked.
    //
    ArgList = CommandLineToArgvW(GetCommandLine(), &NumArgs);

    if(ArgList && (NumArgs >= 1)) {
        
        DirPathLength = GetFullPathName(ArgList[0], 
                                        MAX_PATH, 
                                        MediaRootDirectory,
                                        &FileNamePart
                                       );

        if(DirPathLength >= MAX_PATH) {
            //
            // The directory is too large for our buffer.  Set our directory
            // path length to zero so we'll simply bail out in this rare case.
            //
            DirPathLength = 0;
        }

        if(DirPathLength) {
            //
            // Strip the filename off the path.
            //
            *FileNamePart = L'\0';

            DirPathLength = (DWORD)(FileNamePart - MediaRootDirectory);
        }

    } else {
        //
        // For some reason, we couldn't get the command line arguments that
        // were used when invoking our setup app.  Assume current directory
        // instead.
        //
        DirPathLength = GetCurrentDirectory(MAX_PATH, MediaRootDirectory);

        if(DirPathLength >= MAX_PATH) {
            //
            // The current directory is too large for our buffer.  Set our
            // directory path length to zero so we'll simply bail out in this
            // rare case.
            //
            DirPathLength = 0;
        }
         
        if(DirPathLength) {
            //
            // Ensure that path ends in a path separator character.
            //
            if((MediaRootDirectory[DirPathLength-1] != L'\\') && 
               (MediaRootDirectory[DirPathLength-1] != L'/')) 
            {
                MediaRootDirectory[DirPathLength++] = L'\\';

                if(DirPathLength < MAX_PATH) {
                    MediaRootDirectory[DirPathLength] = L'\0';
                } else {
                    //
                    // Not enough room in buffer to add path separator char
                    //
                    DirPathLength = 0;
                }
            }
        }
    }

    if(ArgList) {
        GlobalFree(ArgList);
    }

    if(!DirPathLength) {
        //
        // Couldn't figure out what the root directory of our installation
        // media was.  Bail out.
        //
        return 0;
    }

    DoValueAddWizard(MediaRootDirectory);

    return 0;
}

