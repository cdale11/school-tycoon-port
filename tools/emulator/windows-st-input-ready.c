/* SPDX-License-Identifier: MIT
 * Read-only input readiness probe for the inspected School Tycoon executable.
 * Its main image must match the hash documented in emulator.md. This is a
 * private VM fixture, not an input shim or modification of the game.
 * /xN /yN: observe readiness for a screen cursor moved by the native driver.
 * /button0 or /button1: wait for the engine's left-button state.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
static BOOL read_bytes(HANDLE process,DWORD address,void *output,SIZE_T size) {
    SIZE_T read=0;
    return ReadProcessMemory(process,(void *)(ULONG_PTR)address,output,size,&read) && read==size;
}
static int value(const char *p) {
    int n=0,digits=0;
    while(*p>='0'&&*p<='9') { if(++digits>4)return -1; n=n*10+*p++-'0'; }
    return digits?n:-1;
}
void input_start(void) {
    int x=-1,y=-1,button=-1;
    for(const char *p=GetCommandLineA();*p;p++) {
        if(p[0]=='/'&&p[1]=='x')x=value(p+2);
        if(p[0]=='/'&&p[1]=='y')y=value(p+2);
        if(p[0]=='/'&&p[1]=='b'&&p[2]=='u'&&p[3]=='t'&&p[4]=='t'&&p[5]=='o'&&p[6]=='n')button=value(p+7);
    }
    if(button<0 && (x<0||y<0||x>=800||y>=600))ExitProcess(1);
    if(button>1)ExitProcess(1);
    HWND window=FindWindowA(NULL,"School Tycoon");
    DWORD pid=0;GetWindowThreadProcessId(window,&pid);
    HANDLE process=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION,FALSE,pid);
    if(!window||!process)ExitProcess(2);
    DWORD engine=0,mouse=0,owner=0;
    if(!read_bytes(process,0x654bcc,&engine,sizeof(engine))||!engine
        ||!read_bytes(process,engine+8,&owner,sizeof(owner))||owner!=(DWORD)(ULONG_PTR)window
        ||!read_bytes(process,0x65490c,&mouse,sizeof(mouse))||!mouse)ExitProcess(3);
    POINT expected={x,y};
    if(button<0 && !ScreenToClient(window,&expected))ExitProcess(4);
    DWORD start=GetTickCount();
    BOOL ready=FALSE;
    while(GetTickCount()-start<120000) {
        if(button>=0) {
            BYTE pressed=0;
            if(!read_bytes(process,mouse+0x28,&pressed,1))ExitProcess(5);
            ready=(pressed!=0)==button;
        } else {
            float point[2];
            if(!read_bytes(process,engine+0x2c,point,sizeof(point)))ExitProcess(5);
            ready=point[0]==(float)expected.x && point[1]==(float)expected.y;
        }
        if(ready)break;
        Sleep(250);
    }
    CloseHandle(process);
    if(!ready)ExitProcess(6);
    const char marker[]="ST-INPUT-READY\r\n";
    DWORD written=0;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),marker,sizeof(marker)-1,&written,NULL);
    ExitProcess(written==sizeof(marker)-1?0:7);
}
