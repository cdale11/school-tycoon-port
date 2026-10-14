/* SPDX-License-Identifier: MIT
 * Warm Wine's private X11 desktop before game startup; retire when it opens.
 * No game memory access, graphics bridge or proprietary code.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static WNDCLASSA cls;
static void report(const char *text) {
    DWORD written;
    WriteFile(GetStdHandle(STD_ERROR_HANDLE),text,(DWORD)lstrlenA(text),&written,NULL);
}
static void fail(DWORD status,const char *text) {report(text);ExitProcess(status);}

static LRESULT CALLBACK procedure(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    if(message==WM_TIMER && FindWindowA(NULL,"School Tycoon")) {report("ST-DESKTOP-RETIRED: game window exists\r\n");DestroyWindow(window);return 0;}
    if(message==WM_CLOSE) {DestroyWindow(window);return 0;}
    if(message==WM_DESTROY) {PostQuitMessage(0);return 0;}
    return DefWindowProcA(window,message,wp,lp);
}
void desktop_start(void) {
    cls.lpfnWndProc=procedure;cls.hInstance=GetModuleHandleA(NULL);
    cls.lpszClassName="SchoolTycoonDesktopReady";
    if(!RegisterClassA(&cls)) fail(1,"Desktop class registration failed\r\n");
    HWND window=CreateWindowExA(0,cls.lpszClassName,"School Tycoon desktop readiness",
        WS_OVERLAPPED,0,0,1,1,NULL,NULL,cls.hInstance,NULL);
    if(!window) fail(2,"Private Wine desktop window creation failed\r\n");
    HANDLE output=CreateFileA("desktop-ready.txt",GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(output==INVALID_HANDLE_VALUE) {DestroyWindow(window);fail(3,"Desktop readiness log cannot be opened\r\n");}
    DWORD written=0;const char marker[]="ST-DESKTOP-READY\r\n";
    BOOL ok=WriteFile(output,marker,sizeof(marker)-1,&written,NULL);CloseHandle(output);
    if(!ok || written!=sizeof(marker)-1) {DestroyWindow(window);fail(4,"Desktop readiness marker write failed\r\n");}
    if(!SetTimer(window,1,1000,NULL)) {DestroyWindow(window);fail(5,"Desktop retirement timer failed\r\n");}
    report("ST-DESKTOP-READY\r\n");
    MSG message;int result;
    while((result=GetMessageA(&message,NULL,0,0))>0) {TranslateMessage(&message);DispatchMessageA(&message);}
    if(result<0) fail(6,"Desktop message loop failed\r\n");
    ExitProcess(0);
}
