/* SPDX-License-Identifier: MIT. Own DirectSound fixture; no game code/assets. */
#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
static void report(const char *text) {
    DWORD written;
    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),text,(DWORD)lstrlenA(text),&written,NULL);
}
void audio_start(void) {
    LPDIRECTSOUND sound=NULL;
    LPDIRECTSOUNDBUFFER buffer=NULL,primary=NULL;
    HRESULT status=DirectSoundCreate(NULL,&sound,NULL);
    if(FAILED(status)) {report("FAIL: DirectSoundCreate\r\n");ExitProcess(1);}
    HWND window=CreateWindowExA(0,"STATIC","School Tycoon audio probe",WS_OVERLAPPED,
        0,0,1,1,NULL,NULL,GetModuleHandleA(NULL),NULL);
    if(!window || FAILED(IDirectSound_SetCooperativeLevel(sound,window,DSSCL_PRIORITY))) {
        report("FAIL: audio cooperative level\r\n");ExitProcess(2);
    }
    WAVEFORMATEX format={WAVE_FORMAT_PCM,2,22050,88200,4,16,0};
    DSBUFFERDESC primary_desc={0};primary_desc.dwSize=sizeof(primary_desc);
    primary_desc.dwFlags=DSBCAPS_PRIMARYBUFFER;
    if(FAILED(IDirectSound_CreateSoundBuffer(sound,&primary_desc,&primary,NULL)) ||
       FAILED(IDirectSoundBuffer_SetFormat(primary,&format))) {
        report("FAIL: primary PCM16 format\r\n");ExitProcess(7);
    }
    DSBUFFERDESC desc={0};desc.dwSize=sizeof(desc);
    desc.dwFlags=DSBCAPS_GLOBALFOCUS;desc.dwBufferBytes=88200;desc.lpwfxFormat=&format;
    if(FAILED(IDirectSound_CreateSoundBuffer(sound,&desc,&buffer,NULL))) {
        report("FAIL: sound buffer creation\r\n");ExitProcess(3);
    }
    void *first=NULL,*second=NULL;DWORD first_size=0,second_size=0;
    if(FAILED(IDirectSoundBuffer_Lock(buffer,0,88200,&first,&first_size,&second,&second_size,0))) {
        report("FAIL: sound buffer lock\r\n");ExitProcess(4);
    }
    unsigned frame=0;
    for(unsigned part=0;part<2;part++) {
        short *samples=part?second:first;DWORD size=part?second_size:first_size;
        for(DWORD i=0;i<size/sizeof(short);i+=2,frame++) {
            short value=(frame%50<25)?3000:-3000;
            samples[i]=value;samples[i+1]=value;
        }
    }
    IDirectSoundBuffer_Unlock(buffer,first,first_size,second,second_size);
    if(FAILED(IDirectSoundBuffer_Play(buffer,0,0,DSBPLAY_LOOPING))) {
        report("FAIL: sound buffer playback\r\n");ExitProcess(5);
    }
    report("ST-AUDIO-PLAYING\r\n");Sleep(3000);
    DWORD position=0,write_position=0;
    status=IDirectSoundBuffer_GetCurrentPosition(buffer,&position,&write_position);
    IDirectSoundBuffer_Stop(buffer);IDirectSoundBuffer_Release(buffer);
    IDirectSoundBuffer_Release(primary);IDirectSound_Release(sound);DestroyWindow(window);
    if(FAILED(status) || !position) {report("FAIL: playback cursor did not advance\r\n");ExitProcess(6);}
    report("ST-AUDIO-API-PASS\r\n");ExitProcess(0);
}
