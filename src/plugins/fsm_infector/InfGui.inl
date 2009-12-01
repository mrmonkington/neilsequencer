#include "resource.h"
#include <commctrl.h>

mi *gpmi=NULL;

int nLastSrc=0;
int nWaves=0;
int wavexlt[256];

CWaveSource clipboard;
BOOL bClipEnabled=FALSE;

void GetSource(CWaveSource *pSrc)
{
  for (int i=0; i<nWaves; i++)
    if (wavexlt[i]==pSrc->nSampleNo)
    {
      SendMessage(GetDlgItem(hwndGUI,IDC_WAVE),CB_SETCURSEL,i,i);
      break;
    }  
	SendMessage(GetDlgItem(hwndGUI,IDC_WAVELOC),TBM_SETPOS,1,pSrc->nPosition);
	SendMessage(GetDlgItem(hwndGUI,IDC_WAVESTR),TBM_SETPOS,1,pSrc->nStretch);
	SendMessage(GetDlgItem(hwndGUI,IDC_SMOOTHING),TBM_SETPOS,1,pSrc->nSmoothing);
	SendMessage(GetDlgItem(hwndGUI,IDC_CLIP),TBM_SETPOS,1,pSrc->nClip);
	SendMessage(GetDlgItem(hwndGUI,IDC_BENDING),TBM_SETPOS,1,pSrc->nBend);
	SendMessage(GetDlgItem(hwndGUI,IDC_GAIN),TBM_SETPOS,1,pSrc->nGain);
}

void SetSource(CWaveSource *pSrc)
{
  pSrc->nSampleNo=wavexlt[SendMessage(GetDlgItem(hwndGUI,IDC_WAVE),CB_GETCURSEL,0,0)];
	pSrc->nPosition=SendMessage(GetDlgItem(hwndGUI,IDC_WAVELOC),TBM_GETPOS,0,0);
	pSrc->nStretch=SendMessage(GetDlgItem(hwndGUI,IDC_WAVESTR),TBM_GETPOS,0,0);
	pSrc->nSmoothing=SendMessage(GetDlgItem(hwndGUI,IDC_SMOOTHING),TBM_GETPOS,0,0);
	pSrc->nClip=SendMessage(GetDlgItem(hwndGUI,IDC_CLIP),TBM_GETPOS,0,0);
	pSrc->nBend=SendMessage(GetDlgItem(hwndGUI,IDC_BENDING),TBM_GETPOS,0,0);
	pSrc->nGain=SendMessage(GetDlgItem(hwndGUI,IDC_GAIN),TBM_GETPOS,0,0);
}

void Generate(HWND hWnd)
{
	int nWaveSel=SendMessage(GetDlgItem(hWnd,IDC_WAVE),CB_GETCURSEL,0,0);
	if (nWaveSel==-1) return;
	int nWave=wavexlt[nWaveSel];

  int nSlot=SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_GETCURSEL,0,0);
  const CWaveLevel *pLevel=gpmi->pCB->GetWaveLevel(nWave,0);
	if (!pLevel)
		return;
	if (pLevel->numSamples<65) return;
  int nSize=pLevel->numSamples;
  if (nSize>2048) nSize=2048;
  int nSmooth=SendMessage(GetDlgItem(hwndGUI,IDC_SMOOTHING),TBM_GETPOS,0,0);
  int nBend=SendMessage(GetDlgItem(hwndGUI,IDC_BENDING),TBM_GETPOS,0,0);
  int nClip=SendMessage(GetDlgItem(hwndGUI,IDC_CLIP),TBM_GETPOS,0,0);
  int nGain=SendMessage(GetDlgItem(hwndGUI,IDC_GAIN),TBM_GETPOS,0,0);
  float fInc=(float)(pow(2.0,-SendMessage(GetDlgItem(hWnd,IDC_WAVESTR),TBM_GETPOS,0,0)/(12.0*16.0))*nSize/2048);
	float fStart=(float)SendMessage(GetDlgItem(hWnd,IDC_WAVELOC),TBM_GETPOS,0,0);
	fStart=fStart*(pLevel->numSamples-nSize)/1000;
	if (fStart>(pLevel->numSamples-nSize))
		fStart=float(pLevel->numSamples-nSize);
  float amp=(float)pow(2.0,0.5*(nGain-75)/5.0);
	for (int i=0; i<2048; i++)
	{
		double phase=i/2048.0;
    phase=fmod(phase+nBend*sin(2*PI*phase)/25,1.0f);
    while(phase<0.0f) phase+=1.0f;
    double pt=fStart+fInc*2048*phase;
    float val=INTERPOLATE((float)fmod(pt,1.0f),pLevel->pSamples[int(pt)],pLevel->pSamples[int(pt+1)]);
    val=val*amp;
    val=float((nClip*atan(val/32767.0*nClip*128/100)*32767/(PI/2)+(100-nClip)*val)/100);
//    if (val<-32767) val=-32767;
//    if (val>32767) val=32767;
		gpmi->userwaves[nSlot][i]=val;
	}
  
  CBiquad bq;
  bq.rbjLPF(float(22050*pow(1/128.0,nSmooth/100.0)),float(sqrt(2)/2),44100.0f);
  for (i=0; i<2048; i++)
    bq.ProcessSample(gpmi->userwaves[nSlot][i]);
  for (i=0; i<2048; i++)
    gpmi->userwaves[nSlot][i]=bq.ProcessSample(gpmi->userwaves[nSlot][i]);

  float sum=0.0f;
  for (i=0; i<2048; i++)
    sum+=gpmi->userwaves[nSlot][i];
  for (i=0; i<2048; i++)
  {
    float &val=gpmi->userwaves[nSlot][i];
    val-=sum/2048;
    if (val<-32767) val=-32767;
    if (val>32767) val=32767;
  }

	gpmi->GenerateUserWaves(nSlot);
  ::InvalidateRect(hWnd,NULL,FALSE);
  ::UpdateWindow(hWnd);
}

void AutoSelectPeriod()
{
	int nWaveSel=SendMessage(GetDlgItem(hwndGUI,IDC_WAVE),CB_GETCURSEL,0,0);
	if (nWaveSel==-1) return;
	int nWave=wavexlt[nWaveSel];
  const CWaveLevel *pLevel=gpmi->pCB->GetWaveLevel(nWave,0);
	if (!pLevel)
		return;
  int nSize=pLevel->numSamples;
  if (nSize>2048) nSize=2048;
	float fStart=(float)SendMessage(GetDlgItem(hwndGUI,IDC_WAVELOC),TBM_GETPOS,0,0);
	fStart=fStart*(pLevel->numSamples-nSize)/1000;
	if (fStart>(pLevel->numSamples-nSize))
		gpmi->pCB->MessageBox("Sample too short");
  int nBest=-1;
  double fCorr=999999999.0;
  for (int i=8; i<256; i++)
  {
    if (fStart+2*i>pLevel->numSamples)
      break;
    int nPtr=(int)fStart;
    double fThis=0.0;
    for (int j=0; j<i; j++)
    {
      double val=pLevel->pSamples[nPtr+i+j]-pLevel->pSamples[nPtr+j];
      fThis+=val*val;
    }
//    fThis/=i;
    if (fThis<fCorr)
      nBest=i, fCorr=fThis;
  }
  double v1=double(nBest)/nSize;
  double v2=-log(v1)/log(2.0);
  float fInc=(float)(pow(2.0,-SendMessage(GetDlgItem(hwndGUI,IDC_WAVESTR),TBM_GETPOS,0,0)/(12.0*16.0))*nSize);
  SendMessage(GetDlgItem(hwndGUI,IDC_WAVESTR),TBM_SETPOS,1,long(12*16*v2));
}

RECT rectStatic;

int CALLBACK MyDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
  PAINTSTRUCT ps;
	switch(uMsg)
	{
  case WM_PAINT:
    {
      HBRUSH hbr,holdbr;
      HPEN hpen,holdpen;

      hbr=(HBRUSH)::GetStockObject(BLACK_BRUSH);
      hpen=::CreatePen(PS_SOLID,1,RGB(0,255,0));

      ::BeginPaint(hWnd,&ps);
      holdbr=(HBRUSH)::SelectObject(ps.hdc,hbr);
      holdpen=(HPEN)::SelectObject(ps.hdc,hpen);
      ::FillRect(ps.hdc,&rectStatic,hbr);
      ::DrawEdge(ps.hdc,&rectStatic,EDGE_SUNKEN,BF_RECT);
      ::InflateRect(&rectStatic,-1,-1);
      ::MoveToEx(ps.hdc,rectStatic.left,int(rectStatic.top+(rectStatic.bottom-rectStatic.top)*(32768+gpmi->userwaves[nLastSrc][0])/65536),NULL);
      for (int i=1; i<rectStatic.right-rectStatic.left; i++)
      {
        int y=int(rectStatic.top+(rectStatic.bottom-rectStatic.top)*(32768+gpmi->userwaves[nLastSrc][i*2048/(rectStatic.right-rectStatic.left)])/65536);
        ::LineTo(ps.hdc,rectStatic.left+i,y);        
      }
      ::InflateRect(&rectStatic,1,1);
      ::SelectObject(ps.hdc,holdbr);
      ::DeleteObject(::SelectObject(ps.hdc,holdpen));
      ::EndPaint(hWnd,&ps);
      return 1;
    }
  case WM_INITDIALOG:
    {
      HWND hTmp=GetDlgItem(hWnd,IDC_SCOPE);
      POINT ptStart,ptEnd;
      ::GetWindowRect(hTmp,&rectStatic);
      ptStart.x=rectStatic.left;
      ptStart.y=rectStatic.top;
      ptEnd.x=rectStatic.right;
      ptEnd.y=rectStatic.bottom;
      ::ScreenToClient(hWnd,&ptStart);
      ::ScreenToClient(hWnd,&ptEnd);
      rectStatic.left=ptStart.x, rectStatic.top=ptStart.y;
      rectStatic.right=ptEnd.x, rectStatic.bottom=ptEnd.y;
      ::DestroyWindow(hTmp);
    }
		SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_ADDSTRING,0,(LPARAM)"A");
		SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_ADDSTRING,0,(LPARAM)"A'");
		SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_ADDSTRING,0,(LPARAM)"B");
		SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_ADDSTRING,0,(LPARAM)"B'");
		SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_ADDSTRING,0,(LPARAM)"C");
		SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_ADDSTRING,0,(LPARAM)"C'");
		SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_ADDSTRING,0,(LPARAM)"D");
		SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_ADDSTRING,0,(LPARAM)"D'");
		nWaves=0;
    hwndGUI=hWnd;
		for (i=1; i<200; i++)
		{
      if (!gpmi->pCB->GetWave(i))
        continue;
			char buf[255];
      const char *name=gpmi->pCB->GetWaveName(i);
			if (name)
			{
        const CWaveLevel *pLevel=gpmi->pCB->GetWaveLevel(i,0);
				if (pLevel && pLevel->numSamples>64)
				{
					wavexlt[nWaves++]=i;
					sprintf(buf,"%03d - %s",i,name);
					SendMessage(GetDlgItem(hWnd,IDC_WAVE),CB_ADDSTRING,0,(LPARAM)buf);
				}
			}
		}
		SendMessage(GetDlgItem(hWnd,IDC_WAVELOC),TBM_SETRANGE,1,MAKELONG(0,1000));
		SendMessage(GetDlgItem(hWnd,IDC_WAVESTR),TBM_SETRANGE,1,MAKELONG(0,96*16));
		SendMessage(GetDlgItem(hWnd,IDC_SMOOTHING),TBM_SETRANGE,1,MAKELONG(0,100));
		SendMessage(GetDlgItem(hWnd,IDC_CLIP),TBM_SETRANGE,1,MAKELONG(0,100));
		SendMessage(GetDlgItem(hWnd,IDC_BENDING),TBM_SETRANGE,1,MAKELONG(0,100));
		SendMessage(GetDlgItem(hWnd,IDC_GAIN),TBM_SETRANGE,1,MAKELONG(0,100));
		SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_SETCURSEL,0,0);
    nLastSrc=0;
    GetSource(gpmi->usersources);
		Generate(hWnd);
    ::EnableWindow(GetDlgItem(hWnd,IDC_BTN_PASTE),FALSE);
		return 0;

	case WM_COMMAND:
		if ((HIWORD(wParam))==CBN_SELCHANGE && LOWORD(wParam)==IDC_WAVE)
			Generate(hWnd);
		if ((HIWORD(wParam))==CBN_SELCHANGE && LOWORD(wParam)==IDC_ENTRY)
    {
      SetSource(gpmi->usersources+nLastSrc);
      nLastSrc=SendMessage(GetDlgItem(hWnd,IDC_ENTRY),CB_GETCURSEL,0,0);
			GetSource(gpmi->usersources+nLastSrc);
      ::InvalidateRect(hWnd,NULL,FALSE);
      ::UpdateWindow(hWnd);
    }
    if (LOWORD(wParam)==IDC_BTN_AUTO)
    {
      AutoSelectPeriod();
		  Generate(hWnd);
      ::InvalidateRect(hWnd,NULL,FALSE);
      ::UpdateWindow(hWnd);
    }
    if (LOWORD(wParam)==IDC_BTN_UNDO)
    {
      GetSource(gpmi->usersources+nLastSrc);
		  Generate(hWnd);
      ::InvalidateRect(hWnd,NULL,FALSE);
      ::UpdateWindow(hWnd);
    }
    if (LOWORD(wParam)==IDC_BTN_COPY)
    {
      SetSource(&clipboard);
      ::InvalidateRect(hWnd,NULL,FALSE);
      ::UpdateWindow(hWnd);
      ::EnableWindow(GetDlgItem(hWnd,IDC_BTN_PASTE),TRUE);
    }
    if (LOWORD(wParam)==IDC_BTN_PASTE)
    {
      GetSource(&clipboard);
		  Generate(hWnd);
      ::InvalidateRect(hWnd,NULL,FALSE);
      ::UpdateWindow(hWnd);
    }
		break;
	case WM_HSCROLL:
		Generate(hWnd);
		break;

	case WM_CLOSE:
    SetSource(gpmi->usersources+nLastSrc);
		::EndDialog(hWnd,IDOK);
    hwndGUI=NULL;
		return 1;
	}
	return 0;
}

void mi::Command(int const i)
{
	if (i==0 && !hwndGUI)
	{
		gpmi=this;
		DialogBox(hInstance,"WAVEUI",NULL,MyDlgProc);
	}
	if (i==1)
		pCB->MessageBox("FSM Infector version 0.02982002003 ALPHA !\nWritten by Krzysztof Foltman (kf@cw.pl)\nAdditional ideas by canc3r\nSpecial thx to: oskari, canc3r, Oom, Zephod and all #buzz crew\n\n\n"
			"Visit my homepage at www.mp3.com/psytrance\n(buzz-generated goa trance) and grab my songs ! :-)");
}

