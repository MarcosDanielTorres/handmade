#include <windows.h>
#include <stdint.h>

#define global_variable	 	static
#define internal		 	static
#define local_persist		static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;


global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void RenderWeirdGradient(int xOffset, int yOffset){
	for (int y = 0; y < BitmapHeight; y++) {
		for (int x = 0; x < BitmapWidth; x++) {
			u8 *Pixel = ((u8 *)BitmapMemory + (x*BytesPerPixel) + (y*BitmapWidth*BytesPerPixel));
			*Pixel = x + xOffset; 			// blue
			*(Pixel + 1) = y + yOffset;		// green
			*(Pixel + 2) = 0; 				// red
			*(Pixel + 3) = 0;				// padding
		}
	}
}


internal void Win32ResizeDIBSection(int w, int h) {
	if (BitmapMemory) {
		VirtualFree(BitmapMemory, 0, MEM_RELEASE);
	}

	BitmapWidth = w;
	BitmapHeight = h;

	BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
	BitmapInfo.bmiHeader.biWidth = w;
	BitmapInfo.bmiHeader.biHeight = -h;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = w*h*BytesPerPixel;
	BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT* WindowRect, int X, int Y, int Width, int Height) {
	int WindowWidth = WindowRect->right - WindowRect->left;
	int WindowHeight = WindowRect->bottom - WindowRect->top;

	StretchDIBits(
		DeviceContext,
		// First do the entire window (use the bitmaps size) instead of a subregion
		// X, Y, Width, Height,
		// X, Y, Width, Height,
		0, 0, BitmapWidth, BitmapHeight,
		0, 0, WindowWidth, WindowHeight,
		BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY
	);
}

LRESULT CALLBACK Win32MainWindowCallback( HWND WinHandle, UINT Message, WPARAM WParam, LPARAM LParam) {
	LRESULT Result = 0;

	switch(Message) {
		case WM_SIZE: {
			RECT ClientRect;
			GetClientRect(WinHandle, &ClientRect);
			int w = ClientRect.right - ClientRect.left;
			int h = ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(w, h);
			OutputDebugStringA("WM_SIZE\n");
		} break;

		case WM_DESTROY: {
			OutputDebugStringA("WM_DESTROY\n");
		} break;

		case WM_CLOSE: {
			Running = false;
			OutputDebugStringA("WM_CLOSE\n");
		} break;

		case WM_ACTIVATEAPP: {
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;

		case WM_PAINT: {
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(WinHandle, &Paint);
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			RECT ClientRect;
			GetClientRect(WinHandle, &ClientRect);
			Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);

			EndPaint(WinHandle, &Paint);
		} break;

		default: {
			Result = DefWindowProc(WinHandle, Message, WParam, LParam);
		} break;
	}

	return Result;
}


int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode) {
	WNDCLASS WindowClass = {};

	WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	// WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if (RegisterClass(&WindowClass)) {
		HWND Window = CreateWindowEx(
			0,
			WindowClass.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0
		);
		if (Window) {
			Running = true;
			int xOffset = 0;
			int yOffset = 0;
			while(Running) {
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
					if (Message.message == WM_QUIT) {
						Running = false;
					}
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				} // PeekMessage instead of GetMessage (non-blocking)
				RenderWeirdGradient(xOffset, yOffset);

				HDC DeviceContext = GetDC(Window);
				RECT ClientRect;
				GetClientRect(Window, &ClientRect);
				int Width = ClientRect.right - ClientRect.left;
				int Height = ClientRect.bottom - ClientRect.top;

				Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, Width, Height);
				ReleaseDC(Window, DeviceContext);
				xOffset++;
			}
		} else {	
		// TODO Logging
		}
	}else{
		// TODO Logging
	}
}