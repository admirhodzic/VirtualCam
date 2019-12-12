//////////////////////////////////////////////////////////////////////////
//  This file contains routines to register / Unregister the 
//  Directshow filter 'Virtual Cam'
//  We do not use the inbuilt BaseClasses routines as we need to register as
//  a capture source
//////////////////////////////////////////////////////////////////////////

#pragma comment(lib, "winmm")

#include "baseclasses/streams.h"
#include <olectl.h> 
#include <initguid.h>
#include <dllsetup.h>
#include "filters.h"

#define CreateComObject(clsid, iid, var) CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, iid, (void **)&var);

STDAPI AMovieSetupRegisterServer( CLSID   clsServer, LPCWSTR szDescription, LPCWSTR szFileName, LPCWSTR szThreadingModel = L"Both", LPCWSTR szServerType     = L"InprocServer32" );
STDAPI AMovieSetupUnregisterServer( CLSID clsServer );
// {8E14549A-CB61-4309-AFA1-3578E927E933}
// {8E14549A-CB61-4309-AFA1-3578E927E933}
DEFINE_GUID(CLSID_VirtualCam, 0x8e14549a, 0xcb61, 0x4309, 0xaf, 0xa1, 0x35, 0x78, 0xe9, 0x27, 0xe9, 0x33);
// {8E14549A-CB61-4309-AFA1-3578E927E934}
DEFINE_GUID(CLSID_VirtualCamAudio, 0x8e14549a, 0xcb61, 0x4309, 0xaf, 0xa1, 0x35, 0x78, 0xe9, 0x27, 0xe9, 0x34);


const AMOVIESETUP_MEDIATYPE AMSMediaTypesVCam = 
{ 
    &MEDIATYPE_Video, 
    &MEDIASUBTYPE_NULL 
};
const AMOVIESETUP_MEDIATYPE AMSMediaTypesVCamAudio = 
{ 
    &MEDIATYPE_Audio, 
    &MEDIASUBTYPE_NULL 
};

const AMOVIESETUP_PIN AMSPinVCam=
{
    L"VOutput",             // Pin string name
    FALSE,                 // Is it rendered
    TRUE,                  // Is it an output
    FALSE,                 // Can we have none
    FALSE,                 // Can we have many
    &CLSID_NULL,           // Connects to filter
    NULL,                  // Connects to pin
    1,                     // Number of types
    &AMSMediaTypesVCam      // Pin Media types
};
const AMOVIESETUP_PIN AMSPinVCamAudio=
{
    L"AOutput",             // Pin string name
    FALSE,                 // Is it rendered
    TRUE,                  // Is it an output
    FALSE,                 // Can we have none
    FALSE,                 // Can we have many
    &CLSID_NULL,           // Connects to filter
    NULL,                  // Connects to pin
    1,                     // Number of types
    &AMSMediaTypesVCamAudio      // Pin Media types
};
const AMOVIESETUP_PIN AMSPinVCamSink=
{
    L"VInput",             // Pin string name
    FALSE,                 // Is it rendered
    FALSE,                  // Is it an output
    FALSE,                 // Can we have none
    FALSE,                 // Can we have many
    &CLSID_NULL,           // Connects to filter
    NULL,                  // Connects to pin
    1,                     // Number of types
    &AMSMediaTypesVCam      // Pin Media types
};
const AMOVIESETUP_PIN AMSPinVCamAudioSink=
{
    L"AInput",             // Pin string name
    FALSE,                 // Is it rendered
    FALSE,                  // Is it an output
    FALSE,                 // Can we have none
    FALSE,                 // Can we have many
    &CLSID_NULL,           // Connects to filter
    NULL,                  // Connects to pin
    1,                     // Number of types
    &AMSMediaTypesVCamAudio      // Pin Media types
};

const AMOVIESETUP_FILTER AMSFilterVCam =
{
    &CLSID_VirtualCam,  // Filter CLSID
    VIDEO_NAME,     // String name
    MERIT_DO_NOT_USE,      // Filter merit
    1,                     // Number pins
    &AMSPinVCam             // Pin details
};
const AMOVIESETUP_FILTER AMSFilterVCamAudio =
{
    &CLSID_VirtualCamAudio,  // Filter CLSID
    AUDIO_NAME,     // String name
    MERIT_DO_NOT_USE,      // Filter merit
    1,                     // Number pins
    &AMSPinVCamAudio             // Pin details
};

CFactoryTemplate g_Templates[] = 
{
    {
        VIDEO_NAME,
        &CLSID_VirtualCam,
        CVCam::CreateInstance,
        NULL,
        &AMSFilterVCam
    },
	{
        AUDIO_NAME,
        &CLSID_VirtualCamAudio,
        CVCamAudio::CreateInstance,
        NULL,
        &AMSFilterVCamAudio
    }

};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI RegisterFilters( BOOL bRegister )
{
    HRESULT hr = NOERROR;
    WCHAR achFileName[MAX_PATH];
    char achTemp[MAX_PATH];
    ASSERT(g_hInst != 0);

    if( 0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp))) 
        return AmHresultFromWin32(GetLastError());

    MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1, 
                       achFileName, NUMELMS(achFileName));
  
    hr = CoInitialize(0);
    
	if(bRegister)
    {
        hr = AMovieSetupRegisterServer(CLSID_VirtualCam, VIDEO_NAME, achFileName, L"Both", L"InprocServer32");
        hr = AMovieSetupRegisterServer(CLSID_VirtualCamAudio, AUDIO_NAME, achFileName, L"Both", L"InprocServer32");
    }

    if( SUCCEEDED(hr) )
    {
        IFilterMapper2 *fm = 0;
        hr = CreateComObject( CLSID_FilterMapper2, IID_IFilterMapper2, fm );
        if( SUCCEEDED(hr) )
        {
            if(bRegister)
            {
                IMoniker *pMoniker = 0;
				REGFILTER2 rf2={0};
                rf2.dwVersion = 1;
                rf2.dwMerit = MERIT_DO_NOT_USE;
                rf2.cPins = 1;
                rf2.rgPins = &AMSPinVCam;
                hr = fm->RegisterFilter(CLSID_VirtualCam, VIDEO_NAME, &pMoniker, &CLSID_VideoInputDeviceCategory, NULL, &rf2);
				if(pMoniker) pMoniker->Release();pMoniker=0;
                rf2.rgPins = &AMSPinVCamAudio;
                hr = fm->RegisterFilter(CLSID_VirtualCamAudio, AUDIO_NAME, &pMoniker, &CLSID_AudioInputDeviceCategory, NULL, &rf2);
				if(pMoniker) pMoniker->Release();pMoniker=0;
            }
            else
            {
                hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0, CLSID_VirtualCam);
				hr = fm->UnregisterFilter(&CLSID_AudioInputDeviceCategory, 0, CLSID_VirtualCamAudio);
            }
        }

      // release interface
      //
      if(fm)
          fm->Release();
    }

    if( SUCCEEDED(hr) && !bRegister ){
        hr = AMovieSetupUnregisterServer( CLSID_VirtualCam );
        hr = AMovieSetupUnregisterServer( CLSID_VirtualCamAudio );
	}

    CoFreeUnusedLibraries();
    CoUninitialize();
    return hr;
}

STDAPI DllRegisterServer()
{

    return RegisterFilters(TRUE);
}

STDAPI DllUnregisterServer()
{
    return RegisterFilters(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

void memcpy2(void* dest, const void* src, const unsigned long size_t)
{
#ifdef _WIN64
	memcpy(dest, src, size_t);
#else

	__asm
	{
		mov esi, src;    //src pointer
		mov edi, dest;   //dest pointer

		mov ebx, size_t; //ebx is our counter 
		shr ebx, 7;      //divide by 128 (8 * 128bit registers)


	loop_copy:
		prefetchnta 128[ESI]; //SSE2 prefetch
		prefetchnta 160[ESI];
		prefetchnta 192[ESI];
		prefetchnta 224[ESI];

		movdqa xmm0, 0[ESI]; //move data from src to registers
		movdqa xmm1, 16[ESI];
		movdqa xmm2, 32[ESI];
		movdqa xmm3, 48[ESI];
		movdqa xmm4, 64[ESI];
		movdqa xmm5, 80[ESI];
		movdqa xmm6, 96[ESI];
		movdqa xmm7, 112[ESI];

		movntdq 0[EDI], xmm0; //move data from registers to dest
		movntdq 16[EDI], xmm1;
		movntdq 32[EDI], xmm2;
		movntdq 48[EDI], xmm3;
		movntdq 64[EDI], xmm4;
		movntdq 80[EDI], xmm5;
		movntdq 96[EDI], xmm6;
		movntdq 112[EDI], xmm7;

		add esi, 128;
		add edi, 128;
		dec ebx;

		jnz loop_copy; //loop please
		emms;
	}

	if (size_t & 127) memcpy((LPBYTE)dest + (size_t & 0xffffff80), (LPBYTE)src + (size_t & 0xffffff80), size_t & 0x7f);
#endif
}