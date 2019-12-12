#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include<refclock.h>
#include "filters.h"
 
CUnknown * WINAPI CVCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);
    CUnknown *punk = new CVCam(lpunk, phr);
    return punk;
}

CVCam::CVCam(LPUNKNOWN lpunk, HRESULT *phr) : 
    CSource(NAME(VIDEO_NAME1), lpunk, CLSID_VirtualCam)
{
    ASSERT(phr); 
    CAutoLock cAutoLock(&m_cStateLock);
    // Create the one and only output pin
    m_paStreams = (CSourceStream **) new CSourceStream*[2]; 
    m_paStreams[0] = new CVCamStream(phr, this, L"video out");
	m_paStreams[1] = new CVCamAudioStream(phr, this, L"audio out");
}

HRESULT CVCam::QueryInterface(REFIID riid, void **ppv)
{
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if(riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
        return m_paStreams[0]->QueryInterface(riid, ppv);
    else
        return CSource::QueryInterface(riid, ppv);
}

 

//////////////////////////////////////////////////////////////////////////
// CVCamStream is the one and only output pin of CVCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CVCamStream::CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName) :
    CSourceStream(NAME(VIDEO_NAME1),phr, pParent, pPinName), m_pParent(pParent),fps(25.0f)
{
	
	CMediaType *pmt=&m_mt;
	DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));
	//pvi->dwInterlaceFlags=AMINTERLACE_IsInterlaced|AMINTERLACE_Field1First;

	pvi->bmiHeader.biCompression = MAKEFOURCC('U','Y','V','Y');
	//pvi->dwPictAspectRatioX=16;		pvi->dwPictAspectRatioY=9;

	pvi->bmiHeader.biBitCount    = 16;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biWidth      = 1920;
    pvi->bmiHeader.biHeight     = 1080;
    pvi->bmiHeader.biPlanes     = 1;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

	fps = 25;
	pvi->AvgTimePerFrame = UNITS/(25);//25 fps

	pvi->AvgTimePerFrame = UNITS/fps;//25 fps
	BITMAPINFOHEADER bih2={sizeof(bih2),1920,1080,1,16,MAKEFOURCC('U','Y','V','Y'),0,0,0,0,0};
	bih2.biSizeImage  = GetBitmapSize(&bih2);
	pvi->bmiHeader=bih2;
	
	timeBeginPeriod(1);

	hTick = CreateEvent(0, 0, 0, 0);
	hEvent = timeSetEvent(1, 0, (LPTIMECALLBACK)hTick, (DWORD_PTR)this, TIME_PERIODIC | TIME_CALLBACK_EVENT_SET | TIME_KILL_SYNCHRONOUS);



    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle
	pvi->dwBitErrorRate=0;
	pvi->dwBitRate=pvi->bmiHeader.biWidth*pvi->bmiHeader.biHeight*pvi->bmiHeader.biBitCount*(25.0f);

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
	pmt->SetSubtype(&MEDIASUBTYPE_UYVY);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

    l1=pvi->bmiHeader.biWidth*pvi->bmiHeader.biHeight*pvi->bmiHeader.biBitCount/8;
	



}

CVCamStream::~CVCamStream()
{
	timeKillEvent(hEvent);
	CloseHandle(hTick);
} 

HRESULT CVCamStream::QueryInterface(REFIID riid, void **ppv)
{   
    // Standard OLE stuff
    if(riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if(riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
    else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef();
    return S_OK;
}



HRESULT CVCamStream::FillBuffer(IMediaSample *pms)
{
	REFERENCE_TIME avgFrameTime = UNITS/(fps);//25.0f;//((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;


	//((CBaseFilter*)m_pFilter)->StreamTime(rtNow);
	//rtNow+=avgFrameTime;

	
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
	if(false) {

		CMediaType *pmt=&m_mt;
		ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

		pvi->bmiHeader.biCompression = BI_RGB;

		pvi->bmiHeader.biBitCount    = 32;
		pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
		pvi->bmiHeader.biWidth      = VIDEO_WIDTH;
		pvi->bmiHeader.biHeight     = VIDEO_HEIGHT;
		pvi->bmiHeader.biPlanes     = 1;
		pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
		pvi->bmiHeader.biClrImportant = 0;

		fps = VIDEO_FPS;
		pvi->AvgTimePerFrame = UNITS/(fps);//25 fps

		pvi->dwBitRate=pvi->bmiHeader.biWidth*pvi->bmiHeader.biHeight*pvi->bmiHeader.biBitCount*fps;

		const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
		pmt->SetSubtype(&SubTypeGUID);
		pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

		l1=pvi->bmiHeader.biWidth*pvi->bmiHeader.biHeight*pvi->bmiHeader.biBitCount/8;
		
		pms->SetMediaType(&m_mt);
	}

    rtNow = m_rtLastTime;
	m_rtLastTime = rtNow + avgFrameTime;



	__int64 f0=fc64;fc64++;
	
	pms->SetTime(&rtNow.m_time, &m_rtLastTime);
	pms->SetMediaTime(&f0,&fc64);
	
	pms->SetPreroll(false);
	
	pms->SetActualDataLength(l1);
	pms->SetDiscontinuity(fc==0 || m_Discontinuity);//fc%100==0?TRUE:FALSE
	m_Discontinuity = false;

    pms->SetSyncPoint(TRUE);

//	pms->SetDiscontinuity(fc % 100 == 0 ? TRUE : FALSE);	pms->SetSyncPoint(fc % 100 == 0 ? TRUE : FALSE);

	fc++; 

    BYTE *pData;
    pms->GetPointer(&pData);

		
	for (int f = 0; f < pms->GetSize()/4;f++) ((DWORD*)(pData))[f]=0x007f007f;
	//memset(pData+(rand()%1440000),255,720*2); 

	return NOERROR;
} // FillBuffer


//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CVCamStream::Notify(IBaseFilter * pSender, Quality q)
{
	//if(q.Type==Famine && q.Late>0){		m_rtLastTime+=q.Late;	}
    return S_OK;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CVCamStream::SetMediaType(const CMediaType *pmt)
{
//    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CVCamStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 0) return VFW_S_NO_MORE_ITEMS;

	*pmt=m_mt;
    return NOERROR;

} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CVCamStream::CheckMediaType(const CMediaType *pMediaType)
{
//    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());
    if(*pMediaType != m_mt)         return E_INVALIDARG;
    return S_OK;
} // CheckMediaType



// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CVCamStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 2;
    pProperties->cbBuffer = max(1920*1080*2,pvi->bmiHeader.biSizeImage);

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);

    if(FAILED(hr)) 
		return hr;
    if(Actual.cbBuffer < pProperties->cbBuffer) 
		return E_FAIL;
    return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CVCamStream::OnThreadCreate()
{
    m_rtLastTime = 0*UNITS/25;fc64=0;
	fc=0;
	lastTime = 0;
	rtNow.m_time=0;
	//buf->vidRt=0;


	//if (m_pFilter && m_pFilter->m_pClock) 		(CBaseFilter*)m_pFilter->m_pClock->GetTime(&((CBaseFilter*)m_pFilter)->m_tStart.m_time);
	//else ((CBaseFilter*)m_pFilter)->m_tStart.m_time=(REFERENCE_TIME)timeGetTime()*10000;


	return NOERROR;
} // OnThreadCreate


//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////


HRESULT STDMETHODCALLTYPE CVCamStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
//    DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
    //m_mt = *pmt;//flash sets this to corrupted value???
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    *piCount = 1;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
    *pmt = CreateMediaType(&m_mt);
	if(!(*pmt)->pbFormat) (*pmt)->pbFormat=(LPBYTE)CoTaskMemAlloc(sizeof(VIDEOINFOHEADER));
    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);
	
	int w=VIDEO_WIDTH,h=VIDEO_HEIGHT,bpp=16;float fps=25.0f;
	
	fps = VIDEO_FPS;
	pvi->AvgTimePerFrame = UNITS / fps;
	BITMAPINFOHEADER bih2={sizeof(bih2),w,h,1,bpp,MAKEFOURCC('U','Y','V','Y'),0,0,0,0,0};
	bih2.biSizeImage  = GetBitmapSize(&bih2);
	pvi->bmiHeader=bih2;

	SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle
	pvi->dwBitErrorRate=0;
	pvi->dwBitRate=w*h*bpp*fps;

    (*pmt)->majortype = MEDIATYPE_Video;
    (*pmt)->subtype = MEDIASUBTYPE_UYVY;
    (*pmt)->formattype = FORMAT_VideoInfo;
    (*pmt)->bTemporalCompression = FALSE;
    (*pmt)->bFixedSizeSamples= TRUE;
    (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);
    
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
    
    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = w;
    pvscc->InputSize.cy = h;
    pvscc->MinCroppingSize.cx = w;
    pvscc->MinCroppingSize.cy = h;
    pvscc->MaxCroppingSize.cx = w;
    pvscc->MaxCroppingSize.cy = h;
    pvscc->CropGranularityX = w;
    pvscc->CropGranularityY = h;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize.cx = w;
    pvscc->MinOutputSize.cy = h;
    pvscc->MaxOutputSize.cx = w;
    pvscc->MaxOutputSize.cy = h;
    pvscc->OutputGranularityX = 0;
    pvscc->OutputGranularityY = 0;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;
    pvscc->MinFrameInterval = 200000;   //50 fps
    pvscc->MaxFrameInterval = 50000000; // 0.2 fps
	pvscc->MinBitsPerSecond = (w * h * (bpp/8) * 8) / 5;
    pvscc->MaxBitsPerSecond = w * h * (bpp/8) * 8 * 50;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CVCamStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
    return S_OK;
}

// Get: Return the pin category (our only property). 
HRESULT CVCamStream::Get(
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
HRESULT CVCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
    return S_OK;
}



/*
// Pin become active
public override int Active()
{
    m_rtStart = 0;
    m_bStartNotified = false;
    m_bStopNotified = false;
    {
        lock (m_Filter.FilterLock)
        {
            m_pClock = m_Filter.Clock; // Get's the filter clock
            if (m_pClock.IsValid) // Check if were SetSyncSource called
            {
                m_pClock._AddRef(); // handle instance
                m_hSemaphore = new Semaphore(0,0x7FFFFFFF); // Create semaphore for notify
            }
        }
    }
    return base.Active();
}

// Pin become inactive (usually due Stop calls)
public override int Inactive()
{
    HRESULT hr = (HRESULT)base.Inactive();
    if (m_pClock != null) // we have clock
    {
        if (m_dwAdviseToken != 0) // do we advice
        {
            m_pClock.Unadvise(m_dwAdviseToken); // shutdown advice
            m_dwAdviseToken = 0;
        }
        m_pClock._Release(); // release instance
        m_pClock = null;
        if (m_hSemaphore != null)
        {
            m_hSemaphore.Close(); // delete semaphore
            m_hSemaphore = null;
        }
    }
    return hr;
}
*/
HRESULT CVCamStream::DoBufferProcessingLoop(void) {

	Command com = Command::CMD_INIT;
	OnThreadStartPlay();
	
	DWORD  t0 = 0,t1,ts=0,tt=0, nextResync=timeGetTime();
	t0 = timeGetTime();
	int ii = 0; 

	IBaseFilter *pp = 0;
	m_pFilter->m_pGraph->FindFilterByName(AUDIO_NAME, &pp);
	FILTER_STATE oldState = m_pFilter->m_State;

	
	do {
		while (!CheckRequest(&com)) {
			IMediaSample *pSample;

			//if (m_pFilter->m_State == State_Running) 
			{
				t1 = timeGetTime();
				if (t1 - (t0+ts) < (ii*(1000 / fps))) { Sleep(1); continue; }
				
				m_lastFn = 0;
				
				ii++;
			}

			HRESULT hr = GetDeliveryBuffer(&pSample, NULL, NULL, 0);
			if (FAILED(hr)) {
				Sleep(1);
				//{	FILE*flog = fopen("c:\\temp\\virt.log", "at"); fprintf(flog, "GEtDeliveryBuffer returned %lx\n",hr); fclose(flog);		}
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
				if (hr != S_OK)
				{
					//{	FILE*flog = fopen("c:\\temp\\virt.log", "at"); fprintf(flog, "exit after deliver returned %lx\n",hr); fclose(flog);		}
					return S_OK;
				}
				else {
					//{	static DWORD tt = 0; FILE*flog = fopen("c:\\temp\\virt.log", "at"); fprintf(flog, "%ld: deliver ok after %d ms\n", timeGetTime(), timeGetTime()-tt); fclose(flog); tt = timeGetTime();		}
				}
			}
			else if (hr == S_FALSE) {
				// derived class wants us to stop pushing data
				pSample->Release();
				DeliverEndOfStream();
				//{	FILE*flog = fopen("c:\\temp\\virt.log", "at"); fprintf(flog, "exit after FillBuffer returned %lx\n", hr); fclose(flog);		}
				return S_OK;
			}
			else {
				// derived class encountered an error
				//{	FILE*flog = fopen("c:\\temp\\virt.log", "at"); fprintf(flog, "exit after FillBuffer returned %lx\n", hr); fclose(flog);		}
				pSample->Release();
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
		}
		else if (com != CMD_STOP) {
			Reply((DWORD)E_UNEXPECTED);
		}
	} while (com != CMD_STOP);
	//{	FILE*flog = fopen("c:\\temp\\virt.log", "at"); fprintf(flog, "exit ok\n"); fclose(flog);		}
	return S_FALSE;
}

