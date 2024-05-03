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

struct win32_offscreen_buffer {
	u16 Width;
	u16 Height;
	void *Memory;
	BITMAPINFO Info;
	u8 BytesPerPixel;
};

struct win32_window_dimension {
	u16 Width;
	u16 Height;
};


u8 BytesPerPixel = 4;
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;


internal void RenderWeirdGradient(win32_offscreen_buffer Buffer, int xOffset, int yOffset){
	// Considerations
	// Even though at the moment GlobalBackBuffer is global. In the future I may use different buffers.
	// Buffer passed by copy because it doesn't matter. I'm not changing the buffer itself, just the memory it points to. So I think-
	// passing by copy or by reference is the same.

	for (int y = 0; y < Buffer.Height; y++) {
		for (int x = 0; x < Buffer.Width; x++) {
			u8 *Pixel = ((u8 *)Buffer.Memory + (x*Buffer.BytesPerPixel) + (y*Buffer.Width*Buffer.BytesPerPixel));
			// 0x xx RR GG BB -> 0xBB GG RR xx
			*Pixel = x + xOffset; 			// blue
			*(Pixel + 1) = y + yOffset;		// green
			*(Pixel + 2) = 0; 				// red
			*(Pixel + 3) = 0;				// padding
		}
	}
}


internal win32_window_dimension Win32GetWindowDimension(HWND Window) {
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return Result;
}

internal void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height) {
	if (Buffer->Memory) {
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Width;
	Buffer->Info.bmiHeader.biHeight = -Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	Buffer->BytesPerPixel = BytesPerPixel;
	int BitmapMemorySize = Width * Height * BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32DisplayBufferWindow(
		HDC DeviceContext,
		int destWidth, int destHeight,
		win32_offscreen_buffer Buffer,
		int X, int Y, int Width, int Height) {

	// TODO Aspect ratio correction
	StretchDIBits(
		DeviceContext,
		// First do the entire window (use the bitmaps size) instead of a subregion
		// X, Y, Width, Height,
		// X, Y, Width, Height,
		0, 0, destWidth, destHeight,
		0, 0, Buffer.Width, Buffer.Height,
		Buffer.Memory, &Buffer.Info,
		DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback( HWND WinHandle, UINT Message, WPARAM WParam, LPARAM LParam) {
	LRESULT Result = 0;

	switch(Message) {
		case WM_SIZE: {
			OutputDebugStringA("WM_SIZE\n");
		} break;

		case WM_DESTROY: {
			OutputDebugStringA("WM_DESTROY\n");
		} break;

		case WM_CLOSE: {
			GlobalRunning = false;
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

			win32_window_dimension Dimension = Win32GetWindowDimension(WinHandle);
			Win32DisplayBufferWindow(DeviceContext, 
				Dimension.Width, Dimension.Height,
				GlobalBackBuffer, 
				X, Y, Width, Height
			);

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

	GlobalBackBuffer = {};
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW|CS_OWNDC;
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
			HDC DeviceContext = GetDC(Window);
			GlobalRunning = true;
			int xOffset = 0;
			int yOffset = 0;

			while(GlobalRunning) {
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
					if (Message.message == WM_QUIT) {
						GlobalRunning = false;
					}
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				} // PeekMessage instead of GetMessage (non-blocking)
				
				RenderWeirdGradient(GlobalBackBuffer, xOffset, yOffset);


				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferWindow(DeviceContext, 
					Dimension.Width, Dimension.Height,
					GlobalBackBuffer, 
					// These last four parameters are ignored for now.
					0, 0, Dimension.Width, Dimension.Height
				);
				xOffset++;
			}
		} else {	
		}
	}else{
	}
}