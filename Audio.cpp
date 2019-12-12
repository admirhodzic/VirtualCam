#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include "baseclasses/streams.h"
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include <refclock.h>
#include "filters.h"

CUnknown * WINAPI CVCamAudio::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);
	CUnknown *punk = new CVCamAudio(lpunk, phr);
	return punk;
}

CVCamAudio::CVCamAudio(LPUNKNOWN lpunk, HRESULT *phr) :
	CSource(NAME(AUDIO_NAME1), lpunk, CLSID_VirtualCam)
{
	ASSERT(phr);
	CAutoLock cAutoLock(&m_cStateLock);
	// Create the one and only output pin
	m_paStreams = (CSourceStream **) new CVCamAudioStream*[1];
	m_paStreams[0] = new CVCamAudioStream(phr, this, L"audio out");
}

HRESULT CVCamAudio::QueryInterface(REFIID riid, void **ppv)
{
	//Forward request for IAMStreamConfig & IKsPropertySet to the pin
	if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet) || riid == _uuidof(IReferenceClock))
		return m_paStreams[0]->QueryInterface(riid, ppv);
	
	return CSource::QueryInterface(riid, ppv);
}


//////////////////////////////////////////////////////////////////////////
// CVCamAudioStream is the one and only output pin of CVCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CVCamAudioStream::CVCamAudioStream(HRESULT *phr, CSource *pParent, LPCWSTR pPinName) :fc(0),
    CSourceStream(NAME(AUDIO_NAME1),phr, pParent, pPinName), m_pParent(pParent)
{
	my_clock = new CBaseReferenceClock(CLOCK_NAME, NULL, phr); my_clock->AddRef();

	//AM_MEDIA_TYPE m=buf->_mt;
	//m.pbFormat=(LPBYTE)&buf->_wfx;
    //CopyMediaType(&m_mt,&m);

	m_mt.InitMediaType();
	
	CMediaType *pmt=&m_mt;
	pmt->majortype=MEDIATYPE_Audio;
	pmt->subtype=MEDIASUBTYPE_PCM;
	pmt->bFixedSizeSamples=1;
	pmt->bTemporalCompression=0;
	pmt->cbFormat=sizeof(WAVEFORMATEX);
	pmt->formattype=FORMAT_WaveFormatEx;
	pmt->lSampleSize=2*2;
	pmt->majortype=MEDIATYPE_Audio;
	pmt->pbFormat=(BYTE*)CoTaskMemAlloc(sizeof(WAVEFORMATEX));
	WAVEFORMATEX* pvi=(WAVEFORMATEX* ) pmt->pbFormat;//m_mt.AllocFormatBuffer(sizeof(WAVEFORMATEX));
	WAVEFORMATEX w={WAVE_FORMAT_PCM,2,48000,2*2*48000,4,16,0};
	pmt->pbFormat=(LPBYTE)pvi;
	pBufOriginalSize=w.nAvgBytesPerSec*5;
	expectedMaxBufferSize=pBufOriginalSize;
	CopyMemory(pvi,&w,sizeof(WAVEFORMATEX));
	hTick=CreateEvent(0,0,0,0);
	timeBeginPeriod(1);
	hEvent=timeSetEvent(1,1,(LPTIMECALLBACK)hTick,(DWORD_PTR)this,TIME_PERIODIC|TIME_CALLBACK_EVENT_SET|TIME_KILL_SYNCHRONOUS);
    
}

CVCamAudioStream::~CVCamAudioStream()
{
	timeKillEvent(hEvent);
	CloseHandle(hTick);
	if(my_clock) my_clock->Release();
} 

HRESULT CVCamAudioStream::QueryInterface(REFIID riid, void **ppv)
{   
    // Standard OLE stuff
    if(riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if(riid == _uuidof(IAMBufferNegotiation))
        *ppv = (IAMBufferNegotiation*)this;
    else if(riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
	else if (riid == _uuidof(IReferenceClock))
		return my_clock->QueryInterface(riid,ppv);
	else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef();
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//  This is the routine where we create the data being output by the Virtual
//  Camera device.
//////////////////////////////////////////////////////////////////////////


HRESULT CVCamAudioStream::FillBuffer(IMediaSample *pms)
{
    REFERENCE_TIME rtNow;
	//WaitForSingleObject(hTick,10);

	WAVEFORMATEX *pwfx = (WAVEFORMATEX *) m_mt.pbFormat;
	CAutoLock cAutoLock(&m_cStateLock);
	
	int l = pwfx->nAvgBytesPerSec;
	REFERENCE_TIME FrameTime = (UNITS*(REFERENCE_TIME)l) / (REFERENCE_TIME)pwfx->nAvgBytesPerSec;


	BYTE *pData;
	pms->GetPointer(&pData);
	if(l>0){
		pms->SetActualDataLength(l);
			
		rtNow = m_rtLastTime;

		m_rtLastTime += FrameTime;
		
		pms->SetTime(&rtNow, &m_rtLastTime);
		
		pms->SetSyncPoint(TRUE);
		pms->SetDiscontinuity(m_Discontinuity);m_Discontinuity=false;//m_Discontinuity||(fc && 0==fc%5000)
		__int64 f0=fc64;fc64++;
		pms->SetMediaTime(&f0,&fc64);
		pms->SetPreroll(false);
		
		if (m_pFilter->m_State == State_Paused) {
			memset(pData, 0, l);
		}

	}
	
	fc++;
	Sleep(1);

    return NOERROR;
} // FillBuffer


//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CVCamAudioStream::Notify(IBaseFilter * pSender, Quality q)
{
    return E_NOTIMPL;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CVCamAudioStream::SetMediaType(const CMediaType *pmt)
{
//    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CVCamAudioStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	CopyMediaType(pmt,&m_mt);
	
    return NOERROR;

} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CVCamAudioStream::CheckMediaType(const CMediaType *pMediaType)
{
    if(*pMediaType != m_mt)         return E_INVALIDARG; 
	//if(IsEqualGUID(*pMediaType->FormatType(),FORMAT_WaveFormatEx)) 		return S_OK;
    return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CVCamAudioStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    pProperties->cBuffers = 1;
//	WAVEFORMATEX *pwfx = (WAVEFORMATEX *) &buf->_wfx;
    pProperties->cbBuffer = expectedMaxBufferSize ;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);

    if(FAILED(hr)) return hr;
    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

    return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CVCamAudioStream::OnThreadCreate()
{
    m_rtLastTime = 0*UNITS/25;fc64=0;rt0=0;
	m_lastSrcRt = 0;
	lastTime = 0;

    return NOERROR;
} // OnThreadCreate


//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CVCamAudioStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
	if(!pmt) return S_OK;
//	DECLARE_PTR(WAVEFORMATEX, pvi, pmt->pbFormat);
	if(CheckMediaType((CMediaType*)pmt)) 		return E_FAIL;
	m_mt=*pmt;
    IPin* pin; 
    ConnectedTo(&pin);
    if(pin)
    {
		IFilterGraph *pGraph = m_pParent->m_pGraph;
        pGraph->Reconnect(this);
    }
    return S_OK;
}


STDMETHODIMP CVCamAudioStream::SuggestAllocatorProperties(const ALLOCATOR_PROPERTIES *pprop)
{
	int requested = pprop->cbBuffer;
	if(pprop->cBuffers > 0)
	    requested *= pprop->cBuffers;
	if(pprop->cbPrefix > 0)
	    requested += pprop->cbPrefix;
	
	if(requested <= pBufOriginalSize) {
 		expectedMaxBufferSize = requested;
		return S_OK; // they requested it? ok you got it! You're requesting possible problems! oh well! you requested it!
	} else {
		return E_FAIL;
	}
}
STDMETHODIMP CVCamAudioStream::GetAllocatorProperties(ALLOCATOR_PROPERTIES *pprop)
{
this->m_pAllocator->GetProperties(pprop);
return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamAudioStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	*ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamAudioStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    *piCount = 1;
    *piSize = sizeof(AUDIO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamAudioStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
	
	if(iIndex < 0) return E_INVALIDARG;	if(iIndex > 0)		return S_FALSE;	if(pSCC == NULL)		return E_POINTER;

	*pmt = CreateMediaType(&m_mt);
	
	WAVEFORMATEX *pwfx = (WAVEFORMATEX *) m_mt.pbFormat;
//	WAVEFORMATEX *pwfx2 = (WAVEFORMATEX *) (*pmt)->pbFormat;

    DECLARE_PTR(AUDIO_STREAM_CONFIG_CAPS, pvscc, pSCC);
	ZeroMemory(pvscc,sizeof(AUDIO_STREAM_CONFIG_CAPS));
    
    pvscc->guid = MEDIATYPE_Audio;//FORMAT_WaveFormatEx;
	
	pvscc->BitsPerSampleGranularity=pwfx->wBitsPerSample;
	pvscc->MaximumBitsPerSample=pwfx->wBitsPerSample;
	pvscc->MinimumBitsPerSample=pwfx->wBitsPerSample;

	pvscc->MaximumChannels=pwfx->nChannels;
	pvscc->MinimumChannels=pwfx->nChannels;
	pvscc->ChannelsGranularity=pwfx->nChannels;

	pvscc->MaximumSampleFrequency=pwfx->nSamplesPerSec;
	pvscc->MinimumSampleFrequency=pwfx->nSamplesPerSec;
	pvscc->SampleFrequencyGranularity=pwfx->nSamplesPerSec;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CVCamAudioStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CVCamAudioStream::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void *pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void *pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD *pcbReturned     // Return the size of the property.
)
{
    if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
    if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;
    
    if (pcbReturned) *pcbReturned = sizeof(GUID);
    if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
    if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.
        
    *(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CVCamAudioStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
    return S_OK;
}

//
// DoBufferProcessingLoop
//
// Grabs a buffer and calls the users processing function.
// Overridable, so that different delivery styles can be catered for.
HRESULT CVCamAudioStream::DoBufferProcessingLoop(void) {

    Command com=Command::CMD_INIT;

    OnThreadStartPlay();
	timeBeginPeriod(1);
	
	DWORD t0 = 0, t1,ts=0,tt=0;
	t0 = timeGetTime();
	int ii = 0;
	m_lastFn = 0;
	FILTER_STATE oldState = m_pFilter->m_State;

    do {
		while (!CheckRequest(&com)) {
			IMediaSample *pSample;
			
			{
				t1 = timeGetTime();
				if (t1 - (t0+ts) < (ii*(1000 / 25))) { Sleep(1); continue; }
				
				ii++;
			}

			HRESULT hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
			if (FAILED(hr)) {
				Sleep(1);
				continue;	// go round again. Perhaps the error will go away
						// or the allocator is decommited & we will be asked to
						// exit soon.
			}


			hr = FillBuffer(pSample);

			

	    if (hr == S_OK) {
			hr = Deliver(pSample);
			pSample->Release();
            // downstream filter returns S_FALSE if it wants us to
            // stop or an error if it's reporting an error.
            if(hr != S_OK)
            {
				return S_OK;
			}
	    } else if (hr == 2) {
			pSample->Release();Sleep(1);
		} else if (hr == S_FALSE) {
            // derived class wants us to stop pushing data
			pSample->Release();
			DeliverEndOfStream();
			return S_OK;
	    } else {
            // derived class encountered an error
            pSample->Release();
			DbgLog((LOG_ERROR, 1, TEXT("Error %08lX from FillBuffer!!!"), hr));
            DeliverEndOfStream();
            m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
			return S_OK;
		}

        // all paths release the sample
		Sleep(1);
	}

        // For all commands sent to us there must be a Reply call!
	if (com == CMD_RUN || com == CMD_PAUSE) {
	    Reply(NOERROR);
	} else if (com != CMD_STOP) {
	    Reply((DWORD) E_UNEXPECTED);
	    DbgLog((LOG_ERROR, 1, TEXT("Unexpected command!!!")));
	}
    } while (com != CMD_STOP);
    return S_FALSE;
}

