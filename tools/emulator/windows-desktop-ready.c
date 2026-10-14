/* SPDX-License-Identifier: MIT
 * Private VM: keep a passive Wine desktop alive during original-game tests.
 * Build a PE32 GUI executable without the C runtime; entry desktop_start.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static WNDCLASSA window_class;
static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wp, LPARAM lp) {
    if (message == WM_CLOSE) { DestroyWindow(window); return 0; }
    if (message == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcA(window,message,wp,lp);
}
void desktop_start(void) {
    window_class.lpfnWndProc=window_proc;
    window_class.hInstance=GetModuleHandleA(NULL);
    window_class.lpszClassName="STVMDesktopReady";
    if(!RegisterClassA(&window_class)) ExitProcess(1);
    HWND window=CreateWindowExA(0,window_class.lpszClassName,"ST VM desktop keeper",
        WS_OVERLAPPED,0,0,1,1,NULL,NULL,window_class.hInstance,NULL);
    if(!window) ExitProcess(2);
    HANDLE output=CreateFileA("desktop-ready.txt",GENERIC_WRITE,0,NULL,
        CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE) ExitProcess(3);
    DWORD written=0;
    const char marker[]="ST-DESKTOP-READY\r\n";
    BOOL saved=WriteFile(output,marker,sizeof(marker)-1,&written,NULL);
    CloseHandle(output);
    if(!saved || written!=sizeof(marker)-1) ExitProcess(4);
    MSG message;
    int result;
    while((result=GetMessageA(&message,NULL,0,0))>0) {
        TranslateMessage(&message); DispatchMessageA(&message);
    }
    ExitProcess(result<0?5:0);
}
