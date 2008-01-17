bool mi::MDKWorkStereo(float *psamples, int numsamples, int const mode)
{
	//float outL, outR;
	//int g, w;

		
	   if (!cloud_is_playing) 
	   {
		   memset(psamples, 0, (numsamples*2) * sizeof(float)*2); //clear the last buffer
		   //sample_is_playing = false;
		   return false;
	   }


	for (int i=0; i<numsamples*2 && cloud_is_playing; i+=2)
	{
		float outL = 0.0f, outR = 0.0f;
		int g = 0;
		//gots = false;
		
		gcount++;
		if (gcount > gnext)
		{
			g = FindGrain(maxgrains);
			//printf ("\nfgrain: : %d",g);
			if (g >= 0){
				
				//Grain[g].Set(gdur,SetOffset(offset1, offset2),1,rate,0.0f);
				
				int w = SelectWave(wavemix);

				Grain[g].IsActive = 0;
				
				if (pCB->GetWaveLevel(w,0))
				{	
				Grain[g].Set(gdur,SetOffset(offset1, offset2),1,rate,0.0f);
				Grain[g].SetWave(w, (pCB->GetWave(w)->Flags & WF_STEREO), pCB->GetWaveLevel(w,0));
				Grain[g].IsActive = 1;
				Grain[g].SetEnv(envamt, envq);//
				}

			}
			
			gnext = SetNextGrain(density);
			gcount = 0;

		}

		if (CheckActivGrains(maxgrains)) //check if anything is meant to make noise here
		{
			
			for(int j=0;j<maxgrains;j++)
			{
				if(Grain[j].IsActive == 1)
				{
					
					Grain[j].GetNext();
					outL += Grain[j].outL;
					outR += Grain[j].outR;
				}
			}

			psamples[i]		=	outL;
			psamples[i+1]	=	outR;

		}
		
		else psamples[i] = psamples[i+1] = 0.0f; //zero out the rest of the buffer.

	}

	return true; 
}