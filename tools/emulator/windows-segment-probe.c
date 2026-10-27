/* SPDX-License-Identifier: MIT. Decode and play an original ADPCM segment supplied privately by the user.
 * API success must be accompanied by a non-silent, unclipped audio capture. */
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <objbase.h>
#include <dmusici.h>
static void report(const char *text) {
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),text,(DWORD)lstrlenA(text),&written,NULL);
}
static void result(const char *name,HRESULT status) {
    char hex[11]="0x00000000";
    for(unsigned i=0;i<8;i++)hex[9-i]="0123456789abcdef"[((unsigned)status>>(4*i))&15];
    report(name);report(": ");report(hex);report("\r\n");
    if(FAILED(status))ExitProcess(1);
}
void music_start(void) {
    result("CoInitialize",CoInitialize(NULL));
    IDirectMusicPerformance8 *performance=NULL;
    result("Performance8",CoCreateInstance(&CLSID_DirectMusicPerformance,NULL,
        CLSCTX_INPROC_SERVER,&IID_IDirectMusicPerformance8,(void **)&performance));
    HWND window=CreateWindowExA(0,"STATIC","School Tycoon music probe",WS_OVERLAPPED,
        0,0,1,1,NULL,NULL,GetModuleHandleA(NULL),NULL);
    if(!window)ExitProcess(2);
    ShowWindow(window,SW_SHOW);SetForegroundWindow(window);
    result("InitAudio",IDirectMusicPerformance8_InitAudio(performance,NULL,NULL,window,
        DMUS_APATH_DYNAMIC_STEREO,64,DMUS_AUDIOF_ALL,NULL));
    report("ST-MUSIC-INIT-PASS\r\n");
    WCHAR directory[MAX_PATH];
    DWORD length=GetEnvironmentVariableW(L"ST_AUDIO_DIRECTORY",directory,MAX_PATH);
    if(length && length<MAX_PATH) {
        IDirectMusicLoader8 *loader=NULL;
        result("Loader8",CoCreateInstance(&CLSID_DirectMusicLoader,NULL,
            CLSCTX_INPROC_SERVER,&IID_IDirectMusicLoader8,(void **)&loader));
        result("SearchDirectory",IDirectMusicLoader8_SetSearchDirectory(loader,&GUID_DirectMusicAllTypes,directory,TRUE));
        DMUS_OBJECTDESC desc={0};desc.dwSize=sizeof(desc);
        desc.dwValidData=DMUS_OBJ_CLASS|DMUS_OBJ_FILENAME;
        desc.guidClass=CLSID_DirectMusicSegment;
        lstrcpyW(desc.wszFileName,L"music_menu_part_b.sgt");
        IDirectMusicSegment8 *segment=NULL;
        result("LoadSegment",IDirectMusicLoader8_GetObject(loader,&desc,&IID_IDirectMusicSegment8,(void **)&segment));
        result("DownloadSegment",IDirectMusicSegment8_Download(segment,(IUnknown *)performance));
        result("PlaySegment",IDirectMusicPerformance8_PlaySegmentEx(performance,(IUnknown *)segment,NULL,NULL,0,0,NULL,NULL,NULL));
        report("ST-SEGMENT-PLAYING\r\n");
        DWORD started=GetTickCount();
        while(GetTickCount()-started<15000) {
            MSG message;
            while(PeekMessageA(&message,NULL,0,0,PM_REMOVE)) {
                TranslateMessage(&message);DispatchMessageA(&message);
            }
            Sleep(10);
        }
        IDirectMusicSegment8_Release(segment);IDirectMusicLoader8_Release(loader);
    } else Sleep(1000);
    IDirectMusicPerformance8_CloseDown(performance);
    IDirectMusicPerformance8_Release(performance);
    DestroyWindow(window);CoUninitialize();ExitProcess(0);
}
