#include <windows.h>
#include <ddraw.h>

WORD SCREEN_WIDTH = 640;
WORD SCREEN_HEIGHT = 480;
BYTE SCREEN_BPP = 16; //Can be 32, but 8-bit gfx doesn't need it

HWND parentHWND;

HINSTANCE hInst;
LPDIRECTDRAW7 lpDD = NULL;
LPDIRECTDRAWSURFACE7 lpPrimary = NULL;
LPDIRECTDRAWSURFACE7 lpBack = NULL;
BOOL bRunning = TRUE;

//Setup Mouse Detection
POINT ptCurrentMouse = {NULL, NULL};
WORD centerX = (SCREEN_WIDTH >> 1); //Dividing by 2, bit-shifting is faster!
WORD centerY = (SCREEN_HEIGHT >> 1);

//Setup Preview Mode Vars
RECT rcClient = {0, 0, 0, 0};
POINT clientWindowPt = {0, 0};

//Setup Multi-threaded Debug Messages
DWORD WINAPI DebugMessageThread(LPVOID lpParam)
{
	MessageBox(NULL, (LPCSTR)lpParam, "Debug", MB_OK | MB_TOPMOST | MB_SYSTEMMODAL);
	return 0;
}

void ShowDebugMessage(const char* message)
{
	DWORD threadId;

	HANDLE hThread = CreateThread(NULL, 0, DebugMessageThread, (LPVOID)message,
								  0, &threadId);

	if (hThread)
	{
		WaitForSingleObject(hThread, 10000);
		CloseHandle(hThread);
	}
}

//DirectDraw Init
BOOL InitDirectDraw_Fullscreen(HWND hWnd)
{
	if (DirectDrawCreateEx(NULL, (void**)&lpDD, IID_IDirectDraw7, NULL) != DD_OK)
	{
		ShowDebugMessage("DirectDrawCreateEx Failed!");
		return false;	
	}

	if (!SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE))
	{
		ShowDebugMessage("SetWindowPos Failed!");
		return false;
	}

	if (!SetActiveWindow(hWnd))
	{
		ShowDebugMessage("SetActiveWindow Failed!");
		return false;
	}

	if (!SetFocus(hWnd))
	{
		ShowDebugMessage("SetFocus Failed!");
		return false;
	}

	if (lpDD->SetCooperativeLevel(hWnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE) != DD_OK)
	{
		ShowDebugMessage("SetCooperativeLevel Failed!");
		return false;
	}

	if (lpDD->SetDisplayMode(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, 0, 0) != DD_OK)
	{
		ShowDebugMessage("SetDisplayMode Failed!");
		return false;
	}
	
	DDSURFACEDESC2 ddsd = {sizeof(ddsd)};
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	if (lpDD->CreateSurface(&ddsd, &lpPrimary, NULL) != DD_OK)
	{
		ShowDebugMessage("CreateSurface Failed!");
		return false;
	}

	return true;
}

BOOL InitDirectDraw_Preview(HWND hWnd)
{
	if (DirectDrawCreateEx(NULL, (void**)&lpDD, IID_IDirectDraw7, NULL) != DD_OK)
	{
		ShowDebugMessage("DirectDrawCreateEx Failed!");
		return false;	
	}

	if (lpDD->SetCooperativeLevel(hWnd, DDSCL_NORMAL) != DD_OK)
	{
		ShowDebugMessage("SetCooperativeLevel Failed!");
		return false;
	}

	DDSURFACEDESC2 ddsd = {sizeof(ddsd)};
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	if (lpDD->CreateSurface(&ddsd, &lpPrimary, NULL) != DD_OK)
	{
		ShowDebugMessage("CreateSurface Failed!");
		return false;
	}
	
	return true;
}

//Cleanup
void CleanupDirectDraw()
{
	if (lpPrimary)
	{
		lpPrimary->Release();
		lpPrimary = NULL;
	}

	if (lpDD)
	{
		lpDD->Release();
		lpDD = NULL;
	}
}

//Render loop
void RenderFrame(HWND hWnd)
{
	if (!lpPrimary)
	{
		return;
	}

	parentHWND = GetParent(hWnd);
	GetClientRect(parentHWND, &rcClient);

	clientWindowPt.x = rcClient.left;
	clientWindowPt.y = rcClient.top;

	ClientToScreen(parentHWND, &clientWindowPt);

	DDBLTFX bltfx = {sizeof(bltfx)};
	bltfx.dwFillColor = (BYTE)(rand() % 256);
	
	if (rcClient.right == 0 && rcClient.bottom == 0) //Fullscreen Render
	{
		lpPrimary->Blt(NULL, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx);
	}
	else //Windowed Mode Render
	{
		OffsetRect(&rcClient, clientWindowPt.x, clientWindowPt.y);
		lpPrimary->Blt(&rcClient, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &bltfx);	
	}
	
	Sleep(100);
}

//Screensaver window procedure
LRESULT CALLBACK ScreenSaverProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		case WM_CREATE:

			break;

		case WM_SIZE: //Adjust rendering size for preview window
		
			break;
	
		case WM_PAINT:
			
			RenderFrame(hWnd);
			break;
			
		case WM_MOUSEMOVE:

			if (GetParent(hWnd) == NULL) // Exit if in preview
			{
				//Check if the mouse has moved

				if (!GetCursorPos(&ptCurrentMouse))
				{
					throw;
				}

				if (ptCurrentMouse.x != centerX || ptCurrentMouse.y != centerY)
				{
					bRunning = FALSE;
					PostQuitMessage(0);
				} //This will throw a warning about comparing signed and unsigned types
			}	  //Ignore the warning because centerX and centerY can never be negative

			break;

		case WM_KEYDOWN:

			if (GetParent(hWnd) == NULL) //Keydown escape if not in preview
			{
				//Exit on any keypress
				bRunning = FALSE;
				PostQuitMessage(0);
			}

				break;

		case WM_SYSKEYDOWN:

			if (GetParent(hWnd) == NULL) //ALT+KEY escape if not in preview
			{
				//Handles system keys like ALT+KEY
				bRunning = FALSE;
				PostQuitMessage(0);
			}

				break;

		case WM_DESTROY:

			CleanupDirectDraw();
			PostQuitMessage(0);
			break;

		case WM_CLOSE:

			bRunning = FALSE;
			DestroyWindow(hWnd);
			break;

		default:

			return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	return 0;
}

MSG PreviewModeDraw(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	HWND hParent = (HWND)atoi(&lpCmdLine[3]); //Extract parent HWND

	if(!IsWindow(hParent))
	{
		hParent = NULL;
		throw;
	}

    WNDCLASS wc = 
    {
        CS_HREDRAW | CS_VREDRAW,
        ScreenSaverProc,
        0,
        0,
        hInstance,
        NULL,
        LoadCursor(NULL, IDC_ARROW),
        NULL,
        NULL,
        "DDrawPreview"
	};

    RegisterClass(&wc);

	GetWindowRect(hParent, &rcClient); // Get preview box size and coords

    HWND hWnd = CreateWindow("DDrawPreview", "TGL Preview", 
                            WS_CHILD | WS_VISIBLE, NULL, NULL, 1, 
                            1, hParent, NULL, hInstance, NULL);

    if (!hWnd)
	{
		throw;
	}

	if (!InitDirectDraw_Preview(hWnd))
	{
		throw;
	}

    MSG msg;

    while (bRunning)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                PostQuitMessage(0);
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
		}
	}
            
	return msg;
}

//Entry point
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{	
	MSG msg;

	//Detect command-line params
	if (lpCmdLine[1] == 112 || lpCmdLine[1] == 80) //preview mode
    {
		msg = PreviewModeDraw(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
	    
        return (short)msg.wParam;
    }

    else if (lpCmdLine[1] == 'c' || lpCmdLine[1] == 'C') //Configure mode
    {
        MessageBox(NULL, "No configuration available.", "Screensaver", MB_OK);
        return 0;
    }
	else if (lpCmdLine[1] == 115 || lpCmdLine[1] == 83
		     || lpCmdLine == NULL || lpCmdLine[0] == NULL) //Normal mode
	{
		WNDCLASS wc = 
        {
           CS_HREDRAW | CS_VREDRAW,
           ScreenSaverProc,
           0,
           0,
           hInstance,
           NULL,
           LoadCursor(NULL, IDC_ARROW),
           NULL,
           NULL,
           "DDrawScreenSaver"
		};

        RegisterClass(&wc);

        //Create a window inside the preview box
        HWND hWnd = CreateWindow("DDrawScreenSaver", "TGL Screensaver", 
                                WS_POPUP | WS_VISIBLE, 0, 0, SCREEN_WIDTH, 
                                SCREEN_HEIGHT, NULL, NULL, hInstance, NULL);
		
		if (!hWnd)
		{
			throw;
		}

		InitDirectDraw_Fullscreen(hWnd);
            
        ShowCursor(FALSE);

        while (bRunning)
        {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    PostQuitMessage(0);
                }

                TranslateMessage(&msg);
                DispatchMessage(&msg);
			}
		}
            
        ShowCursor(TRUE);
        return (short)msg.wParam;
	}
	else //ERROR
	{
		throw;
	}

	return (short)msg.wParam;
}
