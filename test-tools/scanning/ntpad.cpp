#include <windows.h>
#include <string.h>
#include "NTPAD.h"

HANDLE h;
HANDLE g_hThread;
HANDLE g_hEndEvent;

HINSTANCE  g_hInstance ;
HWND       g_hMainWnd ;
CPSX psx;

HWND InitWindow( void ) ;
DWORD WINAPI ShowStatus(LPVOID);
LRESULT CALLBACK MainWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, 
                              LPARAM lParam ) ;

int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                    LPSTR lpCmdLine, int nCmdShow )
{
   DWORD dwID;

   MSG msg ;

   g_hInstance = hInstance ;

   g_hMainWnd = InitWindow() ;  
 
   if (! g_hMainWnd) return 0;

    int started = 0;
    SC_HANDLE h2;
    SC_HANDLE h3;
    LPSERVICE_STATUS servicio = 0;
    TCHAR buff[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, buff);
	strcat(buff, "\\giveio.sys");
     h2 = OpenSCManagerA(NULL, NULL, GENERIC_WRITE);
    if (h2 != NULL)
    {
      h3 = CreateService(h2, "GIVEIO", "GIVEIO", SERVICE_ALL_ACCESS,
                     SERVICE_KERNEL_DRIVER, SERVICE_DEMAND_START,
                     SERVICE_ERROR_IGNORE,
                     buff,
                     NULL, NULL, NULL, NULL, NULL);
      if(h3 != NULL)
      {
        if (StartService(h3, 0, NULL))
          started = 1;
        else
        {
          DeleteService(h3);
          CloseServiceHandle(h3);
          CloseServiceHandle(h2);
        }
      } else {
        if (GetLastError() == ERROR_SERVICE_EXISTS)
           {
             h3 = OpenService(h2, "GIVEIO", SERVICE_ALL_ACCESS);
             if ( h3 != NULL)
             {
               if (StartService(h3, 0, NULL))
                 started = 1;
               else
               {
                 DeleteService(h3);
                 CloseServiceHandle(h3);
                 CloseServiceHandle(h2);
               }
             }
           }
        CloseServiceHandle(h2);
      }
    }
    h = CreateFile("\\\\.\\giveio", GENERIC_READ, 0, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if(h == INVALID_HANDLE_VALUE) {
        
		MessageBox(g_hMainWnd, "No se encuentra el driver GIVEIO", "Error", MB_OK + MB_ICONERROR);

        if (started)
        {
         ControlService(h3, SERVICE_CONTROL_STOP, servicio);
         DeleteService(h3);
         CloseServiceHandle(h3);
         CloseServiceHandle(h2);
        };
        return -1;
    }
   
   	    psx.Init(0x378, 2, 4, 1);    

   g_hEndEvent = CreateEvent( NULL, TRUE, FALSE, NULL ) ;
   g_hThread = CreateThread( NULL, 0, ShowStatus, NULL, 0, &dwID) ;
   
   while( GetMessage( &msg, NULL, 0, 0 ) )
   {
     TranslateMessage( &msg ) ;
      DispatchMessage( &msg ) ;
   }

   SetEvent(g_hEndEvent) ;
   WaitForSingleObject( g_hThread, INFINITE ) ;
   CloseHandle(g_hThread);	
   CloseHandle(h);
        if (started)
        {
         ControlService(h3, SERVICE_CONTROL_STOP, servicio);
         DeleteService(h3);
         CloseServiceHandle(h3);
         CloseServiceHandle(h2);
        };

        return 0;
}

HWND InitWindow( void )
{
   WNDCLASSEX  wndclass ;
   HWND        hwnd ;

   wndclass.cbSize = sizeof( wndclass ) ; 
   wndclass.style = CS_BYTEALIGNCLIENT ; 
   wndclass.lpfnWndProc = MainWndProc ; 
   wndclass.cbClsExtra = 0 ; 
   wndclass.cbWndExtra = 0 ; 
   wndclass.hInstance = g_hInstance ;
   wndclass.hIcon = NULL;
   wndclass.hCursor = LoadCursor(NULL,IDC_ARROW) ; 
   wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH) ; 
   wndclass.lpszMenuName = NULL ; 
   wndclass.lpszClassName = "Demo Wnd Class" ; 
   wndclass.hIconSm = NULL ; 

   if( !RegisterClassEx( &wndclass ) )
   {
      return NULL ;
   }

   hwnd = CreateWindowEx( WS_EX_APPWINDOW, "Demo Wnd Class", 
                          "NTPAD - Testeo", WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          CW_USEDEFAULT, CW_USEDEFAULT,
                          NULL, NULL, g_hInstance, NULL) ;

   if( !hwnd )
      return NULL ;

   UpdateWindow( hwnd ) ;
   ShowWindow( hwnd, SW_SHOW ) ;

   return hwnd ;
}

DWORD WINAPI ShowStatus(LPVOID) {

	    while(WaitForSingleObject(g_hEndEvent, 1000/32) != WAIT_OBJECT_0)
        {	
    		  psx.ScanPSX();
			  psx.motorc = 0;
			  if(psx.l1) psx.motorg == 255 ? psx.motorg = 30: psx.motorg++;
			  if(psx.r1) psx.motorc = 1;
			if(psx.l2 & psx.r2) break;
 
			InvalidateRect( g_hMainWnd, NULL, TRUE ) ;
		};

        return 0;
}
void MakeReport(char* buffer)
{

      wsprintf(buffer,"X Izq:  %d\n"
               "Y Izq:  %d\n"
               "X Der:  %d\n"
               "Y Der:  %d\n"
			   "Arriba: %d\n"
			   "Abajo: %d\n"
			   "Izquierda: %d\n"
			   "Derecha: %d\n"
               "Cuadrado: %i\n"
			   "Cruz: %i\n"
			   "Circulo: %i\n"
			   "Triangulo: %i\n"
			   "Select: %i\n"
			   "Start: %i\n"
			   "L1: %i L2: %i L3:%i\n"
			   "R1: %i R2: %i R3:%i\n"
			   "Motor derecho %i\n"
			   "Fuerza del motor izquierdo: %i\n",
               psx.lx,psx.ly,psx.rx,psx.ry,
			   psx.up, psx.dn, psx.lf, psx.rt,
			   psx.box, psx.cross, psx.circle, psx.triangle,
			   psx.select, psx.start,
			   psx.l1, psx.l2, psx.l3,
			   psx.r1, psx.r2, psx.r3,
			   psx.motorc, psx.motorg);

//   wsprintf(buffer, "El código devuelto es (the returned code is): 0x%x", psx.type);
}


LRESULT CALLBACK MainWndProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
   PAINTSTRUCT ps ;
   HDC         hdc ;
   char  szBuffer[256] ;
   RECT  rect ;

   switch( uMsg )
   {
   case WM_CREATE:
      break ;
   case WM_PAINT:
      hdc = BeginPaint( hwnd, &ps ) ;
      GetClientRect( g_hMainWnd, &rect ) ;
      MakeReport(szBuffer);   
	  DrawText( hdc, szBuffer, -1, &rect, DT_LEFT|DT_NOCLIP) ;
	  EndPaint( hwnd, &ps ) ;
      break ;
   case WM_CLOSE:
      DestroyWindow( hwnd ) ;
      break ;
   case WM_DESTROY:
      PostQuitMessage(0) ;
   }
   return DefWindowProc( hwnd, uMsg, wParam, lParam ) ;
}
