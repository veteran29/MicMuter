#ifdef _DEBUG
// headers for console
	#include <io.h>
	#include <fcntl.h>
// memory leak detection stuff
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
	#ifndef DBG_NEW
		#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )      
		#define new DBG_NEW   
	#endif
#endif

#include "stdafx.h"
#include "resource.h"
#include "EndpointManagement.h"

#define MAX 100
#define	WM_USER_SHELLICON WM_USER + 1

// global Variables:
BOOL bDisable = FALSE;							// keep microphone state
HINSTANCE hInst;	// current instance
NOTIFYICONDATA nid;
HMENU hPopMenu;
TCHAR szTitle[MAX]= _T("Mic Muter");					// The title bar text
TCHAR szWindowClass[MAX]= _T("Mic Muter");;		// the main window class name
TCHAR szApplicationToolTip[MAX]=_T("Mic Muter");	    // the main window class name

// forward declarations of functions
ATOM				CustomRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

//
EndpointManage *endpointHandler;
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	#ifdef _DEBUG
	//console for debbuging
		AllocConsole();

		HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
		int hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
		FILE* hf_out = _fdopen(hCrt, "w");
		setvbuf(hf_out, NULL, _IONBF, 1);
		*stdout = *hf_out;

		HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
		hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
		FILE* hf_in = _fdopen(hCrt, "r");
		setvbuf(hf_in, NULL, _IONBF, 128);
		*stdin = *hf_in;

		SetConsoleTitle(L"Debug Output of Micmuter");

		_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	#endif

	endpointHandler= new EndpointManage;

	MSG msg;

	CustomRegisterClass(hInstance);

	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	// main message loop
	while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	delete endpointHandler;
    return (int) msg.wParam;
}

ATOM CustomRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
   HICON hMainIcon;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }

   hMainIcon = LoadIcon(NULL, IDI_APPLICATION);//LoadIcon(hInstance,(LPCTSTR)MAKEINTRESOURCE(IDI_SYSTRAYDEMO)); 
   
   nid.cbSize = sizeof(NOTIFYICONDATA); // sizeof the struct in bytes 
   nid.hWnd = (HWND) hWnd;              //handle of the window which will process this app. messages 
   nid.uID = 100;           //ID of the icon that willl appear in the system tray 
   nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP; //ORing of all the flags 
   nid.hIcon = hMainIcon; // handle of the Icon to be displayed, obtained from LoadIcon 
   nid.uCallbackMessage = WM_USER_SHELLICON; 
   wcscpy_s(nid.szTip, L"Microphone");
   Shell_NotifyIcon(NIM_ADD, &nid); 

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	POINT lpClickPoint;

    switch (message)
    {
    case WM_CREATE:
         break;

    case WM_USER_SHELLICON:
         switch(lParam)
         {
         case WM_LBUTTONDBLCLK:
			 {
                 MessageBox(NULL, L"Tray icon double clicked!", L"clicked", MB_OK);
			 }
             break;

		case WM_RBUTTONDOWN: 
			{
				GetCursorPos(&lpClickPoint);
				hPopMenu = CreatePopupMenu();				
				if ( bDisable == TRUE )
				{
					InsertMenu(hPopMenu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,(UINT)1,_T("Enable"));									
				}
				else 
				{
					InsertMenu(hPopMenu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,(UINT)2,_T("Disable"));				
				}
				InsertMenu(hPopMenu,0xFFFFFFFF,MF_SEPARATOR,(UINT)"SEP",_T("SEP"));				
				InsertMenu(hPopMenu,0xFFFFFFFF,MF_BYPOSITION|MF_STRING,(UINT)3,_T("Exit"));
									
				SetForegroundWindow(hWnd);
				TrackPopupMenu(hPopMenu,TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_BOTTOMALIGN,lpClickPoint.x, lpClickPoint.y,0,hWnd,NULL);
				return TRUE; 
			}
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		 }
	case WM_COMMAND:
	{
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		switch (wmId)
		{

			case (UINT)1:
				{
					endpointHandler->mute_Endpoint();
					bDisable=FALSE;
					PlaySound(MAKEINTRESOURCE(IDR_ON),GetModuleHandle(NULL),SND_RESOURCE); 
				}
				break;
			case (UINT)2:
				{
					endpointHandler->mute_Endpoint();
					bDisable=TRUE;
					PlaySound(MAKEINTRESOURCE(IDR_OFF),GetModuleHandle(NULL),SND_RESOURCE);
				}
				break;
			case (UINT)3:
				CoUninitialize();
				Shell_NotifyIcon(NIM_DELETE,&nid);
				DestroyWindow(hWnd);
				PostQuitMessage(0);
				break;
			default:
				return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}