#pragma once

#define VIDEO_NAME L"Virtual Video Camera"
#define VIDEO_NAME1 "Virtual Video Camera"
#define AUDIO_NAME L"Virtual Video Camera Audio"
#define AUDIO_NAME1 "Virtual Video Camera Audio"

#define VIDEO_WIDTH 1920
#define VIDEO_HEIGHT 1080
#define VIDEO_FPS 25

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

#ifdef _WIN64
#define CLOCK_NAME L"my_clock"
#else
#define CLOCK_NAME "my_clock"
#endif

EXTERN_C const GUID CLSID_VirtualCam;

#define TIME_KILL_SYNCHRONOUS   0x0100 

#ifndef __DVDMEDIA_H__
#include <dvdmedia.h>
#endif

class CVCamStream;
class CVCamAudioStream;

class CVCam : public CSource
{
public:

    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);


private:
    CVCam(LPUNKNOWN lpunk, HRESULT *phr);
};

class CVCamAudio : public CSource
{
public:

	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);


private:
	CVCamAudio(LPUNKNOWN lpunk, HRESULT *phr);
};

class CVCamStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{
	long l1;
	float fps;
public:
	STDMETHODIMP QueryId(__deref_out LPWSTR * Id) { *Id = (LPWSTR)CoTaskMemAlloc(10); (*Id)[0] = '0'; (*Id)[1] = 0; return S_OK; };
	virtual HRESULT DoBufferProcessingLoop(void);
	HANDLE hTick; MMRESULT hEvent;

	int m_lastFn;
	int m_Discontinuity ;

	DWORD lastTime;
	REFERENCE_TIME m_lastSrcRt;

    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
    STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

    //////////////////////////////////////////////////////////////////////////
    //  IQualityControl
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

    //////////////////////////////////////////////////////////////////////////
    //  IKsPropertySet
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData,DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);
    
    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
    CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName);
    ~CVCamStream();

    HRESULT FillBuffer(IMediaSample *pms); 
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT OnThreadCreate(void);
    
public:
    CVCam *m_pParent;
	REFERENCE_TIME m_rtLastTime; __int64 fc64; bool m_bLate;
	CRefTime rtNow;
    HBITMAP m_hLogoBmp;
    CCritSec m_cSharedState;
    //IReferenceClock *m_pClock;
	int fc;
};


class CVCamAudioStream: public CSourceStream, public IAMStreamConfig, public IKsPropertySet, public IAMBufferNegotiation
{
	CCritSec m_cStateLock;	
	CBaseReferenceClock *my_clock;
	int fc, m_lastFn;
public:
	STDMETHODIMP QueryId(__deref_out LPWSTR * Id) { *Id = (LPWSTR)CoTaskMemAlloc(10); (*Id)[0] = '0'; (*Id)[1] = 0; return S_OK; };

	HANDLE hTick;MMRESULT hEvent;
	long pBufOriginalSize,expectedMaxBufferSize ;

	virtual HRESULT STDMETHODCALLTYPE SuggestAllocatorProperties(             /* [in] */ const ALLOCATOR_PROPERTIES *pprop) ;
	virtual HRESULT STDMETHODCALLTYPE GetAllocatorProperties(             /* [annotation][out] */             __out  ALLOCATOR_PROPERTIES *pprop) ;
	virtual HRESULT DoBufferProcessingLoop(void) ;

    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
    STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

    //////////////////////////////////////////////////////////////////////////
    //  IQualityControl
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC);

    //////////////////////////////////////////////////////////////////////////
    //  IKsPropertySet
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void *pInstanceData,DWORD cbInstanceData, void *pPropData, DWORD cbPropData, DWORD *pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport);
    
    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
	CVCamAudioStream(HRESULT *phr, CSource *pParent, LPCWSTR pPinName);

	~CVCamAudioStream();
    HRESULT FillBuffer(IMediaSample *pms);
    HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
    HRESULT CheckMediaType(const CMediaType *pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType *pmt);
    HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT OnThreadCreate(void);
    
public:
    CSource *m_pParent;
	REFERENCE_TIME m_rtLastTime, m_lastSrcRt, rt0; __int64 fc64; bool m_bLate;
	DWORD lastTime;
	bool m_Discontinuity;
    HBITMAP m_hLogoBmp;
    CCritSec m_cSharedState;
    //IReferenceClock *m_pClock;

};


void memcpy2(void* dest, const void* src, const unsigned long size_t);