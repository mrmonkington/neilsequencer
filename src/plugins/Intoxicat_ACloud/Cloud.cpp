#include <zzub/signature.h>
#include <zzub/plugin.h>
#include <cstdio>
#include <stdlib.h>
#include <math.h>
#include <string.h>


//#include "gAHDEnv.h"
#include "CGrain.h"
#include "themi.h"

//#pragma optimize ("awy", on)


//class mi;

class CGrain;

inline int b2n(int b)
{
	int n = ((int(b / 16) * 12) + (b % 16));
	return n -= 49;
}

float HackValue0520a(int maxv, int minv, int value)
{
//      float rtn = 1.0f;
        int rng = maxv-minv;
        if (value < rng/2) return (((0.5f/(rng/2.0f))* value)+0.5f);
        else return ((float)value / ((float)rng/2.0f));
} 

float HackValue0520(int maxv, int minv, int value)
{
//      float rtn = 1.0f;
        int rng = maxv-minv;
        if (value < rng/2) return (((0.75f/(rng/2.0f))* value)+0.25f);
        else return (((float)value + ((value-127)*2)) / ((float)rng/2.0f));
}


struct acloud::WAVESEL
{
	int wnum;
	int waveslot;
};

acloud::acloud() {
  global_values = &gval;
  printf("RAND_MAX = %d\n", RAND_MAX);
}

acloud::~acloud() {
}

void acloud::process_events() {

  int o = _master_info->samples_per_tick;

  if (gval.note != zzub::note_value_none) //this is going to change to note
  {
	  if(gval.note != 255)
	  {
		 //printf ("\nNOTE!: : %d",gval.note);
		if (!cloud_is_playing)///added march 2006
		{
			for(int j=0; j < maxgrains; j++) Grain[j].IsActive = 0;
			gnext = SetNextGrain(SetDens(densMode));
			gcount = 0;
		}
		cloud_is_playing = true;
		int n = b2n(gval.note);
		nrate = powf(2.0f, ((n) / 12.0f));
		//gnext = 0; //check if this works
		offsCount = 0;
	  }
	  else{
		  cloud_is_playing = false;
		  //gcount = 0;
	  }
  }
  if (gval.seed != 0xFFFF) srand(gval.seed);
  if (gval.wnumber1 != zzub::wavetable_index_value_none) wavenum1 = gval.wnumber1;
  if (gval.offset1 != 0xFFFF) offset1 = (float)gval.offset1/0xFFFE;
  if (gval.offset2 != 0xFFFF) offset2 = (float)gval.offset2/0xFFFE;
  if (gval.wnumber2 != zzub::wavetable_index_value_none) wavenum2 = gval.wnumber2;
  if (gval.offstype != 0xFF) offstype = gval.offstype;
  if (gval.offsmode != 0xFF){
	  offsMode = gval.offsmode;
	  offsCount = 0;
  }
  if (gval.skiprate != 0xFF) skiprate = HackValue0520(254,0,gval.skiprate);
  if (gval.w2offset1 != 0xFFFF) w2offset1 = (float)gval.w2offset1/0xFFFE;
  if (gval.w2offset2 != 0xFFFF) w2offset2 = (float)gval.w2offset2/0xFFFE;
  if (gval.wavemix != 0xFFFF) wavemix = gval.wavemix;
  if (gval.gduration != 0xFFFF) gdur = gval.gduration * (_master_info->samples_per_second/44100);
  if (gval.gdrange != 0xFFFF) gdrange = gval.gdrange;
 
  if (gval.amp != 0xFFFF){
	iamp = (int)gval.amp;
	if (iamp > 0x8000){
		ampt = 1.0f;
		ampb = (float)(iamp - 0x8000)/0x8000;
	}
	else{
		ampb = 0.000001f;
		ampt = (float)iamp / 0x8000;
	}
  }

  if (gval.rate != 0xFF) rate = HackValue0520(254, 0, gval.rate);
  if (gval.rrate != 0xFF) rrate = gval.rrate;

  if (gval.envamt != 0xFF) envamt = (float)gval.envamt / 512.0f;
  if (gval.envq != 0xFF) envq = (float)gval.envq/128.0f;
 
  if (gval.lpan != 0xFF) lpan = (float)gval.lpan;
  if (gval.rpan != 0xFF) rpan = (float)gval.rpan;
  
  //Cloud Params
  if (gval.densmode != 0xFF) densMode = gval.densmode;
  if (gval.density != 0xFFFF) density = gval.density; densfactor = (float)density/1000.0f;
  if (gval.maxgrains != 0xFF) maxgrains = gval.maxgrains;

	//printf ("\nt1: : %d",test1);
	//printf ("\nt2: : %d",test2);


  //FIXME omitting the GrainLoad dialog box for now.

// 	if ( (GetDlgItem(hDlgGLoad, IDC_PROGRESS1)) != NULL )
// 	{
// 		SendDlgItemMessage(         hDlgGLoad,
// 									IDC_PROGRESS1,
// 									PBM_SETPOS,
// 									(WPARAM)( (CountGrains() / (float) maxgrains) * 100 ) ,
// 									0);
// 	};

	//printf ("\nlenc: : %d",gcount);

}

void acloud::init(zzub::archive *arc) {

	// ThisMachine = _host->get_metaplugin();
	// _host->set_track_count(ThisMachine,2);

	//mi vars
	wavenum1 = wavenum2 = 1;
	wavemix = 0x4000; 
	maxgrains = 20;
	gdur = 1000;
	gdrange = 0;
	rate =  nrate = 1.0f;
	rrate = 0;
	cloud_is_playing = false;
	gcount = 0;
	gnext = SetNextGrain(20);
	offset1 = offset2 = 0.0f;
	offstype = 0;
	w2offset1 = w2offset2 = 0.0f;
	envamt = envq = 0.0f;
	
	waveslot = 1;
	offsCount = 0;
	offsInc = 0;
	offsMode = 0;
	skiprate = 1.0f;

	density = 20; //obviously this should be settable - maybe attribute would do...

	//init grain class
	for(int i=0; i<127;i++)
	{
		Grain[i].SetMiPointers(&_master_info->samples_per_second);
		Grain[i].Init();
	}

	iamp = 0xFFFF;
	ampt = ampb = 0.0f;

	envamt = envq = 0.0f;
	lpan = rpan = 0.0f;
	skiprate = 1.0f;

	//AllocConsole(); freopen("CON", "w", stdout);
	//printf ("\nlenc: : %d",NOTE_MIN);
	//printf ("\nlenc: : %d",NOTE_MAX);

	process_events();
	cloud_is_playing = false;

 }


bool acloud::process_stereo(float **pin, float **pout, int numsamples, int mode) {

  float psamples[256 * 2];

	int g = -1;
	int w = 1;
	//bool madenoise = false;
//	WAVESEL *ws;//struct test

	int last_gnext = 0;//next onset position should be
	bool IsFirstInWork = true;
		

	//if (!cloud_is_playing) return false;

	last_gnext = gnext - (int)gcount; //get the next grain onset pos withing this work call - taken before gcount is incremented - will be negative most of the time but shouldnt evaluate true below so shouldn't matter
	gcount += numsamples;//increment the grain counter by the whole numsamples
	
	//printf ("\nWORK!: : %d",gcount);

	if (gcount > gnext && cloud_is_playing)//check if grain should be generated thsi work
	{
		do
		{
			g = FindGrain(maxgrains);
			 
			if (g >= 0)
			{
				//gnext - (gcount - numsamples);//this will only work for the fist time it is called in a work
				Grain[g].IsActive = 0;
				
				w = SelectWave(wavemix);
				if (_host->get_wave_level(w,0))
				{	
					Grain[g].CDown = last_gnext;//this is the onset position for this grain in the work
					Grain[g].Set(SetGDRange(),SetOffset(waveslot, w),1,nrate*rate*GetRandRate(),GetRandPan());
					Grain[g].SetWave(w, (_host->get_wave(w)->flags & zzub::wave_flag_stereo), _host->get_wave_level(w,0));
					Grain[g].SetEnv(Grain[g].Duration, envamt, envq);
					Grain[g].SetAmp(ampt, ampb, _host->get_wave(w)->volume);
					Grain[g].IsActive = 1;
				}
			}
			
			gnext = SetNextGrain(SetDens(densMode));//set the next onset pos
			last_gnext += gnext;//add next onset pos to last onset postion
			//offsCount += gnext * skiprate;
			offsInc += gnext;//change to increment as of 191205 as SetOffset would only be called if a new grain is triggered which might not happen at high densities
		//for the forward offset counter - gets reset to 0 every time SetOffset() is called.
			
		}while (last_gnext < numsamples);//if total of onset positions is still less that numsamples loop and generate another grain.
		
		
		gcount = numsamples - (last_gnext - gnext);//the remainder of numsamples - the last onset pos of a grain... - gnext  so that it wont go out of range.
	}
	
	if (!CheckActivGrains(maxgrains)) 
	{
		memset(pout[0], 0, (numsamples) * sizeof(float)); //clear the last buffer
		memset(pout[1], 0, (numsamples) * sizeof(float)); //clear the last buffer
		return false;
	}
	
	memset(pout[0], 0, (numsamples) * sizeof(float));//????
	memset(pout[1], 0, (numsamples) * sizeof(float));//????
	
	for(int j=0;j<maxgrains;j++) //can the isfirstinwork check be made more efficient?
	{
		//Grain[j].GenerateAdd(psamples, numsamples);
		if(Grain[j].IsActive == 1 && IsFirstInWork) Grain[j].Generate(psamples, numsamples, _host->get_wave_level(Grain[j].Wave,0));
		if(Grain[j].IsActive == 1 && !IsFirstInWork) Grain[j].GenerateAdd(psamples, numsamples, _host->get_wave_level(Grain[j].Wave,0));
		
		IsFirstInWork = false;
	}
	
	for (int i = 0; i < numsamples; i++) {
	  pout[0][i] = psamples[2 * i] * downscale;
	  pout[1][i] = psamples[2 * i + 1] * downscale;
	}


	//if (madenoise) return true;
	//return false;
	return true;

}

void acloud::command(int i) {

	int j;
	switch (i)
	{
	case 0:
	  // FIXME should be a message box -- jmmcd
	  printf("About ACloud: \nIntoxicat ACloud\n\nAsynchronous Sample Granulator\n");
	  //MessageBox(NULL,"Intoxicat ACloud\n\nAsynchronous Sample Granulator","About ACloud",MB_OK|MB_SYSTEMMODAL);
		break;
	case 1:
	  printf("Reset!\n");
		cloud_is_playing = false;
		for(j=0; j < maxgrains; j++){
			Grain[j].IsActive = 0;
			Grain[j].Init();
		}
		break;
	case 2:
	  //g_mi=this;
	// FIXME re-add the GrainLoad dialog (not essential!) -- jmmcd
//         DialogBox(dllInstance, MAKEINTRESOURCE (IDD_GLOAD), GetForegroundWindow(), (DLGPROC) &GLoadDialog);
	default:
		break;
	}

}

const char * acloud::describe_value(int param, int value) {
	static char txt[16];
	float t, p;

	switch(param)
	{
	case 0:				
		return NULL;
		break;

	case 2:
		sprintf(txt, "%d %s",value, _host->get_wave_name(value));
		return txt;
		break;
	case 3:		
		sprintf(txt, "%X %.1f%%", value, ((float)value/65534.0f)*100);
		return txt;
		break;
	case 4:		
		sprintf(txt, "%X %.1f%%", value, ((float)value/65534.0f)*100);
		return txt;
		break;
	case 5:
		sprintf(txt, "%d %s",value, _host->get_wave_name(value));
		return txt;
		break;
	case 6:		
		sprintf(txt, "%X %.1f%%", value, ((float)value/65534.0f)*100);
		return txt;
		break;
	case 7:		
		sprintf(txt, "%X %.1f%%", value, ((float)value/65534.0f)*100);
		return txt;
		break;
	case 8:
		if (value == 0) return "Off [!Slaved]";
		if (value == 1) return "On [Slaved]";
		else return "N00b";
		break;
	case 9:
		if (value == 0) return "Random";
		if (value == 1) return "Forwards";
		else return "Backwards";
		break;
	case 10:
		sprintf(txt, "%.2f", HackValue0520(254, 0, value));
		return txt;
		break;
	case 11:		
		sprintf(txt, "%.1f%% / %.1f%%", 100-((value/32767.0f)*100), (value/32767.0f)*100);
		return txt;
		break;
	case 12:
		return "----------0";
		break;
	case 13:		
		sprintf(txt, "%.1fms", (value/(float)_master_info->samples_per_second)*1000);
		return txt;
		break;
	case 14:		
		sprintf(txt, "%.1fms", (value/(float)_master_info->samples_per_second)*1000);
		return txt;
		break;
	case 15:	
		t = ((float)value/32767.0f) - 1.0f;
		if (t<0.0f) t = 0.0f;
		p = ((float)(value/32767.0f));
		if (p > 1.0f) p = 1.0f;
		sprintf(txt, "B%.2f / T%.2f", t, p);
		return txt;
		break;
	case 16:		
		sprintf(txt, "%.2f", HackValue0520(254, 0, value));
		return txt;
		break;
	case 17:		
		sprintf(txt, "%.1f semi", (float)value/10.0f);
		return txt;
		break;
	case 18:		
		sprintf(txt, "%.1f%%", ((float)value/254.0f)*100);
		return txt;
		break;
	case 19:		
		sprintf(txt, "%.2f", ((float)value/127.0f)-1.0f);
		return txt;
		break;
	case 20:
		if (value <= 64) sprintf(txt, "L %.2f", ((float)value/64.0f)-1.0f);
		else sprintf(txt, "R %.2f", ((float)value/64.0f)-1.0f);
		return txt;
		break;
	case 21:
		if (value < 64) sprintf(txt, "L %.2f", ((float)value/64.0f)-1.0f);
		else sprintf(txt, "R %.2f", ((float)value/64.0f)-1.0f);
		return txt;
		break;
	case 22:
		return "----------0";
		break;
	case 23:
		if (value == 0) return "Avr.Grs pSec";
		if (value == 1) return "Perceived";
		else return "N00b";
		break;
	case 24:
		sprintf(txt, "%d/%.2f%%", value, (float)value/10.0f);
		return txt;
		break;
	default: return NULL;
		break;
	};

}


void acloud::set_track_count(int n) {
  //numTracks = n;
}
void acloud::stop() {
	cloud_is_playing = false;
	//for(int j=0; j < maxgrains; j++){
	//	Grain[j].IsActive = 0;
	//	Grain[j].Init();
	//}
  
}

void acloud::destroy() { 
  delete this; 
}

void acloud::attributes_changed() {
  // do nothing
}


const char *zzub_get_signature() { return ZZUB_SIGNATURE; }



inline int f2i(double d)
{
  const double magic = 6755399441055744.0; // 2^51 + 2^52
  double tmp = (d-0.5) + magic;
  return *(int*) &tmp;
}




//___________________________________________________________________
// 
// My functions start here
//___________________________________________________________________
//
inline int acloud::FindGrain(int maxg)
{
	for(int i=0; i < maxg; i++) if (Grain[i].IsActive == 0) return i;
	return -1;
}
//just do n+rnd()*range, where rnd goes from -1..1 and use the inverse of the result as the interval before triggering the next grain

inline int acloud::SetNextGrain(int dens)
{
        float r = ((float)rand() * 2.0f / RAND_MAX);//range 0/2.0
	int t = (int)((_master_info->samples_per_second / dens) * r);
	return t + 1;
}

inline int acloud::CountGrains()
{
	int c = 0;
	for (int i = 0; i < maxgrains; i++)
		if (Grain[i].IsActive) c++;

	return c;
}

inline int acloud::SetDens(int mode)
{
	if (mode == 0) return density;

	return (int)((220500.0f/gdur) * densfactor) + 1;
}
inline bool acloud::CheckActivGrains(int maxg)
{
	for(int i=0; i < maxg; i++) if (Grain[i].IsActive == 1) return true;
	return false;
}

double acloud::SetOffset(int wave, int wnum) //start end
{
	float s = 0.0f;
	float e = 0.0f;
	double rtn;
	const zzub::wave_level *pwl = _host->get_wave_level(wnum,0);
	int wnums = pwl->sample_count;
	
	s = offset1;
	e = offset2;

	if (wave == 2 && offstype == 0)
	{
		s = w2offset1;
		e = w2offset2;
	}


	offsCount += (offsInc * skiprate * ((float)pwl->samples_per_second / (float)_master_info->samples_per_second));
	offsInc = 0; //191205 to reset now incremental counter.

	if(offsMode == 0){//normal random
		if (e == 0.0f) return s * wnums;//check if no range
		if (e > (1.0f-s)) e = (1.0f-s);//check if range greater than available range
		float r = (float)rand()/RAND_MAX;//random factor
		return ((r*e)+s)*wnums;
	}
	
	if(offsMode == 1){

		if (offsCount+(wnums*s) > wnums) offsCount = 0;
		
		if (e == 0.0f) return (wnums*s) + offsCount;
		if (e > (1.0f-s)) e = (1.0f-s);//check if range greater than available range

		float r = (float)rand()/RAND_MAX;
		rtn = (((e*r)+s)*wnums) + offsCount;
		//assert (rtn <= wnums);
			if (rtn > wnums) return (wnums*s) + ((int)rtn % wnums);
		
		return rtn;
	}

	if(offsMode == 2){
		if ((wnums*s) - offsCount < 0) offsCount = 0;
		
		if (e == 0.0f) return (wnums*s) - offsCount;
		if (e > s) e = s;//check if range is greated than is available

		float r = (float)rand()/RAND_MAX;
		rtn = ((s-(e*r))*wnums) - offsCount;
		//assert (rtn <= wnums);
			if (rtn < 0) return (wnums*s);// - (rtn-(wnums*s));
		return rtn;
	}

	if (e == 0.0f) return s * wnums;//check if no range
	if (e > (1.0f-s)) e = (1.0f-s);//check if range greater than available range
	float r = (float)rand()/RAND_MAX;//random factor
	return ((r*e)+s)*wnums;
}

inline int acloud::SelectWave(int mix)
{

  // The win version, I think, assumed that RAND_MAX was always
  // 0xffff, and used the value of the paraWaveMix parameter directly
  // as a probability (because 0xffff is its maximum value). Here,
  // instead, we calculate: (mix / paraWaveMix->value_max) and and
  // (rand() / RAND_MAX). Each of these two quantities is in [0, 1] so
  // they can be directly compared. Similar story in SelectWave2,
  // below. -- jmmcd

  float r = rand() / (float) RAND_MAX;
  float m = mix / (float) paraWaveMix->value_none;
  if (r > m) {
    waveslot = 1;
    return wavenum1;
  }
  waveslot = 2;
  return wavenum2;
}

void acloud::SelectWave2(int mix, WAVESEL * Wv)
{
	
	Wv->wnum = wavenum2;
	Wv->waveslot = 2;
		
	if (rand() / (float) RAND_MAX < mix / (float) paraWaveMix->value_max)
	{
		Wv->wnum = wavenum1;
		Wv->waveslot = 1;
	}
	//return x;
}

inline int acloud::SetGDRange()
{
	if (gdrange <= gdur) return gdur;
	float r = (float)rand()/RAND_MAX;
	return gdur+(int)((gdrange-gdur) * r);
}

inline float acloud::GetRandRate()//this should be rewritten
{
	//float rtn = 0.0f;
	if(rrate == 0) return 1.0f;

	// I think 0x4000 was intended as 
	float r = ((float)rand() * 2.0f/RAND_MAX) - 1.0f;
	float q = (r*rrate)/120.0f;
	//float n = (q/128.0f);
	return powf(2.0f, q);
	//return x;
}

float acloud::GetRandPan()//thisis a bit crap i think... 
{
	float pwidth;
	float r = (float)rand()/RAND_MAX;
	//pwidth *= r;
	if (rpan > lpan) {
		pwidth = (rpan - lpan) / 128.0f;
		return (pwidth*r) + (float)lpan/128.0f;
		}
	//else
	pwidth = (lpan - rpan) / 128.0f;
	return (pwidth*r) + (float)rpan/128.0f;
		
}





//"Intoxicat ACloud\n\nAsynchruos Sample Granulator" && 

// HINSTANCE dllInstance;
// mi *g_mi;

// BOOL WINAPI DllMain ( HANDLE hModule, DWORD fwdreason, LPVOID lpReserved )
// {
//    switch (fwdreason) {
//    case DLL_PROCESS_ATTACH:
//       dllInstance = (HINSTANCE) hModule;
// 	  overloaderdata_initialize();
//       break;

//    case DLL_THREAD_ATTACH: break;
//    case DLL_THREAD_DETACH: break;
//    case DLL_PROCESS_DETACH: break;
//    }
//    return TRUE;
// }

// BOOL APIENTRY GLoadDialog(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
// 	//int aBar = GetDlgItem(hDlg, IDC_PROGRESS1);
//    switch(uMsg) {
//    case WM_INITDIALOG:
// 	   g_mi->SetDialogHandle(hDlg);
//       return 1;

//    case WM_SHOWWINDOW: 
//       return 1;

//    case WM_CLOSE:
//       EndDialog (hDlg, TRUE);
//       return 0;

//    case WM_COMMAND:
//       switch (LOWORD (wParam))
//       {
//       case IDOK:
//          EndDialog(hDlg, TRUE);
//          return 1;
//       case IDCANCEL:
//          //update the progressbar  g_mi->maxgrains
// 		  //SendDlgItemMessage(hDlg, IDC_PROGRESS1, PBM_SETRANGE ,0,100 );
// 		  SendDlgItemMessage(hDlg, IDC_PROGRESS1, PBM_SETPOS ,50,0 );
// 		  //SetDlgItemText(hDlg, IDC_EDIT1, "hey");
// 		//	GLoadDialog.
// 			return 0;
//       default:
//          return 0;
//       }
//       break;
//    }
//    return 0;
// }

/*
BOOL WINAPI DllMain ( HANDLE hModule, DWORD fwdreason, LPVOID lpReserved )
{
	switch (fwdreason) {
	case DLL_PROCESS_ATTACH:
	{
		overloaderdata_initialize();
	//	overloaderdata_registerimage("cloudgrain",(HINSTANCE)hModule,"Cl1");
	//	overloaderdata_registerimage("pramtest_test2",(HINSTANCE)hModule,"test2");
	}
		break;
	case DLL_THREAD_ATTACH: break;
	case DLL_THREAD_DETACH: break;
	case DLL_PROCESS_DETACH: break;
	}
	return TRUE;
}
*/
 //DLL_EXPORTS
