//===========================================================================
// BUTTON.CPP
//
// Button Custom control
//
// Functions:
//    SetButtonState()
//    ButtonWndProc()
//    RegisterCustomButtonClass()
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

#include "cplsvr1.h"	   // for ghInst

#include "resrc1.h"

// Colour of text for buttons!
#define TEXT_COLOUR  RGB(000,000,000)
#define TEXT_OFF  RGB(220,220,220)

// Local globals!
static HBITMAP hIconArray[4] = {NULL, NULL, NULL, NULL};

void SetButtonState(HWND hDlg, BYTE nButton, BOOL bState )
{
   if ((nButton < 0) || (nButton > 32))
   {
#ifdef _DEBUG
      OutputDebugString(TEXT("Button.cpp: SetState: nButton out of range!\n"));
#endif
      return;
   }

   HWND hCtrl = GetDlgItem(hDlg, IDC_TESTJOYBTNICON + nButton);
   assert (hCtrl);

   // Set the Extra Info
   if(GetWindowLongPtr(hCtrl, GWLP_USERDATA) == bState)
      return;
   SetWindowLongPtr(hCtrl, GWLP_USERDATA, (bState) ? 1 : 0);


   RedrawWindow(hCtrl, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ERASENOW);
}                                                    

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    FUNCTION :  ButtonWndProc
//    REMARKS  :  The callback function for the CustomButton Window.
//				      
//    PARAMS   :  The usual callback funcs for message handling
//
//    RETURNS  :  LRESULT - Depends on the message
//    CALLS    :  
//    NOTES    :
//                

LRESULT CALLBACK ButtonWndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        int modo = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        TRACKMOUSEEVENT t;
	switch(iMsg)
	{
         case WM_USER+1:
           SetWindowLongPtr(hWnd, GWLP_USERDATA, 2);
           break;
         case WM_CREATE:
           // load the up and down states!
           if(!hIconArray[0])
           {
                 hIconArray[0] = LoadBitmap(ghInst, MAKEINTRESOURCE(IDB_OFF));
                 assert (hIconArray[0]);
           }

           if(!hIconArray[1])
           {
                 hIconArray[1] = LoadBitmap(ghInst, MAKEINTRESOURCE(IDB_ON));
                 assert (hIconArray[1]);
           }
           if(!hIconArray[2])
           {
                 hIconArray[2] = LoadBitmap(ghInst, MAKEINTRESOURCE(IDB_LOFF));
                 assert (hIconArray[1]);
           }
           if(!hIconArray[3])
           {
                 hIconArray[3] = LoadBitmap(ghInst, MAKEINTRESOURCE(IDB_LON));
                 assert (hIconArray[1]);
           }
           return FALSE;

         case WM_DESTROY:
           // Delete the buttons...
           DeleteObject(hIconArray[0]);
           hIconArray[0] = NULL;
           DeleteObject(hIconArray[1]);
           hIconArray[1] = NULL;
           DeleteObject(hIconArray[2]);
           hIconArray[2] = NULL;
           DeleteObject(hIconArray[3]);
           hIconArray[3] = NULL;
           return FALSE;

         case WM_PAINT:
         {
         PAINTSTRUCT *pps = new (PAINTSTRUCT);
			assert (pps);
         
         HDC hDC = BeginPaint(hWnd, pps);
         HDC hbDC = CreateCompatibleDC(hDC);
         BITMAP bmp;
         SelectObject(hbDC, hIconArray[modo]);
         GetObject(hIconArray[modo], sizeof(BITMAP), &bmp);
         BitBlt(hDC, 0,0,bmp.bmWidth, bmp.bmHeight, hbDC, 0,0, SRCCOPY);
         DeleteDC(hbDC);
         
         static TCHAR tsz[20];

         // Get the Text
         GetWindowText(hWnd, tsz, sizeof(tsz)/sizeof(TCHAR));         
   
         // Prepare the DC for the text
         SetBkMode   (hDC, TRANSPARENT);
         if((modo == 1) | (modo == 3))
               SetTextColor(hDC, TEXT_COLOUR);
         else
               SetTextColor(hDC, TEXT_OFF);
                 

         // given the size of this RECT, it's faster 
         // to blit the whole the whole thing...
         pps->rcPaint.bottom += 3;
         pps->rcPaint.right  += 3;
         // Draw the Number                        
         DrawText (hDC, (LPCTSTR)tsz, lstrlen(tsz), &pps->rcPaint, DT_VCENTER|DT_CENTER|DT_NOPREFIX|DT_SINGLELINE|DT_NOCLIP);
         SetBkMode(hDC, OPAQUE);
			EndPaint (hWnd, pps);

         if (pps)
            delete (pps);
         }
         return FALSE;
        case WM_MOUSELEAVE:           
           if(modo==3)
           {
                   SetWindowLongPtr(hWnd, GWLP_USERDATA, 2);
                   InvalidateRect(hWnd, NULL, 0);
           }
           break;
        case WM_MOUSEMOVE:
           if(modo == 2)
           {
             SetWindowLongPtr(hWnd, GWLP_USERDATA, 3);
             InvalidateRect(hWnd, NULL, 0);
             TRACKMOUSEEVENT t;
             t.cbSize = sizeof(t);
             t.dwFlags  = TME_LEAVE;
             t.hwndTrack = hWnd;
             t.dwHoverTime = 0;
             TrackMouseEvent(&t);
           }
           break;          
         case WM_LBUTTONDOWN:
           if(modo>1)
              SendMessage(GetParent(hWnd), WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(hWnd), BN_CLICKED), (LPARAM)hWnd);
           break;
         default:
             return DefWindowProc(hWnd, iMsg,wParam, lParam);
	}
   return FALSE;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//    FUNCTION :  RegisterCustomButtonClass
//    REMARKS  :  Registers the Custom Button control window.
//				      
//    PARAMS   :  hInstance - Used for the call to RegisterClassEx
//
//    RETURNS  :  TRUE - if successfully registered
//                FALSE - failed to register
//    CALLS    :  RegisterClassEx
//    NOTES    :
//

extern BOOL RegisterCustomButtonClass()
{

   WNDCLASSEX     *pCustCtrlClass = new (WNDCLASSEX);
   assert (pCustCtrlClass);

   pCustCtrlClass->cbSize         = sizeof(WNDCLASSEX);
   pCustCtrlClass->style          = CS_HREDRAW | CS_VREDRAW;
   pCustCtrlClass->lpfnWndProc    = ButtonWndProc;
   pCustCtrlClass->cbClsExtra     = 0;
   pCustCtrlClass->cbWndExtra     = 0;
   pCustCtrlClass->hInstance      = ghInst;
   pCustCtrlClass->hIcon          = NULL;
   pCustCtrlClass->hCursor        = LoadCursor(NULL, IDC_ARROW);
   pCustCtrlClass->hbrBackground  = (HBRUSH) (COLOR_BTNFACE + 1);
   pCustCtrlClass->lpszMenuName   = NULL;
   pCustCtrlClass->lpszClassName  = TEXT("TESTBUTTON");
   pCustCtrlClass->hIconSm        = NULL; 

   ATOM aRet = RegisterClassEx( pCustCtrlClass );

   if (pCustCtrlClass)
      delete (pCustCtrlClass);

   return ( aRet ) ? TRUE : FALSE;
}


#pragma pack(push, 1)

typedef struct
{
    BYTE    IDLength;
    BYTE    ColorMapType;
    BYTE    ImageType;
    WORD    CMapStart;
    WORD    CMapLength;
    BYTE    CMapDepth;
    WORD    XOffset;
    WORD    YOffset;
    WORD    Width;
    WORD    Height;
    BYTE    PixelDepth;
    BYTE    ImageDescriptor;
}   TGA_Header;

#pragma pack(pop)

HBITMAP Load32bppTga(const TCHAR * pFileName, bool bPreMultiply)
{

    HRSRC hR;
    HGLOBAL hRG;
    char* datos;
    TGA_Header header;
    hR =  FindResource(ghInst, pFileName, RT_RCDATA);
    hRG = LoadResource(ghInst, hR);
    datos = (char*)LockResource(hRG);
    memcpy(&header, datos, sizeof(header));

    datos += sizeof(header);

    BITMAPINFO bmp = { { sizeof(BITMAPINFOHEADER), header.Width, header.Height, 1, 32 } };

    void * pBits = NULL;

    HBITMAP hBmp = CreateDIBSection(NULL, & bmp, DIB_RGB_COLORS, & pBits, NULL, NULL);

    if ( hBmp==NULL )
    {
        return NULL;
    }
    memcpy(pBits, datos, header.Width * header.Height * 4);

    if ( bPreMultiply )
    {
        for (int y=0; y<header.Height; y++)
        {
            BYTE * pPixel = (BYTE *) pBits + header.Width * 4 * y;

            for (int x=0; x<header.Width; x++)
            {
                pPixel[0] = pPixel[0] * pPixel[3] / 255; 
                pPixel[1] = pPixel[1] * pPixel[3] / 255; 
                pPixel[2] = pPixel[2] * pPixel[3] / 255; 

                pPixel += 4;
            }
        }
    }

    return hBmp;
}

void AlphaDraw(HDC hDC, int x, int y, int width, int height, HBITMAP hBmp)
{
    HDC     hMemDC = CreateCompatibleDC(hDC);
    HGDIOBJ hOld   = SelectObject(hMemDC, hBmp);
 
    BLENDFUNCTION pixelblend = { AC_SRC_OVER, 0, 255, 1 };

    AlphaBlend(hDC, x, y+height, width, height, hMemDC, 0, 0, width, height, pixelblend);

    SelectObject(hMemDC, hOld);
    DeleteObject(hMemDC);
}
