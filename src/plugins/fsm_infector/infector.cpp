// globalny tuning
// zakres dla suwaków (cutoff, resonance, modulation)
// lepszy tryb mono
// sustain 0 -> b³¹d
// startuje -> bzdury
// bug w seq<->buzz

// lokalne/globalne LFO

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <zzub/plugin.h>
#include <iostream>
#pragma optimize ("a", on)
#include "DSPChips.h"

#include "infector.h"

namespace fsm {

static int times[]={
  1,2,3,4,6,8,12,16,24,28,32,48,64,96,128
};

int intsinetable[2048];
float atanTable[8193];
float atanTable2[8193];
float atanTable3[8193];
float atanTable4[8193];
float *atanTables[4]={atanTable,atanTable2,atanTable3,atanTable4};

///////////////////////////////////////////////////////////////////////////////////

CBandlimitedTable sintable;
CBandlimitedTable sawtable;
CBandlimitedTable spstable;
CBandlimitedTable sp2table;
CBandlimitedTable sqrtable;
CBandlimitedTable tritable;
CBandlimitedTable hextable;
CBandlimitedTable nultable;
CBandlimitedTable fm1table;
CBandlimitedTable xt1table;
CBandlimitedTable xt2table;
CBandlimitedTable octtable;
CBandlimitedTable prttable;
CBandlimitedTable pr2table;
CBandlimitedTable pr3table;
CBandlimitedTable spltable;
CBandlimitedTable smntable;

const char *tabnames[]={"Sine","Triangle","SuperSaw","PWM Sqr","Dbl Sqr","Hexagon","Carrot","Onion","Tomato","Cabbage","Cucumber","Octave","1-3","4-6","Regs","Saw","User A","User AA'","User B","User BB'","User C","User CC'","User D","User DD'"};
const char *stabnames[]={"Sine","Tri","Saw","SuperSaw","Sqr","Oct","1-3","4-6","Regs","Hex","FM","XT1","XT2","User A","User A'","User B","User B'","User C","User C'","User D","User D'"};

CBandlimitedTable *tablesA[]={&sintable,&tritable,&spstable,&sawtable,&sqrtable,&hextable,&xt1table,&sawtable,&fm1table,&fm1table,&xt2table,&octtable,&prttable,&pr2table,&pr3table,&spltable};
CBandlimitedTable *tablesB[]={&nultable,&tritable,&sp2table,&sawtable,&sqrtable,&hextable,&xt1table,&sqrtable,&fm1table,&sqrtable,&xt2table,&octtable,&prttable,&pr2table,&pr3table,&smntable};
CBandlimitedTable *tablesC[]={&sintable,&tritable,&sawtable,&spstable,&sqrtable,&octtable,&prttable,&pr2table,&pr3table,&hextable,&fm1table,&xt1table,&xt2table};

void GenerateWaves()
{
	int i;

	for (i=0; i<8193; i++)
	{
		atanTable[i]=float((i-4096)/4096.0);
		atanTable2[i]=float(atan(2*(i-4096)/7000.0)*2/PI);
		atanTable3[i]=float(atan(2*sin(3*(i-4096)/7000.0)+2*(i-4096)/7000.0)/PI);
		atanTable4[i]=float(sin(sin(3*(i-4096)/7000.0)+(i-4096)/7000.0)/2);
	}

//  for (i=0; i<2048; i++)
//    intsinetable[i]=int(32768*sin(i*PI/1024));
  for (i=0; i<2048; i++)
	{
		float phs=float((i-1024)/1024.0);
    intsinetable[i]=int(32768*(2*phs*phs-1));
	}
/*	for (i=0; i<1024; i++)
	{
		double phs=(i-512)/512.0;
		intsinetable[i]=(int)(32767*(phs*phs-1));
		intsinetable[i+1024]=-(int)(32767*(phs*phs-1));
	}*/
/*
	for (i=0; i<512; i++)
	{
		double phs=i/512.0;
		intsinetable[i]=int(32767*phs);
		intsinetable[i+512]=int(32767*(1-phs));
		intsinetable[i+1024]=int(32767*(-phs));
		intsinetable[i+1536]=int(32767*(phs-1));
	}
	*/

/*
  for (i=0; i<1024; i++)
    intsinetable[i]=int(32768*i/1024.0);
  for (i=1024; i<2048; i++)
    intsinetable[i]=int(32768*(2048-i)/1024.0);
    */

  float triwave[2048];
  for (i=0; i<512; i++)
	{
    triwave[i]=float(32000*(i/512.0));
    triwave[i+512]=float(32000*(1-i/512.0));
    triwave[i+1024]=float(-32000*(i/512.0));
    triwave[i+1536]=float(-32000*(1-i/512.0));
	}
  tritable.m_pBuffer=triwave;
  tritable.m_nBufSize=2048;
  tritable.Make(2,0.25);

  float nulwave[2048];
  for (i=0; i<2048; i++)
    nulwave[i]=0.0f;
  nultable.m_pBuffer=nulwave;
  nultable.m_nBufSize=2048;
  nultable.Make(16,0.5);

  float sinwave[2048];
  for (i=0; i<2048; i++)
    sinwave[i]=float(32000*sin(i/1024.0*PI));
  sintable.m_pBuffer=sinwave;
  sintable.m_nBufSize=2048;
  sintable.Make(8.0f,0.5f);

  float sawwave[2048];
  for (i=0; i<2048; i++)
    sawwave[i]=float(32000*(i/1024.0-1.0));
  sawtable.m_pBuffer=sawwave;
  sawtable.m_nBufSize=2048;
  sawtable.Make(1.1f,0.25f);

	float splwave[2048];
  for (i=0; i<2048; i++)
		splwave[i]=float(i<1024?32000-32000*(i/1024.0):0);
  spltable.m_pBuffer=splwave;
  spltable.m_nBufSize=2048;
  spltable.Make(1.08f,0.25f);

	float smnwave[2048];
  for (i=0; i<2048; i++)
		smnwave[i]=float(i<1024?32000*(i/1024.0):0);
  smntable.m_pBuffer=smnwave;
  smntable.m_nBufSize=2048;
  smntable.Make(1.1f,0.25f);

	float spswave[2048];
  for (i=0; i<2048; i++)
    spswave[i]=float(32000*(0.125*fmod(4*i/1024.0+1.0,2.0)+0.125*fmod(2*i/1024.0+1.0,2.0)+0.750*fmod(i/1024.0+1.0,2.0)-1.0));
  spstable.m_pBuffer=spswave;
  spstable.m_nBufSize=2048;
  spstable.Make(1.3f,0.25f);

  float sp2wave[2048];
  for (i=0; i<2048; i++)
    sp2wave[i]=float(-32000*(0.125*fmod(4*i/1024.0+1.0,2.0)+0.275*fmod(2*i/1024.0+1.0,2.0)+0.600*fmod(i/1024.0+1.0,2.0)-1.0));
  sp2table.m_pBuffer=sp2wave;
  sp2table.m_nBufSize=2048;
  sp2table.Make(1.3f,0.25f);

  float fm1wave[2048];
  for (i=0; i<2048; i++)
    fm1wave[i]=float(32000*sin((i-1024)*PI/2048+PI*cos((i-1024)*PI*4/2048)));
  fm1table.m_pBuffer=fm1wave;
  fm1table.m_nBufSize=2048;
  fm1table.Make(1.5f,0.25f);

  float xt1wave[2048];
  for (i=0; i<2048; i++)
    xt1wave[i]=float(32000*sin(i*4*PI/1024*sin(i*PI/1024)));
  xt1table.m_pBuffer=xt1wave;
  xt1table.m_nBufSize=2048;
  xt1table.Make(1.5f,0.25f);

  float xt2wave[2048];
  for (i=0; i<2048; i++)
    xt2wave[i]=float(32000*sin(i*2*PI/1024*(sin(i*3*PI/1024)+sin(i*4*PI/1024))));
  xt2table.m_pBuffer=xt2wave;
  xt2table.m_nBufSize=2048;
  xt2table.Make(1.5f,0.25f);

  float octwave[2048];
  for (i=0; i<2048; i++)
  {
    double sum=0;
    for (int j=1; j<8; j++)
      sum+=sin((1<<j)*i*2*PI/1024);
    octwave[i]=(float)(32000*sum/7);
  }
  octtable.m_pBuffer=octwave;
  octtable.m_nBufSize=2048;
  octtable.Make(2.0f,0.25f);

  float prtwave[2048];
  for (i=0; i<2048; i++)
  {
    double sum=0;
    for (int j=1; j<=3; j++)
      sum+=sin(j*i*2*PI/1024);
    prtwave[i]=(float)(32000*sum/3);
  }
  prttable.m_pBuffer=prtwave;
  prttable.m_nBufSize=2048;
  prttable.Make(4.0f,0.25f);

  float pr2wave[2048];
  for (i=0; i<2048; i++)
  {
    double sum=0;
    for (int j=3; j<=6; j++)
      sum+=sin(j*i*2*PI/1024);
    pr2wave[i]=(float)(32000*sum/3);
  }
  pr2table.m_pBuffer=pr2wave;
  pr2table.m_nBufSize=2048;
  pr2table.Make(4.0f,0.25f);

  float pr3wave[2048];
  for (i=0; i<2048; i++)
  {
    static int partials[8]={1,2,3,4,6,8,12,16};
    double sum=0;
    for (int j=0; j<8; j++)
      sum+=sin(partials[j]*i*2*PI/1024);
    pr3wave[i]=(float)(32000*sum/3);
  }
  pr3table.m_pBuffer=pr3wave;
  pr3table.m_nBufSize=2048;
  pr3table.Make(4.0f,0.25f);

  float hexwave[2048];
  for (i=0; i<256; i++)
	{
    hexwave[i]=(float)(32000*(i/256.0));
    hexwave[i+256]=(float)32000;
    hexwave[i+512]=(float)32000;
    hexwave[i+768]=(float)(32000*(1-i/256.0));
    hexwave[i+1024]=(float)(-32000*(i/256.0));
    hexwave[i+1280]=(float)-32000;
    hexwave[i+1536]=(float)-32000;
    hexwave[i+1792]=(float)(-32000*(1-i/256.0));
	}
  hextable.m_pBuffer=hexwave;
  hextable.m_nBufSize=2048;
  hextable.Make(1.64f,0.25f);

  float sqrwave[2048];
  for (i=0; i<2048; i++)
    sqrwave[i]=float(i<1024?-32000:32000);
  sqrtable.m_pBuffer=sqrwave;
  sqrtable.m_nBufSize=2048;
  sqrtable.Make(1.1f,0.25);
}

/*
HINSTANCE hInstance;

BOOL __stdcall DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason==DLL_PROCESS_ATTACH)
  {
		hInstance=hInst;
    GenerateWaves();
  }
	return TRUE;
}
*/

///////////////////////////////////////////////////////////////////////////////////

const zzub::parameter *paraWaveformA = 0;
const zzub::parameter *paraPWMRateA = 0;
const zzub::parameter *paraPWMRangeA = 0;
const zzub::parameter *paraPWOffsetA = 0;
const zzub::parameter *paraWaveformB = 0;
const zzub::parameter *paraPWMRateB = 0;
const zzub::parameter *paraPWMRangeB = 0;
const zzub::parameter *paraPWOffsetB = 0;
const zzub::parameter *paraTranspose = 0;
const zzub::parameter *paraDetune = 0;
const zzub::parameter *paraOscMix = 0;
const zzub::parameter *paraSubOscWave = 0;
const zzub::parameter *paraSubOscvolume = 0;
const zzub::parameter *paraGlide = 0;
const zzub::parameter *paraFilterType = 0;
const zzub::parameter *paraFilterCutoff = 0;
const zzub::parameter *paraFilterResonance = 0;
const zzub::parameter *paraFilterModulation = 0;
const zzub::parameter *paraFilterAttack = 0;
const zzub::parameter *paraFilterDecay = 0;
const zzub::parameter *paraFilterSustain = 0;
const zzub::parameter *paraFilterRelease = 0;
const zzub::parameter *paraFilterShape = 0;
const zzub::parameter *paraFilterInertia = 0;
const zzub::parameter *paraFilterTrack = 0;
const zzub::parameter *paraLFORate = 0;
const zzub::parameter *paraLFOAmount1 = 0;
const zzub::parameter *paraLFOAmount2 = 0;
const zzub::parameter *paraLFOShape = 0;
const zzub::parameter *paraLFO2Rate = 0;
const zzub::parameter *paraLFO2Amount1 = 0;
const zzub::parameter *paraLFO2Amount2 = 0;
const zzub::parameter *paraLFO2Shape = 0;
const zzub::parameter *paraAmpAttack = 0;
const zzub::parameter *paraAmpDecay = 0;
const zzub::parameter *paraAmpSustain = 0;
const zzub::parameter *paraAmpRelease = 0;
const zzub::parameter *paraLFOMode = 0;
const zzub::parameter *paraNote = 0;
const zzub::parameter *paraVelocity = 0;
const zzub::parameter *paraLength = 0;
const zzub::parameter *paraCommand1 = 0;
const zzub::parameter *paraArgument1 = 0;
const zzub::parameter *paraCommand2 = 0;
const zzub::parameter *paraArgument2 = 0;

const zzub::attribute *attrMIDIChannel = 0;
const zzub::attribute *attrMIDIVelocity = 0;
const zzub::attribute *attrHighQuality = 0;
const zzub::attribute *attrCrispness = 0;
const zzub::attribute *attrTheviderness = 0;
const zzub::attribute *attrGlobalTuning = 0;
const zzub::attribute *attrClipTable = 0;

void fsm_infector::GenerateUserWaves(int nSlot)
{
	static float CrispnessValues[4]={2.0f,1.7f,1.5f,1.3f};
	usertables[nSlot].m_pBuffer=userwaves[nSlot];
	usertables[nSlot].m_nBufSize=2048;
	usertables[nSlot].Make(CrispnessValues[aval.crispness],0.25f);
}

short const *fsm_infector::GetOscillatorTab(int const waveform)
{
  return NULL;
//  if (waveform<5) return _host->get_oscillator_table(waveform);
//  return extwaves[waveform-5];
}

fsm_infector::fsm_infector()
{
	global_values = &gval;
	track_values = tval;
	attributes = (int *)&aval;
  for (int i=0; i<38; i++)
    ((unsigned char *)&gvalAct)[i]=fsm_infector_info.global_parameters[i]->value_default;
  for (int c=0; c<8; c++)
    for (int i=0; i<2048; i++)
  		userwaves[c][i]=0.0f,

  CurCutoff=64.0f;
	CurRes=64.0f;

	aval.crispness=0;
	nCurChannel=0;
	for (int c=0; c<8; c++)
    GenerateUserWaves(c);
  numTracks=0;
  Osc1PWM.m_nPhase=0;
  Osc2PWM.m_nPhase=0;
}

fsm_infector::~fsm_infector()
{

}

const char * fsm_infector::describe_value(int const param, int const value)
{
  static char txt[36];

  switch(param)
	{
	case 13:		// OSC A pw offset
	case 20:		// OSC B pw offset
	case 22:
	case 35:
		sprintf(txt,"%d%%",value*100/240);
		break;
	case 3:		// OSC A pw offset
	case 7:		// OSC B pw offset
		sprintf(txt,"%d%%",(value-120)*100/120);
		break;
	case 0:		// OSC A
	case 4:		// OSC B
		strcpy(txt,tabnames[value]);
		break;
  case 8:
    sprintf(txt,"%d#",(value-36));
    break;
  case 9:
    sprintf(txt,"%d ct",value*100/240);
    break;
  case 10:
    sprintf(txt,"%0.1f%%:%0.1f%%",(240-value)*100.0/240,value*100.0/240);
    break;
	case 11:		// SubOsc
		strcpy(txt,stabnames[value]);
		break;

  case 14: // filter type
		C6thOrderFilter::GetFilterDesc(txt,value);
    break;

  case 12: // glide
  case 15: // filter cutoff/resonance
  case 16:
    sprintf(txt,"%d %%",value*100/240);
    break;
  case 17:
    sprintf(txt,"%d %%",(value-120)*200/120);
    break;
  case 24:
    sprintf(txt,"%d ct",(value-120)*200/120);
    break;

  case 19:
  case 21:
  case 34:
  case 36:
    sprintf(txt, "%5.3f s", (double)GETENVTIME(value));
    break;
  case 18:
  case 33:
    sprintf(txt, "%5.3f s", (double)GETENVTIME2(value));
    break;
  case 40:
		if (value<240)
			sprintf(txt, "%0.2f tick", (double)value/16.0);
		else
			strcpy(txt,"infinite");
    break;
  case 41:
	case 43:
    switch(value)
		{
		case 1: return "PortaUp";
		case 2: return "PortaDown";
		case 3: return "TonePorta";
		case 0x0C: return "LFO Phase";
		case 4: return "Set Vib";
		case 5: return "3-Arpeggio";
		case 6: return "2-Arpeggio";
		case 0xE5: return "Finetune";
		case 0xE9: return "Retrig";
		case 0xED: return "Delay";
		case 0xFD: return "Reset Trk";
		case 0xFE: return "Reset All";
		default: return "unused";
		};

  case 25:		// LFO rate
  case 29:		// LFO rate
		if (value<240)
      sprintf(txt, "%5.3f Hz", (double)LFOPAR2TIME(value));
    else
      sprintf(txt, "%d ticks", times[value-240]);
		break;
  case 26:
  case 27:
  case 30:
  case 31:
    sprintf(txt,"%d %%",(value-120)*100/120);
    break;
  case 28: // LFO shape
  case 32: // LFO shape
    if (value==0) strcpy(txt,"sine");
    if (value==1) strcpy(txt,"saw up");
    if (value==2) strcpy(txt,"saw down");
    if (value==3) strcpy(txt,"square");
    if (value==4) strcpy(txt,"triangle");
    if (value==5) strcpy(txt,"weird 1");
    if (value==6) strcpy(txt,"weird 2");
    if (value==7) strcpy(txt,"weird 3");
    if (value==8) strcpy(txt,"weird 4");
    if (value==9) strcpy(txt,"steps up");
    if (value==10) strcpy(txt,"steps down");
    if (value==11) strcpy(txt,"upsaws up");
    if (value==12) strcpy(txt,"upsaws down");
    if (value==13) strcpy(txt,"dnsaws up");
    if (value==14) strcpy(txt,"dnsaws down");
    if (value==15) strcpy(txt,"S'n'H 1");
    if (value==16) strcpy(txt,"S'n'H 2");
    break;
	case 37:
		strcpy(txt,"");
		if (value&1) strcat(txt,"L1");  // LFO 1 restart
		if (value&2) strcat(txt,"L2");  // LFO 2 restart
		if (value&4) strcat(txt,"FE");  // Filter Env Lock
		if (value&8) strcat(txt,"AE");  // Amp Envelope Lock
		if (value&16) strcat(txt,"PQ"); // Pitch Quantize
		if (value&32) strcat(txt,"MM"); // Mono Mode
		if (value&64) strcat(txt,"IK"); // Inertia on Keytrack
		break;
	default:
		return NULL;
  }

	return txt;
}

void fsm_infector::stop()
{
  for (int i=0; i<MAX_TRACKS; i++)
    Tracks[i].note=zzub::note_value_off;
  for (int i=0; i<MAX_CHANNELS; i++)
	{
    Channels[i].AmpEnv.NoteOffFast();
    Channels[i].FilterEnv.NoteOffFast();
	}
  Reset();
}

void fsm_infector::process_eventsTrack(CTrack *pt, tvals *ptval)
{
	CChannel *chn=pt->Chn();
	pt->RetrigCount=0;	
	pt->NoTrig=0;
	pt->Arps[0]=0;
	pt->ArpPoint=0;
	pt->ArpCount=1;
	pt->CommandA(ptval->vCommand1, ptval->vParam1);
	pt->CommandA(ptval->vCommand2, ptval->vParam2);
	
	if (ptval->vVelocity!=paraVelocity->value_none)
    pt->accent=ptval->vVelocity;
  if (ptval->vLength!=paraLength->value_none)
    pt->length=ptval->vLength;
  if (ptval->vNote!=paraNote->value_none)
  {
    pt->note=ptval->vNote;
		if (pt->ShuffleAmount && pt->ShuffleMax && pt->ShuffleData[pt->ShuffleCounter])
		{
			pt->RetrigCount=_master_info->samples_per_tick;
			pt->RetrigPoint=pt->RetrigCount-pt->ShuffleData[pt->ShuffleCounter]*pt->ShuffleAmount*_master_info->samples_per_tick/(100*16);
			pt->NoTrig=1;
			pt->RetrigMode=0;
		}
		if (pt->NoTrig==2)
      pt->NotePortFrequency=float((220*pow(2.0,((pt->note-1)>>4)+((pt->note&15)-22-36)/12.0))/_master_info->samples_per_second);
    else
		if (pt->NoTrig==1)
    {
			pt->lastnote=pt->note, pt->lastaccent=pt->accent, pt->lastlength=pt->length;
    }
		else
			pt->PlayNote(pt->note,pt->accent,pt->length,_master_info);
    /*
    if (pt->note==255)
      Channels[0].FilterEnv.NoteOff();
    else
    {
      Channels[0].FilterEnv.NoteOn(pt->accent/120.0);
    }
    */
  }
	else
		if (pt->NoTrig==-1)
			pt->PlayNote(pt->note,pt->accent,pt->length,_master_info);

	pt->CommandB(ptval->vCommand1, ptval->vParam1);
	pt->CommandB(ptval->vCommand2, ptval->vParam2);

	pt->ShuffleCounter++;
	if (pt->ShuffleCounter>=pt->ShuffleMax)
		pt->ShuffleCounter=0;

  if (chn)
  {
    chn->inrKeyTrack.SetInertia((int)(240*sqrt(gvalAct.vFilterInertia/240.0)));
    chn->inrCutoff2.SetInertia(1);
  }
  
  pt->inrLFO1.SetInertia(1);
  pt->inrLFO2.SetInertia(1);
}



void fsm_infector::init(zzub::archive *arc)
{
	numTracks = 1;


	for (int c = 0; c < MAX_TRACKS; c++)
  {
    Tracks[c].pmi=this;
    InitTrack(c);
  }

	for (int c = 0; c < MAX_CHANNELS; c++)
  {
    Channels[c].Reset();
    Channels[c].init();
  }

  //ThisMachine=_host->get_metaplugin();
  if (arc)
  {
	zzub::instream *pi = arc->get_instream("");
	if (pi) {
		int nVersion;
		pi->read(nVersion);
		if (nVersion==1)
		{
		  pi->read(userwaves,sizeof(userwaves));
		  pi->read(usersources,sizeof(usersources));
		  for (int i=0; i<8; i++)
			GenerateUserWaves(i);
		}
		else
		  _host->message("Unsupported waveform data format - download a newer version");
	}
  }

}

void fsm_infector::attributes_changed()
{
	for (int c=0; c<8; c++)
    GenerateUserWaves(c);	
}


void fsm_infector::set_track_count(int count)
{
	if (numTracks < count)
	{
		for (int c = numTracks; c < count; c++)
			InitTrack(c);
	}
	else if (count < numTracks)
	{
		for (int c = count; c < numTracks; c++)
			ResetTrack(c);
	
	}
	numTracks = count;

}

fsm_infector::info fsm_infector::fsm_infector_info;

void fsm_infector::InitTrack(int const i)
{
  if (i)
	{
		Tracks[i].note=zzub::note_value_none;
		Tracks[i].length=Tracks[i-1].length;
		Tracks[i].accent=Tracks[i-1].accent;
		Tracks[i].Reset();
		Tracks[i].init();
	}
	else
	{
		Tracks[i].note=zzub::note_value_none;
		Tracks[i].length=40;
		Tracks[i].accent=224;
		Tracks[i].Reset();
		Tracks[i].init();
	}
}

void fsm_infector::ResetTrack(int const i)
{
}


void fsm_infector::process_events()
{
  for (int i=0; i<38; i++)
    if (((unsigned char *)&gval)[i]!=fsm_infector_info.global_parameters[i]->value_none)
      ((unsigned char *)&gvalAct)[i]=((unsigned char *)&gval)[i];
	
  inrCutoff.SetInertia(gvalAct.vFilterInertia);
  inrResonance.SetInertia(gvalAct.vFilterInertia);
  inrModulation.SetInertia(gvalAct.vFilterInertia);
  inrModShape.SetInertia(gvalAct.vFilterInertia);
  inrLFO1Dest1.SetInertia(gvalAct.vFilterInertia);
  inrLFO1Dest2.SetInertia(gvalAct.vFilterInertia);
  inrLFO2Dest1.SetInertia(gvalAct.vFilterInertia);
  inrLFO2Dest2.SetInertia(gvalAct.vFilterInertia);

  for (int c = 0; c < numTracks; c++)
		process_eventsTrack(&Tracks[c], &tval[c]);
}

#pragma optimize ("a", on) 

#define DO_CLIP(x) ((Distort((x*0.5), theAtanTable, 8192)*2))
//#define DO_CLIP(x) (x)

static bool DoWorkChannel(float *pout, fsm_infector *pmi, int c, CChannel *chn)
{
  bool istracked=(chn->pTrack!=0);
  CTrack *trk=chn->pTrack?chn->pTrack:&pmi->Tracks[MAX_TRACKS+1];
//  bool istracked=false;
//  CTrack *trk=&pmi->Tracks[MAX_TRACKS+1];

  int i;

  float CurCutoff=pmi->inrCutoff.m_fAccum;
  float CurResonance=pmi->inrResonance.m_fAccum;
  float CurModulation=pmi->inrModulation.m_fAccum;
  float CurModShape=pmi->inrModShape.m_fAccum;
  float CurLFO1Dest1=pmi->inrLFO1Dest1.m_fAccum;  // LFO1 -> Cutoff
  float CurLFO1Dest2=pmi->inrLFO1Dest2.m_fAccum;  // LFO1 -> EnvMod
  float CurLFO2Dest1=pmi->inrLFO2Dest1.m_fAccum; // LFO2 -> Cutoff
  float CurLFO2Dest2=pmi->inrLFO2Dest2.m_fAccum; // LFO2 -> Resonance

  float KeyTrack,KeyTrackAmount;

  if (pmi->gvalAct.vLFOMode&64)
  {
    float PitchDeviation=float(log(chn->Frequency*pmi->_master_info->samples_per_second/264.0)/log(pow(2.0,1/12.0))); // semitones
  	KeyTrackAmount=float((pmi->gvalAct.vFilterTrack-120)/60.0);
    KeyTrack=chn->inrKeyTrack.Process(float(KeyTrackAmount*(PitchDeviation/12.0)*240.0/6.0),c); // 240 - zakres cutoffa, 6 - iloœæ oktaw przypadaj¹cych na ca³¹ skalê
    if (chn->AmpEnv.m_nState==4 /*|| (chn->AmpEnv.m_nState>=1 && chn->AmpEnv.m_fLast<1/64.0)*/)
    {
      chn->AmpEnv.ProcessSample(c);
      chn->FilterEnv.ProcessSample(c);
      return false;
    }
  }
  else
  {
  	KeyTrackAmount=chn->inrKeyTrack.Process(float((pmi->gvalAct.vFilterTrack-120)/60.0),c);

    if (chn->AmpEnv.m_nState==4 /*|| (chn->AmpEnv.m_nState>=1 && chn->AmpEnv.m_fLast<1/64.0)*/)
    {
      chn->AmpEnv.ProcessSample(c);
      chn->FilterEnv.ProcessSample(c);
      return false;
    }
    float PitchDeviation=float(log(chn->Frequency*pmi->_master_info->samples_per_second/264.0)/log(pow(2.0,1/12.0))); // semitones
    KeyTrack=float(KeyTrackAmount*(PitchDeviation/12.0)*240.0/6.0); // 240 - zakres cutoffa, 6 - iloœæ oktaw przypadaj¹cych na ca³¹ skalê
  }


	float Frequency=(pmi->gvalAct.vLFOMode&16)?PitchQuantize(chn->Frequency*pmi->_master_info->samples_per_second)/pmi->_master_info->samples_per_second:chn->Frequency;
  if (pmi->aval.tuning || chn->Detune)
    Frequency*=(float)pow(2.0,(pmi->aval.tuning+(chn->Detune*100.0/256))/1200.0);
	float Frequency1=float(pow(2.0,-pmi->gvalAct.vDetune/(24*240.0)+0.5*trk->Vib1Depth*sin(trk->Vib1Phase/pmi->_master_info->samples_per_tick))*Frequency);
  float Frequency2=float(pow(2.0,(pmi->gvalAct.vTranspose-36)/12.0+pmi->gvalAct.vDetune/(24*240.0)+0.5*trk->Vib2Depth*sin(trk->Vib2Phase/pmi->_master_info->samples_per_tick))*Frequency);
  float Frequency3=float(Frequency/2);

  // OSC A  

  // -----
  float fMul=CAnyWaveLevel::WavePositionMultiplier();
  unsigned nCurPhase1=int(fMul*chn->PhaseOSC1);
  unsigned nCurPhase2=int(fMul*chn->PhaseOSC2);
  unsigned nCurPhase3=int(fMul*chn->PhaseOSC3);
  unsigned nCurFrequency1=int(fMul*Frequency1);
  unsigned nCurFrequency2=int(fMul*Frequency2);
  unsigned nCurFrequency3=int(fMul*Frequency3);
  
//	float vel=(chn==pmi->Channels)?chn->Velocity*1.41f:0; // XXXKF
	float vel=chn->Velocity*1.41f;
  int nTime=chn->AmpEnv.GetTimeLeft();

  float *theAtanTable=atanTables[pmi->aval.cliptable];

	// bool bKey=GetKeyState(VK_SHIFT)<0; //XXXKF
	CADSREnvelope *pEnv=&chn->AmpEnv;
  int *pTime=&pEnv->m_nTime;
  {
		int nTab=sizeof(tablesA)/4;
		int nTab2=sizeof(tablesC)/4;
		int nWavA=pmi->gvalAct.vWaveformA;
		CAnyWaveLevel *pLevel1A=(nWavA<nTab ? tablesA[nWavA] : &pmi->usertables[(nWavA-nTab)&~1])->GetTable(Frequency1);
		CAnyWaveLevel *pLevel1B=(nWavA<nTab ? tablesB[nWavA] : &pmi->usertables[nWavA-nTab])->GetTable(Frequency1);
		int nWavB=pmi->gvalAct.vWaveformB;
		CAnyWaveLevel *pLevel2A=(nWavB<nTab ? tablesA[nWavB] : &pmi->usertables[(nWavB-nTab)&~1])->GetTable(Frequency2);
		CAnyWaveLevel *pLevel2B=(nWavB<nTab ? tablesB[nWavB] : &pmi->usertables[nWavB-nTab])->GetTable(Frequency2);
		CAnyWaveLevel *pLevel3;
    if (pmi->gvalAct.vSubOscWave<nTab2)
      pLevel3=tablesC[pmi->gvalAct.vSubOscWave]->GetTable(Frequency3);
    else
      pLevel3=pmi->usertables[pmi->gvalAct.vSubOscWave-nTab2].GetTable(Frequency3);
		int nPWM1Offset=int((1<<30)*float(pmi->gvalAct.vPWOffsetA)/120);
		int nPWM2Offset=int((1<<30)*float(pmi->gvalAct.vPWOffsetB)/120);
		//int nPWM1Period=int(2000000.0/(1.7*pmi->gvalAct.vPWMRateA+10)+1000);
		//int nPWM2Period=int(2000000.0/(1.7*pmi->gvalAct.vPWMRateB+10)+1000);
		// int nPWM1Depth=int(60000.0*32000*pmi->gvalAct.vPWMRangeA/240/* /(nPWM1Period*240)*/);
		// int nPWM2Depth=int(60000.0*32000*pmi->gvalAct.vPWMRangeB/240/* /(nPWM2Period*240)*/);
		int nPWM1,nPWM2;
		int &nPhase=chn->Phase1, &nPhase2=chn->Phase2;
		float fOscMix=pmi->gvalAct.vOscMix/240.0f;
		float fSubOsc=pmi->gvalAct.vSubOscVol/240.0f;

		///////////////////////////////////////////////
		
		int *pPWM1=pmi->PWMBuffer1, nCoeff1=f2i(65536*(Frequency1*pmi->_master_info->samples_per_second)/1000);
		int *pPWM2=pmi->PWMBuffer2, nCoeff2=f2i(65536*(Frequency1*pmi->_master_info->samples_per_second)/1000);
		int s0=0;
		while(s0<c)
		{
			int c0=c;
			int max=c0-s0;

			if (pmi->aval.hq && chn->FilterEnv.m_nState!=4)
			{
				float CutoffA, CutoffB;
				int nCurMax=std::min(max,chn->FilterEnv.m_nStageTime-chn->FilterEnv.m_nTime)*100/50;
				CutoffA=(float)(240*pow(chn->FilterEnv.m_fLast,(CurModShape+1)/241.0));
				do {
					nCurMax=nCurMax*50/100;
					if (chn->FilterEnv.m_nState<1)
						CutoffB=(float)(240*pow(chn->FilterEnv.m_fLast+(chn->FilterEnv.m_fSeries*nCurMax),(CurModShape+1)/241.0));
					else
						CutoffB=(float)(240*pow(chn->FilterEnv.m_fLast*pow(chn->FilterEnv.m_fSeries,nCurMax),(CurModShape+1)/241.0));
				} while(fabs(CutoffB-CutoffA)>(3.0/(pmi->aval.hq+1)) && nCurMax);
				max=std::max(nCurMax,1);
			  if (c0-s0>max)
				  c0=s0+max;
			}

			float Cutoff=float(CurCutoff+KeyTrack+(CurLFO1Dest1-120)*trk->CurLFO+(CurLFO2Dest1-120)*trk->CurLFO2+
				((CurLFO1Dest2-120)*trk->CurLFO+2*(CurModulation-120))*pow((float)(chn->Velocity*chn->FilterEnv.ProcessSample(c0-s0)),
				(float)((CurModShape+1)/241.0)));

			// XXXKF ???
			if (!pmi->aval.hq)
			{
				Cutoff=float((3)*chn->inrCutoff2.Process(float(Cutoff/3),c0-s0));
				max=std::min(max,32);
			}

			CurResonance+=(CurLFO2Dest2-120)*trk->CurLFO2;
			if (CurResonance<0) CurResonance=0;
			if (CurResonance>240) CurResonance=240;

			chn->Filter.CalcCoeffs(pmi->gvalAct.vFilterType,Cutoff,CurResonance,float(pmi->aval.theviderness/50.0f));

			float amp;

			if (pmi->aval.hq>1)
			{
				if (fSubOsc)
				{
					for (i=s0; i<c0; i++)
					{
						amp=pEnv->Next()*vel;

						nPWM1=nPWM1Offset+nCoeff1*(*pPWM1++);
						nPWM2=nPWM2Offset+nCoeff2*(*pPWM2++);
						float osc1 = pLevel1A->GetWaveAt_Cubic(nCurPhase1)-pLevel1B->GetWaveAt_Cubic(nCurPhase1+nPWM1);
						float osc2 =	pLevel2A->GetWaveAt_Cubic(nCurPhase2)-pLevel2B->GetWaveAt_Cubic(nCurPhase2+nPWM2);

						float osc3 =	pLevel3->GetWaveAt_Cubic(nCurPhase3);

						float output = osc1+(osc2-osc1)*fOscMix+osc3*fSubOsc;
						pout[i]+=(float)(DO_CLIP(amp*chn->Filter.ProcessSample(amp*output)));

						nCurPhase1+=nCurFrequency1;
						nCurPhase2+=nCurFrequency2;
						nCurPhase3+=nCurFrequency3;

					}
				}
				else
				{
					for (i=s0; i<c0; i++)
					{
						amp=pEnv->Next()*vel;

	//          nPWM1=nPWM1Offset+(nPWM1Depth*(intsinetable[nPhase]>>16));
	//          nPWM2=nPWM2Offset+(nPWM2Depth*(intsinetable[nPhase]>>16));
						nPWM1=nPWM1Offset+nCoeff1*(*pPWM1++);
						nPWM2=nPWM2Offset+nCoeff2*(*pPWM2++);
						float osc1 = pLevel1A->GetWaveAt_Cubic(nCurPhase1)-pLevel1B->GetWaveAt_Cubic(nCurPhase1+nPWM1);
						float osc2 =	pLevel2A->GetWaveAt_Cubic(nCurPhase2)-pLevel2B->GetWaveAt_Cubic(nCurPhase2+nPWM2);
						float output = osc1+(osc2-osc1)*fOscMix;
						pout[i]+=(float)(DO_CLIP(amp*chn->Filter.ProcessSample(amp*output)));

						nCurPhase1+=nCurFrequency1;
						nCurPhase2+=nCurFrequency2;
					}
				}
			}
			else if (pmi->aval.cliptable)
			{
				if (fSubOsc)
				{
					for (i=s0; i<c0; i++)
					{
						amp=pEnv->Next()*vel;

						nPWM1=nPWM1Offset+nCoeff1*(*pPWM1++);
						nPWM2=nPWM2Offset+nCoeff2*(*pPWM2++);
						float osc1 = pLevel1A->GetWaveAt_Linear(nCurPhase1)-pLevel1B->GetWaveAt_Linear(nCurPhase1+nPWM1);
						float osc2 =	pLevel2A->GetWaveAt_Linear(nCurPhase2)-pLevel2B->GetWaveAt_Linear(nCurPhase2+nPWM2);
						float osc3 =	pLevel3->GetWaveAt_Linear(nCurPhase3);

						float output = osc1+(osc2-osc1)*fOscMix+osc3*fSubOsc;
//						pout[i]+=(float)(DO_CLIP(amp*chn->Filter.ProcessSample(amp*output)));
						pout[i]+=(float)(DO_CLIP(amp*amp*output));

						nCurPhase1+=nCurFrequency1;
						nCurPhase2+=nCurFrequency2;
						nCurPhase3+=nCurFrequency3;
					}
				}
				else
				{
					for (i=s0; i<c0; i++)
					{
						amp=pEnv->Next()*vel;
						nPWM1=nPWM1Offset+nCoeff1*(*pPWM1++);
						nPWM2=nPWM2Offset+nCoeff2*(*pPWM2++);
						float osc1 = pLevel1A->GetWaveAt_Linear(nCurPhase1)-pLevel1B->GetWaveAt_Linear(nCurPhase1+nPWM1);
						float osc2 =	pLevel2A->GetWaveAt_Linear(nCurPhase2)-pLevel2B->GetWaveAt_Linear(nCurPhase2+nPWM2);
						float output = osc1+(osc2-osc1)*fOscMix;
						pout[i]+=(float)(DO_CLIP(amp*chn->Filter.ProcessSample(amp*output)));

						nCurPhase1+=nCurFrequency1;
						nCurPhase2+=nCurFrequency2;
					}
				}
			}
			else
			{
				if (fSubOsc)
				{
					for (i=s0; i<c0; i++)
					{
						amp=pEnv->Next()*vel;

						nPWM1=nPWM1Offset+nCoeff1*(*pPWM1++);
						nPWM2=nPWM2Offset+nCoeff2*(*pPWM2++);
						float osc1 = pLevel1A->GetWaveAt_Linear(nCurPhase1)-pLevel1B->GetWaveAt_Linear(nCurPhase1+nPWM1);
						float osc2 =	pLevel2A->GetWaveAt_Linear(nCurPhase2)-pLevel2B->GetWaveAt_Linear(nCurPhase2+nPWM2);
						float osc3 =	pLevel3->GetWaveAt_Linear(nCurPhase3);

						float output = osc1+(osc2-osc1)*fOscMix+osc3*fSubOsc;
						pout[i]+=(float)(amp*chn->Filter.ProcessSample(amp*output));

						nCurPhase1+=nCurFrequency1;
						nCurPhase2+=nCurFrequency2;
						nCurPhase3+=nCurFrequency3;
					}
				}
				else
				{
					for (i=s0; i<c0; i++)
					{
						amp=pEnv->Next()*vel;
						nPWM1=nPWM1Offset+nCoeff1*(*pPWM1++);
						nPWM2=nPWM2Offset+nCoeff2*(*pPWM2++);
						float osc1 = pLevel1A->GetWaveAt_Linear(nCurPhase1)-pLevel1B->GetWaveAt_Linear(nCurPhase1+nPWM1);
						float osc2 =	pLevel2A->GetWaveAt_Linear(nCurPhase2)-pLevel2B->GetWaveAt_Linear(nCurPhase2+nPWM2);
						float output = osc1+(osc2-osc1)*fOscMix;
						pout[i]+=(float)(amp*chn->Filter.ProcessSample(amp*output));

						nCurPhase1+=nCurFrequency1;
						nCurPhase2+=nCurFrequency2;
					}
				}
			}
			s0=c0;
		}
	}

  chn->PhaseOSC1+=Frequency1*c;
  chn->PhaseOSC1=float(fmod(chn->PhaseOSC1,(float)1.0));
  chn->PhaseOSC2+=Frequency2*c;
  chn->PhaseOSC2=float(fmod(chn->PhaseOSC2,(float)1.0));
  chn->PhaseOSC3+=Frequency3*c;
  chn->PhaseOSC3=float(fmod(chn->PhaseOSC3,(float)1.0));
	if (!chn->pTrack)
    return true;
  if (chn==trk->Chn())
  {
    trk->Vib1Phase=(float)fmod(trk->Vib1Phase+c*trk->Vib1Rate,(float)(2*PI*pmi->_master_info->samples_per_tick));
    trk->Vib2Phase=(float)fmod(trk->Vib2Phase+c*trk->Vib2Rate,(float)(2*PI*pmi->_master_info->samples_per_tick));
    if (trk->SlideEnd!=0.0f && chn->Frequency==trk->DestFrequency)
	  {
		  int slend=int(trk->SlideEnd*pmi->_master_info->samples_per_tick);
		  trk->SlideCounter+=c;
		  chn->Frequency=trk->DestFrequency=float(trk->BaseFrequency*pow(2.0,trk->SlideRange/12.0*std::min(trk->SlideCounter,(float)slend)/slend));
		  if (trk->SlideCounter>=slend)
		  {
			  trk->SlideCounter=trk->SlideEnd=0.0f;
			  trk->BaseFrequency=chn->Frequency;
		  }
	  }
	  else
      chn->Frequency=Glide(chn->Frequency,trk->DestFrequency,pmi->gvalAct.vGlide,c);
  }
  
  return true;
}

bool fsm_infector::process_stereo(float **pin, float **pout, int numsamples, int mode)
{
  Osc1PWM.Set(300*gvalAct.vPWMRateA,std::max(1<<31,int(65535*120/(gvalAct.vPWMRateA+120)*gvalAct.vPWMRangeA/240)));
  Osc2PWM.Set(300*gvalAct.vPWMRateB,std::max(1<<31,int(65535*120/(gvalAct.vPWMRateB+120)*gvalAct.vPWMRangeB/240)));
	for (int i=0; i<numsamples; i++)
	{
		PWMBuffer1[i]=Osc1PWM.GetSample();
		PWMBuffer2[i]=Osc2PWM.GetSample();
	}
  inrCutoff.Process(gvalAct.vFilterCutoff,numsamples);
  inrResonance.Process(gvalAct.vFilterResonance,numsamples);
  inrModulation.Process(gvalAct.vFilterModulation,numsamples);
  inrModShape.Process(gvalAct.vFilterShape,numsamples);
  inrLFO1Dest1.Process(gvalAct.vLFOAmount1,numsamples);  // LFO1 -> Cutoff
  inrLFO1Dest2.Process(gvalAct.vLFOAmount2,numsamples);  // LFO1 -> EnvMod
  inrLFO2Dest1.Process(gvalAct.vLFO2Amount1,numsamples); // LFO2 -> Cutoff
  inrLFO2Dest2.Process(gvalAct.vLFO2Amount2,numsamples); // LFO2 -> Resonance

	float *psamples = pout[0];
	
  bool donesth=false;
  for (int i=0; i<numsamples; i++)
    psamples[i]=0.0;

  int so=0;
	while(so<numsamples)
	{
		int end=numsamples;
		for (int i=0; i<numTracks; i++)
		{
			int wt=Tracks[i].GetWakeupTime(end-so);
			if (wt<end-so)
				end=so+wt;
		}
		for (int i=0; i<numTracks; i++)
		{
			Tracks[i].UseWakeupTime(end-so);
	    Tracks[i].DoLFO(this,numsamples);
		}
		for (int i=0; i<MAX_CHANNELS; i++)
			donesth |= DoWorkChannel(psamples+so, this, end-so, &Channels[i]);
		for (int i=0; i<numTracks; i++)
			Tracks[i].DoWakeup(this);
		so=end;
	}
	for (int i=0; i<numsamples; i++)
	{
		if (!(psamples[i]<4000000 && psamples[i]>-4000000))
		{
			psamples[i]=0.0;
		}
		else {
			psamples[i] *= 1.0f/32768.0f;
		}
	}
	
	memcpy(pout[1], pout[0], sizeof(float) * numsamples);
	
	return donesth;
}

void fsm_infector::midi_note(int channel, int value, int velocity)
{
  if (channel!=aval.channel-1)
    return;

	zzub_sequence_t *pseq;

	int stateflags = _host->get_state_flags();	
	if (stateflags & zzub::state_flag_playing && stateflags & zzub::state_flag_recording)
		pseq = _host->get_playing_sequence(_host->get_metaplugin());
	else 
		pseq = NULL;

  int note2=((value/12)<<4)+(value%12)+1;
  if (velocity)
  {
    int nPlaybackTrack=-1;
    for (int i=0; i<numTracks; i++)
      if (Tracks[i].MidiNote==note2 || (Tracks[i].MidiNote!=-1 && (gvalAct.vLFOMode&32)))
      {
        nPlaybackTrack=i;
        break;
      }  
    if (nPlaybackTrack==-1)
    {
      for (int i=0; i<numTracks; i++)
			{
				CChannel *chn=Tracks[i].Chn();
				if (!chn || chn->AmpEnv.m_nState==4)
        {
          nPlaybackTrack=i;
          break;
        }  
			}
    }
    if (nPlaybackTrack==-1)
    {
      float fAmplitude=9e9f;
      for (int i=0; i<numTracks; i++)
			{
				CChannel *chn=Tracks[i].Chn();
        if (!chn || chn->AmpEnv.m_fLast<fAmplitude)
        {
          nPlaybackTrack=i;
          fAmplitude=(float)chn->AmpEnv.m_fLast;
        }  
			}
    }
    if (nPlaybackTrack!=-1)
    {
      Tracks[nPlaybackTrack].PlayNote(/*gvalAct,*/note2,aval.usevelocity?velocity*240/127:240,240,_master_info);
      if (pseq)
      {
        unsigned char *pdata = (unsigned char *)_host->get_playing_row(pseq, 2, nPlaybackTrack);
			  pdata[0] = note2;
        if (aval.usevelocity)
  			  pdata[1] = velocity;
        else
          pdata[1] = 224;
        pdata[2] = 240;
      }
    }
  }
  else
  {
    for (int i=0; i<numTracks; i++)
		{
			CChannel *ch=Tracks[i].Chn();
      if (ch && Tracks[i].MidiNote==note2)
      {
        ch->AmpEnv.NoteOff();
        ch->FilterEnv.NoteOff();
        if (pseq)
        {
          unsigned char *pdata = (unsigned char *)_host->get_playing_row(pseq, 2, i);
          pdata[0] = zzub::note_value_off;
        }
      }
		}
  }
}

void fsm_infector::save(zzub::archive *arc)
{
	zzub::outstream *po = arc->get_outstream("");
  po->write(int(1));
  po->write(userwaves,sizeof(userwaves));
  po->write(usersources,sizeof(usersources));
}

void fsm_infector::Reset()
{
  for (int i=0; i<numTracks; i++)
    Tracks[i].Reset();
  for (int i=0; i<MAX_CHANNELS; i++)
    Channels[i].Reset();
}

void fsm_infector::ClearFX()
{
  for (int i=0; i<numTracks; i++)
    Tracks[i].ClearFX();
  for (int i=0; i<MAX_CHANNELS; i++)
    Channels[i].ClearFX();
}

#pragma optimize ("", on)

} // namespace fsm

struct fsminfectorplugincollection : zzub::plugincollection {
	virtual void initialize(zzub::pluginfactory *factory) {
		fsm::GenerateWaves();
		factory->register_info(&fsm::fsm_infector::fsm_infector_info);
	}
	
	virtual const zzub::info *get_info(const char *uri, zzub::archive *data) { return 0; }
	virtual void destroy() { delete this; }
	// Returns the uri of the collection to be identified,
	// return zero for no uri. Collections without uri can not be 
	// configured.
	virtual const char *get_uri() { return 0; }
	
	// Called by the host to set specific configuration options,
	// usually related to paths.
	virtual void configure(const char *key, const char *value) {}
	
};

zzub::plugincollection *zzub_get_plugincollection() {
	return new fsminfectorplugincollection();
}

const char *zzub_get_signature() { return ZZUB_SIGNATURE; }
