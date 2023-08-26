#include "diffcalc.h"
#include <math.h>

float run_diffcalc(chart_data* cd, float rate)
{
	int nLines = atoi(get_datavalue(cd->infodata,"LINES"));
	int total_notes = 0, count = 0;
	struct note_data* notes[nLines];
	
	for(int i = 0; i < nLines; i++)
	{
		total_notes += cd->nNotes[i];
		notes[i] = cd->notes[i];
	}
	
	int* times = (int*)malloc(sizeof(int)*total_notes);
	int* indices = (int*)malloc(sizeof(int)*total_notes);
	int* tech = (int*)malloc(sizeof(int)*total_notes);
	
	int last_index = -1;
	
	while(count < total_notes)
	{
		int32_t nextnote = 999999999, index = -1;
		for(int i = 0; i < nLines; i++)
		{
			if(notes[i] != NULL)
			{
				if(notes[i]->time < nextnote)
				{
					nextnote = notes[i]->time;
					index = i;
				}
				else if(notes[i]->time == nextnote)
				{
					if(last_index == i)
						index = i;
				}
			}
		}
		
		if(index == -1)
			break;
		if(last_index == -1)
			last_index = index;
		if(count != 0)
		{
			if(times[count-1] != nextnote)
			{
				tech[count] = index+1;
				times[count++] = nextnote;
				if(index == last_index)
				{
					indices[count-1] = 0;
				}
				else
				{
					indices[count-1] = 1;
					last_index = -1;
				}
			}
			else
			{
				tech[count-1] = 3;
				last_index = -1;
			}
		}
		else
		{
			times[count] = nextnote;
			tech[count] = index+1;
			indices[count++] = 0;
		}
		
		notes[index] = notes[index]->tail;
	}
	
	float* avgs = (float*)malloc(sizeof(float)*total_notes);
	memset(avgs,0,sizeof(sizeof(float)*total_notes));
	
	for(int i = 0; i < count; i++)
	{
		times[i] = (int)round((float)times[i]/rate);
	}
	
	float strain = 1.f, stamina_ = 0.f, speed_ = 0.f, recovery_time, strain_increase = 1.f, bpm_arb = 100.f;
	int curr = 0, kps_ = 0, max_kps = 0, last_kps = 0, kps_size = 1001;
	
	float* kps_arr = (float*)malloc(sizeof(float)*kps_size);
	
	float kps_avg = 0.0, minkps = 999999999;
	
	for(int i = 0; i < kps_size; i++)
		kps_arr[i] = 1.f;
	
	if(rate > 1.0)
		recovery_time = 60000.f / bpm_arb / 4.f;
	else
		recovery_time = 60000.f / bpm_arb / 4.f;
	
	float limit = 1.0;
	float newval = 0.f;
	
	int patterns[64],p,pp;
	memset(patterns,0,sizeof(patterns));
	
	for(int k = 0; k < 2; k++)
	{
		for(int i = k; i < count-4; i+=2)
		{
			if(tech[i] == tech[i+2] && tech[i+2] == tech[i+4])
				continue;
			if(abs((times[i+2]-times[i])-(times[i+4]-times[i+2])) > 60000/bpm_arb)
				continue;
			
			p = (tech[i])|(tech[i+2]<<2)|(tech[i+4]<<4);
			pp = (tech[i+4])|(tech[i+2]<<2)|(tech[i]<<4);
			if(patterns[p] == 0 && patterns[pp] == 0)
				patterns[p]++;
			else if(patterns[p] > 0)
				patterns[p]++;
			else if(patterns[pp] > 0)
				patterns[pp]++;
			
			i+=2;
		}
	}

	for(int i = times[0]; i <= times[count-1]; i++)
	{
		strain -= strain_increase / recovery_time;
		if(strain < 1)
			strain = 1;
		
		if(i >= times[curr])
		{
			if(curr != 0)
			{
				float x = 1.0/(((float)times[curr]-times[curr-1])/recovery_time);
				float y = ((float)times[curr]-times[curr-1]);
				
				if(indices[curr])
				{
					if(curr > 1)
					{
						x = 1.0/(((float)times[curr]-times[curr-2])/recovery_time);
						y = ((float)times[curr]-times[curr-2]);
					}
					if(y > 1000)
						y = 1000;
					
					/*kps_avg += y;
					avgs[curr] = y;
					
					if(kps_avg != 0)
						speed_ += pow(1000.0/(kps_avg/(kps_+1)),2)*(x<1?1:pow(x,2))/pow(kps_arr[kps_],2.0);
					else
						speed_ += pow(kps_,2)*(x<1?1:pow(x,2))/pow(kps_arr[kps_],2.0);*/
					
					/*speed_ += pow(kps_,2.0)*(x<1?1:pow(x,2))/pow(kps_arr[kps_],2.0);
					kps_arr[kps_] += 1/exp(2);*/
					if(kps_avg != 0)
						speed_ += pow(1000.0*rate/(kps_avg/(kps_+1)),2)/pow(kps_arr[kps_],2);
					kps_arr[kps_] += 1/exp(2);
					limit += 1/exp(1);
					
					/*stamina_ += sqrt(log10(strain))*(x < 1 ? 1 : pow(x,2));
					strain += strain_increase;*/
					kps_--;
				}
				else
				{
					if(indices[curr-1])
					{
						if(curr > 1)
						{
							x = 1.0/(((float)times[curr]-times[curr-2])/recovery_time);
							y = ((float)times[curr]-times[curr-2]);
						}
					}
					if(y > 1000)
						y = 1000;
				
					kps_avg += y;
					avgs[curr] = y;

					speed_ += pow(1000.0*rate/(kps_avg/(kps_+1)),2)/pow(kps_arr[kps_],2);
					//speed_ += pow(1000.0/(kps_avg/(kps_+1)),2)*(x<1?1:pow(x,2))/pow(kps_arr[kps_],2);
					kps_arr[kps_] += 1/exp(exp(1));
					
					//speed_ += pow(kps_,2.0)*(x<1?1:pow(x,2))/pow(kps_arr[kps_],2.0);
					//kps_arr[kps_] += 1/exp(2);
					
					//limit += 1/exp(2);
					
					stamina_ += sqrt(log10(strain)*1000.0*rate/(kps_avg/(kps_+1)))*(x < 1 ? 1 : pow(x,2));
					strain += strain_increase*log10(pow(1000.0*rate/(kps_avg/(kps_+1)),2));
					
					if((kps_avg/(kps_+1)) < minkps)
					{
						minkps = (kps_avg/(kps_+1));
						//limit = sqrt(limit);
					}
				}
			}
			else
			{
				kps_avg += 1000;
				avgs[curr] = 1000;
			}
			
			kps_++;
			curr++;
			
			if(kps_ >= kps_size)
			{
				kps_arr = (float*)realloc(kps_arr,sizeof(float)*(kps_+1));
				kps_arr[kps_] = 1.f;
				kps_size = kps_;
			}
			if(kps_ > max_kps)
			{
				max_kps = kps_;
				//limit = 1.0;
			}
	
			if(curr >= count)
				break;
		}
		if(i - times[last_kps] >= 1000)
		{
			if(!indices[last_kps])
			{
				kps_avg -= avgs[last_kps];
				kps_--;
			}
			
			last_kps++;
		}
	}
	
	/*for(int i = 0; i < kps_size; i++)
	{
		if(kps_arr[i] > 0)
		{
			speed_ += log(exp(1000.0/i));
			break;
		}
	}*/
	
	free(avgs);
	free(kps_arr);
	free(tech);
	free(times);
	free(indices);
	
	for(int i = 0; i < 64; i++)
	{
		if(patterns[i] != 0)
			newval += log(patterns[i]);
	}
	/*
	printf("\n");
	
	int tot = 0;
	for(int i = 1; i < 4; i++)
	{
		for(int j = 1; j < 4; j++)
		{
			for(int k = 1; k < 4; k++)
			{
				if(i == j && j == k)
					continue;
				p = (i)|(j<<2)|(k<<4);
				printf("%d%d%d: %d\n",k,j,i,patterns[p]);
				tot += patterns[p];
			}
		}
	}
	printf("TOTAL: %d\n",tot);*/

	if((stamina_/1000.0+speed_/1000.0)/10.0 < 1)
		return (stamina_/1000.0+speed_/1000.0)+1+newval/10;
	else
		return (stamina_/1000.0+speed_/1000.0)/10.0+10+newval/10;
}