#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

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

// Day 6 23:00 - 30:00
// ---- Loading the WinAPI functions myself. ----
// Two ways of doing the same thing: A and B
// A == B but A gives as the ability to call functions as stub.

// -- A --
// #define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pStat)
// #define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
// -- B --
// typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE* pStat);
// typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
// -- B --


/*
	Easier to see solution:
		typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE* pStat);

		DWORD WINAPI XInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE* pStat){
			return 0;
		}

		global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
		#define XInputGetState XInputGetState_

	Solution chosen:
		#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pStat)
		typedef X_INPUT_GET_STATE(x_input_get_state);
		X_INPUT_GET_STATE(XInputGetStateStub) {
			return 0 ;
		}

		global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
		#define XInputGetState XInputGetState_
*/

// --- XInput GET State ---
	#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pStat)
	typedef X_INPUT_GET_STATE(x_input_get_state);
	X_INPUT_GET_STATE(XInputGetStateStub) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}

	global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
	#define XInputGetState XInputGetState_

	// --- XInput SET State ---
	#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
	typedef X_INPUT_SET_STATE(x_input_set_state);
	X_INPUT_SET_STATE(XInputSetStateStub) {
		return ERROR_DEVICE_NOT_CONNECTED;
	}
	global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
	#define XInputSetState XInputSetState_


	#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
	typedef DIRECT_SOUND_CREATE(direct_sound_create);


	internal void Win32LoadXInput(void) {
		HMODULE XInputLibrary =  LoadLibrary("xinput1_4.dll");
		if (!XInputLibrary) { XInputLibrary = LoadLibrary("xinput1_3.dll"); }

		if (XInputLibrary) {
			// In the debugger to see if these were set correctly I need to inspect the variable
			// `XInputGetState_`
			XInputGetState = (x_input_get_state*) GetProcAddress(XInputLibrary, "XInputGetState");
			XInputSetState = (x_input_set_state*) GetProcAddress(XInputLibrary, "XInputSetState");
		}
	}

// ---- Loading the WinAPI functions myself. ----


u8 BytesPerPixel = 4;
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

global_variable LPDIRECTSOUNDBUFFER GlobalSoundBuffer;


internal void RenderWeirdGradient(win32_offscreen_buffer* Buffer, int xOffset, int yOffset){
	// Considerations
	// Even though at the moment GlobalBackBuffer is global. In the future I may use different buffers.
	// Buffer passed by copy because it doesn't matter. I'm not changing the buffer itself, just the memory it points to. So I think-
	// passing by copy or by reference is the same. Casey later changed it to reference because he is not sure about which one is better and
	// he is used to passing by reference.

	for (int y = 0; y < Buffer->Height; y++) {
		for (int x = 0; x < Buffer->Width; x++) {
			u8 *Pixel = ((u8 *)Buffer->Memory + (x*Buffer->BytesPerPixel) + (y*Buffer->Width*Buffer->BytesPerPixel));
			// 0x xx RR GG BB -> 0xBB GG RR xx
			*Pixel = x + xOffset; 			// blue
			*(Pixel + 1) = y + yOffset;		// green
			*(Pixel + 2) = 0; 				// red
			*(Pixel + 3) = 0;				// padding
		}
	}
}

internal void Win32InitDSound(HWND Window, i32 SamplesPerSecond, i32 BufferSize) {
	HMODULE DSoundLibrary = LoadLibrary("dsound.dll");
	if (DSoundLibrary) {
		direct_sound_create *DirectSoundCreate = (direct_sound_create*) GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))){
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
						OutputDebugString("Primary buffer format was set.\n");
					}else{

					}
				}else{
					
				}
			}else{

			}
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSoundBuffer, 0))) {
				OutputDebugString("Secondary buffer created successfully!\n");
			}
		}else{

		}
	}else{
			
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
	// I could have many different buffers, not just one.
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
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32DisplayBufferWindow(
		HDC DeviceContext,
		int destWidth, int destHeight,
		win32_offscreen_buffer* Buffer
		) {

	// TODO Aspect ratio correction
	StretchDIBits(
		DeviceContext,
		// First do the entire window (use the bitmaps size) instead of a subregion
		// X, Y, Width, Height,
		// X, Y, Width, Height,
		0, 0, destWidth, destHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory, &Buffer->Info,
		DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback( HWND WinHandle, UINT Message, WPARAM WParam, LPARAM LParam) {
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

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:{
			u32 VKCode = WParam;
			bool WasDown = ((LParam & (1 << 30)) != 0);	
			bool IsDown = ((LParam & (1 << 31)) == 0);
			if (WasDown != IsDown) {

			if (VKCode == 'W') {
				bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
				bool CtrlKeyWasDown = GetKeyState(VK_CONTROL) & (1 << 15);
				if (CtrlKeyWasDown){
					GlobalRunning = false;
				}
			}else if (VKCode == 'A') {
			}else if (VKCode == 'S') {
			}else if (VKCode == 'D') {
			}else if (VKCode == 'Q') {
			}else if (VKCode == 'E') {
			}else if (VKCode == VK_SPACE) {
			}else if (VKCode == VK_ESCAPE) {
				OutputDebugString("ESCAPE: ");
				if (IsDown) {
					OutputDebugString("IsDown!");
				}
				if (WasDown) {
					OutputDebugString("WasDown!");
				}
				OutputDebugString("\n");
			}else if (VKCode == VK_UP) {
			}else if (VKCode == VK_DOWN) {
			}else if (VKCode == VK_LEFT) {
			}else if (VKCode == VK_RIGHT) {
			}
			
			}


		} break;

		case WM_PAINT: {
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(WinHandle, &Paint);

			win32_window_dimension Dimension = Win32GetWindowDimension(WinHandle);
			Win32DisplayBufferWindow(DeviceContext, 
				Dimension.Width, Dimension.Height,
				&GlobalBackBuffer
			);

			EndPaint(WinHandle, &Paint);
		} break;

		default: {
			Result = DefWindowProc(WinHandle, Message, WParam, LParam);
		} break;
	}

	return Result;
}


internal int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode) {
	WNDCLASS WindowClass = {};
	Win32LoadXInput();

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

			int SamplesPerSecond = 48000;
			u32 RunningSampleIndex = 0;
			int Hz = 256;
			int ToneVolume = 6000;
			int BytesPerSample = sizeof(i16) * 2;
			int SquareWavePeriod = SamplesPerSecond / Hz;
			int SoundBufferSize = SamplesPerSecond * BytesPerSample;
			int HalfSquareWavePeriod = SquareWavePeriod / 2;
			bool SoundIsPlaying = false;
			Win32InitDSound(Window, SamplesPerSecond, SoundBufferSize);

			while(GlobalRunning) {
				MSG Message;
				while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)){
					if (Message.message == WM_QUIT) {
						GlobalRunning = false;
					}
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				} // PeekMessage instead of GetMessage (non-blocking)
				
				// TODO Should we poll this more frequently
				DWORD dwResult;
				for(int i = 0; i < XUSER_MAX_COUNT; i++) {
					XINPUT_STATE ControllerState;
					ZeroMemory(&ControllerState, sizeof(XINPUT_STATE));
					if(XInputGetState(i, &ControllerState) == ERROR_SUCCESS) {
						// Controller is plugged in
						// TODO See if ControllerState.dwPacketNumber increments too rapidly
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

						i16 StickX = Pad->sThumbLX;
						i16 StickY = Pad->sThumbLY;
						xOffset += StickX >> 12;
						yOffset += StickY >> 12;
					} else {
						// Controller is not available	
					}
				}

				RenderWeirdGradient(&GlobalBackBuffer, xOffset, yOffset);

				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SUCCEEDED(GlobalSoundBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))){
					DWORD ByteToLock = RunningSampleIndex * BytesPerSample % SoundBufferSize;
					DWORD BytesToWrite;
					if (ByteToLock == PlayCursor) {
						BytesToWrite = SoundBufferSize;
					} else if (ByteToLock > PlayCursor) {
						/*                  SoundBufferSize - ByteToLock          
							         PlayCursor           v 
	                                 v           |________________|
							|xxxxxxxxx---------- xxxxxxxxxxxxxxxxx|
							0                    ^                SoundBufferSize
		                                         ByteToLock
						*/
						BytesToWrite = SoundBufferSize - ByteToLock;
						BytesToWrite += PlayCursor;
					}else {
						BytesToWrite = PlayCursor - ByteToLock;
					}

					//	i16   i16     i16	i16	  i16	i16  i16  i16
					// [LEFT RIGHT] [LEFT RIGHT] LEFT RIGHT LEFT RIGHT... 
					VOID *Region1;
					DWORD Region1Size;
					VOID *Region2;
					DWORD Region2Size;

					if (SUCCEEDED(GlobalSoundBuffer->Lock(
						ByteToLock,
						BytesToWrite,
						&Region1, &Region1Size,
						&Region2, &Region2Size,
						0))) {

						i16 *SampleOut = (i16*) Region1;
						DWORD Region1SampleCount = Region1Size / BytesPerSample;
						for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++) {
							i16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume: -ToneVolume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}

						SampleOut = (i16*) Region2;
						DWORD Region2SampleCount = Region2Size / BytesPerSample;
						for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++) {
							i16 SampleValue = ((RunningSampleIndex++ / HalfSquareWavePeriod) % 2) ? ToneVolume: -ToneVolume;
							*SampleOut++ = SampleValue;
							*SampleOut++ = SampleValue;
						}
						GlobalSoundBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
					}
				}
				if (!SoundIsPlaying) {
					GlobalSoundBuffer->Play(0, 0, DSBPLAY_LOOPING);
					SoundIsPlaying = true;
				}
				win32_window_dimension Dimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferWindow(DeviceContext, 
					Dimension.Width, Dimension.Height,
					&GlobalBackBuffer
				);
			}
		} else {	
		}
	}else{
	}
}