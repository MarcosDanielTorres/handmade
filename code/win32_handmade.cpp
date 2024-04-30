#include <windows.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstnace, LPSTR lpCmdLine, int nCmdShow) {
	MessageBoxA(0, "This is handmade hero", "Handmade Hero", MB_OK|MB_ICONINFORMATION);
	return 0;
}