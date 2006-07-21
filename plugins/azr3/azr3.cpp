#include <iostream>
#include <cmath>

#include "azr3.hpp"
#include "lv2-miditype.h"
#include "lv2instrument.hpp"
#include "voice_classes.h"


using namespace std;


AZR3::AZR3(unsigned long rate, const char* bundle_path, 
           const LV2_Host_Feature**)
  : LV2Instrument(kNumParams + 3),
    n1(NUMOFVOICES),
    samplerate(rate),
    vdelay1(441,true),
    vdelay2(441,true),
    wand_r(4410,false),
    wand_l(4410,false),
    delay1(4410,true),
    delay2(4410,true),
    delay3(4410,true),
    delay4(4410,true),
    mono_before(0),
    samplecount(0),
    mono(0),
    mono1(0),
    mono2(0),
    viblfo(0),
    vmix1(0),
    vmix2(0),
    oldmrvalve(0),
    oldmix(0),
    dist(0),
    fuzz(0),
    odmix(0),
    oldspread(0),
    spread(0),
    spread2(0),
    cross1(0),
    lslow(0),
    lfast(0),
    uslow(0),
    ufast(0),
    ubelt_up(0),
    ubelt_down(0),
    lbelt_up(0),
    lbelt_down(0),
    lspeed(0),
    uspeed(0),
    er_r(0),
    er_l(0),
    lp(0),
    right(0),
    left(0),
    lright(0),
    lleft(0),
    upper(0),
    lower(0),
    llfo_out(0),
    llfo_nout(0),
    llfo_d_out(0),
    llfo_d_nout(0),
    lfo_out(0),
    lfo_nout(0),
    lfo_d_out(0),
    lfo_d_nout(0),
    last_out1(0),
    last_out2(0),
    splitpoint(0),
    gp_value(0),
    last_shape(-1),
    mute(true),
    is_real_param(kNumParams, false),
    real_param(kNumParams, -1) {
  
  is_real_param[n_perc] = true;
  real_param[n_perc] = 0;
  is_real_param[n_percvol] = true;
  real_param[n_percvol] = 1;
  is_real_param[n_percfade] = true;
  real_param[n_percfade] = 2;
  is_real_param[n_vol1] = true;
  real_param[n_vol1] = 3;
  is_real_param[n_vol2] = true;
  real_param[n_vol2] = 4;
  is_real_param[n_vol3] = true;
  real_param[n_vol3] = 5;
  is_real_param[n_master] = true;
  real_param[n_master] = 6;
  is_real_param[n_1_perc] = true;
  real_param[n_1_perc] = 7;
  is_real_param[n_2_perc] = true;
  real_param[n_2_perc] = 8;
  is_real_param[n_3_perc] = true;
  real_param[n_3_perc] = 9;
  is_real_param[n_mrvalve] = true;
  real_param[n_mrvalve] = 10;
  is_real_param[n_drive] = true;
  real_param[n_drive] = 11;
  is_real_param[n_set] = true;
  real_param[n_set] = 12;
  is_real_param[n_tone] = true;
  real_param[n_tone] = 13;
  is_real_param[n_mix] = true;
  real_param[n_mix] = 14;
  is_real_param[n_speakers] = true;
  real_param[n_speakers] = 15;
  is_real_param[n_speed] = true;
  real_param[n_speed] = 16;
  is_real_param[n_l_slow] = true;
  real_param[n_l_slow] = 17;
  is_real_param[n_l_fast] = true;
  real_param[n_l_fast] = 18;
  is_real_param[n_u_slow] = true;
  real_param[n_u_slow] = 19;
  is_real_param[n_u_fast] = true;
  real_param[n_u_fast] = 20;
  is_real_param[n_belt] = true;
  real_param[n_belt] = 21;
  is_real_param[n_spread] = true;
  real_param[n_spread] = 22;
  is_real_param[n_splitpoint] = true;
  real_param[n_splitpoint] = 23;
  is_real_param[n_complex] = true;
  real_param[n_complex] = 24;
  is_real_param[n_pedalspeed] = true;
  real_param[n_pedalspeed] = 25;
  is_real_param[n_1_sustain] = true;
  real_param[n_1_sustain] = 26;
  is_real_param[n_2_sustain] = true;
  real_param[n_2_sustain] = 27;
  is_real_param[n_3_sustain] = true;
  real_param[n_3_sustain] = 28;

  pthread_mutex_init(&m_lock, 0);
  
	for(int x=0;x<WAVETABLESIZE*12+1;x++)
		wavetable[x]=0;

	for(int x=0;x<kNumParams;x++)
		last_value[x]=-99;

	if(samplerate<8000)
		samplerate=44100;

	setSampleRate(rate);

	setFactorySounds();
	my_p=programs[0].p;

	make_waveforms(W_SINE);
}


AZR3::~AZR3() {
  pthread_mutex_destroy(&m_lock);
}


void AZR3::activate() {

	mute = false;

	for(int x = 0; x < 4; x++) {
    allpass_r[x].reset();
    allpass_l[x].reset();
  }
  
	delay1.flood(0);
	delay2.flood(0);
	delay3.flood(0);
	delay4.flood(0);
	vdelay1.flood(0);
	vdelay2.flood(0);
	wand_r.flood(0);
	wand_l.flood(0);
}


const LV2_ProgramDescriptor* AZR3::get_program(unsigned long index) {
  if (index < kNumPrograms) {
    pdesc.name = programs[index].name;
    pdesc.number = index;
    return &pdesc;
  }
  return NULL;
}


void AZR3::select_program(unsigned long program) {
  
  if (program >= kNumPrograms)
    return;
  
  pthread_mutex_lock(&m_lock);
  
	flpProgram& ap = programs[program];

  for(unsigned long x = 0; x < kNumParams; x++) {
    if (is_real_param[x])
      *static_cast<float*>(m_ports[real_param[x]]) = ap.p[x];
    else
      setParameter(x, ap.p[x]);
  }
  
  pthread_mutex_unlock(&m_lock);
}


void AZR3::run(unsigned long sampleFrames) {
  
  /*
    OK, here we go. This is the order of actions in here:
    - process event queue
    - return zeroes if in "mute" state
    - clock the "notemaster" and get the combined sound output
    from the voices.
    We actually get three values, one for each keyboard.
    They are added according to the assigned channel volume
    control values.
    - calculate switch smoothing to prevent clicks
    - vibrato
    - additional low pass "warmth"
    - distortion
    - speakers
	*/
  
  midi_ptr = static_cast<LV2_MIDI*>(m_ports[29])->data;
	out1 = static_cast<float*>(m_ports[30]);
	out2 = static_cast<float*>(m_ports[31]);
  
  // set percussion parameters
  {
    int v = (int)(*(float*)m_ports[real_param[n_perc]] * 10);
    float pmult;
    if(v < 1)
      pmult = 0;
    else if(v < 2)
      pmult = 1;
    else if(v < 3)
      pmult = 2;
    else if(v < 4)
      pmult = 3;
    else if(v < 5)
      pmult = 4;
    else if(v < 6)
      pmult = 6;
    else if(v < 7)
      pmult = 8;
    else if(v < 8)
      pmult = 10;
    else if(v < 9)
      pmult = 12;
    else
      pmult = 16;
    n1.set_percussion(1.5f * *(float*)m_ports[real_param[n_percvol]], 
                      pmult, *(float*)m_ports[real_param[n_percfade]]);
  }
  
  // set volumes
  n1.set_volume(*static_cast<float*>(m_ports[real_param[n_vol1]]) * 0.3f, 0);
  n1.set_volume(*static_cast<float*>(m_ports[real_param[n_vol2]]) * 0.4f, 1);
  n1.set_volume(*static_cast<float*>(m_ports[real_param[n_vol3]]) * 0.6f, 2);
  
  // has the distortion switch changed?
  if (*static_cast<float*>(m_ports[real_param[n_mrvalve]]) != oldmrvalve) {
    odchanged = true;
    oldmrvalve = *static_cast<float*>(m_ports[real_param[n_mrvalve]]);
  }
  
  // compute distortion parameters
  float value = *static_cast<float*>(m_ports[real_param[n_drive]]);
  if (value > 0)
    do_dist = true;
  else
    do_dist = false;
  dist = 2 * (0.1f + value);
  sin_dist = sinf(dist);
  i_dist = 1 / dist;
  dist4 = 4 * dist;
  dist8 = 8 * dist;
  fuzz_filt.setparam(800 + *static_cast<float*>(m_ports[real_param[n_tone]]) *
                     3000, 0.7f, samplerate);
  value = *static_cast<float*>(m_ports[real_param[n_mix]]);
  if (value != oldmix) {
    odmix = value;
    if (*static_cast<float*>(m_ports[real_param[n_mrvalve]]) == 1)
      odchanged = true;
  }

  // speed control port
  if (*static_cast<float*>(m_ports[real_param[n_speed]]) > 0.5f)
    fastmode = true;
  else
    fastmode = false;
  
  // different rotation speeds
  lslow = 10 * *static_cast<float*>(m_ports[real_param[n_l_slow]]);
  lfast = 10 * *static_cast<float*>(m_ports[real_param[n_l_fast]]);
  uslow = 10 * *static_cast<float*>(m_ports[real_param[n_u_slow]]);
  ufast = 10 * *static_cast<float*>(m_ports[real_param[n_u_fast]]);
  
  // belt (?)
  value = *static_cast<float*>(m_ports[real_param[n_belt]]);
  ubelt_up = (value * 3 + 1) * 0.012f;
  ubelt_down = (value * 3 + 1) * 0.008f;
  lbelt_up = (value * 3 + 1) * 0.0045f;
  lbelt_down = (value * 3 + 1) * 0.0035f;
  
  if (oldspread != *static_cast<float*>(m_ports[real_param[n_spread]])) {
    lfos_ok = false;
    oldspread = *static_cast<float*>(m_ports[real_param[n_spread]]);
  }
  
  // keyboard split
  splitpoint = (long)(*static_cast<float*>(m_ports[real_param[n_splitpoint]])
                      * 128);
  
	int	x;

  if (pthread_mutex_trylock(&m_lock)) {
    memset(out1, 0, sizeof(float) * sampleFrames);
    memset(out2, 0, sizeof(float) * sampleFrames);
  }
	
  
  for (unsigned long pframe = 0; pframe < sampleFrames; ++pframe) {
    
		// we need this variable further down
		samplecount++;
		if(samplecount > 10000)
			samplecount = 0;
		
		// read events from our own event queue
		while((evt = this->event_clock(pframe)) != NULL) {
			unsigned char channel = evt[0] & 0x0F;
			float* tbl;
			
			if (channel > 2)
				channel = 0;
			switch (evt[0] & 0xF0)
			{
			case evt_noteon: {
        unsigned char note = evt[1];
        bool percenable = false;
        float sustain = my_p[n_sustain] + .0001f;
					
        // here we choose the correct wavetable according to the played note
#define foldstart 80
        if (note > foldstart + 12 + 12)
          tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL + 
                           WAVETABLESIZE * 7];
        else if (note > foldstart + 12 + 8)
          tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
                           WAVETABLESIZE * 6];
        else if (note > foldstart + 12 + 5)
          tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
                           WAVETABLESIZE * 5];
        else if (note > foldstart + 12)
          tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
                           WAVETABLESIZE * 4];
        else if (note > foldstart + 8)
          tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL + 
                           WAVETABLESIZE * 3];
        else if (note > foldstart + 5)
          tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
                           WAVETABLESIZE * 2];
        else if (note > foldstart)
          tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL +
                           WAVETABLESIZE];
        else
          tbl = &wavetable[channel * WAVETABLESIZE * TABLES_PER_CHANNEL];
        
        if (channel == 0) {
          if (*(float*)m_ports[real_param[n_1_perc]] > 0)
            percenable = true;
          if (*(float*)m_ports[real_param[n_1_sustain]] < 0.5f)
            sustain = 0;
        }
        else if (channel == 1) {
          if (*(float*)m_ports[real_param[n_2_perc]] > 0)
            percenable = true;
          if (*(float*)m_ports[real_param[n_2_sustain]] < 0.5f)
            sustain = 0;
        }
        else if (channel == 2) {
          if (*(float*)m_ports[real_param[n_3_perc]] > 0)
            percenable = true;
          if (*(float*)m_ports[real_param[n_3_sustain]] < 0.5f)
            sustain = 0;
        }
					
        n1.note_on(note, evt[2], tbl, WAVETABLESIZE, 
                   channel, percenable, click[channel], sustain);
				
        break;
      }
        
			case evt_noteoff:
				n1.note_off(evt[1], channel);
				break;
        
      case 0xB0:
        break;
        
			case evt_alloff:
				n1.all_notes_off();
				break;
			case evt_pedal:
				n1.set_pedal(evt[1], channel);
				break;
			case evt_pitch:
				n1.set_pitch(evt[1], channel);
				break;
			}
		}
		
		p_mono = n1.clock();
		mono1 = p_mono[0];
		mono2 = p_mono[1];
		mono = p_mono[2];
    
		// smoothing of vibrato switch 1
		if (vibchanged1 && samplecount % 10 == 0) {
			if(my_p[n_1_vibrato] == 1) {
				vmix1 += 0.01f;
				if (vmix1 >= my_p[n_1_vmix])
					vibchanged1 = false;
			}
			else {
				vmix1 -= 0.01f;
				if (vmix1 <= 0)
					vibchanged1 = false;
			}
		}
		
		// smoothing of vibrato switch 2
		if(vibchanged2 && samplecount % 10 == 0) {
			if(my_p[n_2_vibrato] == 1) {
				vmix2 += 0.01f;
				if (vmix2 >= my_p[n_2_vmix])
					vibchanged2 = false;
			}
			else {
				vmix2 -= 0.01f;
				if (vmix2 <= 0)
					vibchanged2 = false;
			}
		}
		
		// smoothing of OD switch
		if(odchanged && samplecount % 10 == 0) {
			if(*static_cast<float*>(m_ports[real_param[n_mrvalve]]) > 0.5) {
				odmix += 0.05f;
				if (odmix >= *static_cast<float*>(m_ports[real_param[n_mix]]))
					odchanged = false;
			}
			else {
				odmix -= 0.05f;
				if (odmix <= 0)
					odchanged = false;
			}
			n_odmix = 1 - odmix;
			n2_odmix = 2 - odmix;
			odmix75 = 0.75f * odmix;
			n25_odmix = n_odmix * 0.25f;
		}
		
		// Vibrato LFO
		lfo_calced = false;
		
		// Vibrato 1
		if(my_p[n_1_vibrato] == 1 || vibchanged1) {
			if(samplecount % 5 == 0) {
				viblfo = vlfo.clock();
				lfo_calced = true;
				vdelay1.set_delay(viblfo * 2 * my_p[n_1_vstrength]);
			}
			mono1 = (1 - vmix1) * mono1 + vmix1 * vdelay1.clock(mono1);
		}
		
		// Vibrato 2
		if(my_p[n_2_vibrato] == 1 || vibchanged2) {
			if(samplecount % 5 == 0) {
				if(!lfo_calced)
					viblfo = vlfo.clock();
				vdelay2.set_delay(viblfo * 2 * my_p[n_2_vstrength]);
			}
			mono2 = (1 - vmix2) * mono2 + vmix2 * vdelay2.clock(mono2);
		}
		
		
		mono += mono1 + mono2;
		mono *= 1.4f;
		
		// Mr. Valve
		/*
      Completely rebuilt.
      Multiband distortion:
      The first atan() waveshaper is applied to a lower band. The second
      one is applied to the whole spectrum as a clipping function (combined
      with an fabs() branch).
      The "warmth" filter is now applied _after_ distortion to flatten
      down distortion overtones. It's only applied with activated distortion
      effect, so we can switch warmth off and on without adding another 
      parameter.
		*/
		if (*static_cast<float*>(m_ports[real_param[n_mrvalve]]) > 0.5 || 
                             odchanged) {
			if (do_dist) {
				body_filt.clock(mono);
				postbody_filt.clock(atanf(body_filt.lp() * dist8) * 6);
				fuzz = atanf(mono * dist4) * 0.25f + 
          postbody_filt.bp() + postbody_filt.hp();
        
				if (_fabsf(mono) > *static_cast<float*>(m_ports[real_param[n_set]]))
					fuzz = atanf(fuzz * 10);
				fuzz_filt.clock(fuzz);
				mono = ((fuzz_filt.lp() * odmix * sin_dist + mono * (n2_odmix)) * 
                sin_dist) * i_dist;
			}
			else {
				fuzz_filt.clock(mono);
				mono = fuzz_filt.lp() * odmix75 + mono * n25_odmix * i_dist;
			}
			mono = warmth.clock(mono);			
		}
		
		// Speakers
		/*
      I started the rotating speaker sim from scratch with just
      a few sketches about how reality looks like:
      Two horn speakers, rotating in a circle. Combined panning
      between low and mid filtered sound and the volume. Add the
      doppler effect. Let the sound of one speaker get reflected
      by a wall and mixed with the other speakers' output. That's
      all not too hard to calculate and to implement in C++, and
      the results were already quite realistic. However, to get
      more density and the explicit "muddy" touch I added some
      phase shifting gags and some unexpected additions with
      the other channels' data. The result did take many nights
      of twiggling wih parameters. There are still some commented
      alternatives; feel free to experiment with the emulation.
      Never forget to mono check since there are so many phase
      effects in here you might end up in the void.
      I'm looking forward to the results...
		*/
		
		/*
      Update:
      I added some phase shifting using allpass filters.
      This should make it sound more realistic.
		*/
		
		if (*static_cast<float*>(m_ports[real_param[n_speakers]]) > 0.5) {
			if (samplecount % 100 == 0) {
        if (fastmode) {
					if (lspeed < lfast)
						lspeed += lbelt_up;
					if (lspeed > lfast)
						lspeed=lfast;
					
					if (uspeed < ufast)
						uspeed += ubelt_up;
					if (uspeed > ufast)
						uspeed = ufast;
				}
				else {
					if (lspeed > lslow)
						lspeed -= lbelt_down;
					if (lspeed < lslow)
						lspeed = lslow;
					if (uspeed > uslow)
						uspeed -= ubelt_down;
					if (uspeed < uslow)
						uspeed = uslow;
				}

				//recalculate mic positions when "spread" has changed
				if(!lfos_ok) {
					float s = (*static_cast<float*>(m_ports[real_param[n_spread]])
                     + 0.5f) * 0.8f;
					spread = (s) * 2 + 1;
					spread2 = (1 - spread) / 2;
          // this crackles - use offset_phase instead
					//lfo1.set_phase(0);
					//lfo2.set_phase(s / 2);
					//lfo3.set_phase(0);
					//lfo4.set_phase(s / 2);
          lfo2.offset_phase(lfo1, s / 2);
          lfo4.offset_phase(lfo3, s / 2);
          
					cross1 = 1.5f - 1.2f * s;
					// early reflections depend upon mic position.
					// we want less e/r if mics are positioned on
					// opposite side of speakers.
					// when positioned right in front of them e/r
					// brings back some livelyness.
					//
					// so "spread" does the following to the mic positions:
					// minimum: mics are almost at same position (mono) but
					// further away from cabinet.
					// maximum: mics are on opposite sides of cabinet and very
					// close to speakers.
					// medium: mics form a 90� angle, heading towards cabinet at
					// medium distance.
					er_feedback = 0.03f * cross1;
					lfos_ok = true;
				}
				
				if (lspeed != lfo3.get_rate()) {
					lfo3.set_rate(lspeed * 5, 1);
					lfo4.set_rate(lspeed * 5, 1);
				}
				
				if (uspeed != lfo1.get_rate()) {
					lfo1.set_rate(uspeed * 5, 1);
					lfo2.set_rate(uspeed * 5, 1);
				} 
			}

			// split signal into upper and lower cabinet speakers
			split.clock(mono);
			lower = split.lp() * 5;
			upper = split.hp();
			
			// upper speaker is kind of a nasty horn - this makes up
			// a major part of the typical sound!
			horn_filt.clock(upper);
			upper = upper * 0.5f + horn_filt.lp() * 2.3f;
			damp.clock(upper);
			upper_damp = damp.lp();
			
			// do lfo stuff
			if(samplecount % 5 == 0) {
				lfo_d_out = lfo1.clock();
				lfo_d_nout = 1 - lfo_d_out;
				
				delay1.set_delay(10 + lfo_d_out * 0.8f);
				delay2.set_delay(17 + lfo_d_nout * 0.8f);
				
				lfo_d_nout = lfo2.clock();
				
				lfo_out = lfo_d_out * spread + spread2;
				lfo_nout = lfo_d_nout * spread + spread2;

				// phase shifting lines
				// (do you remember? A light bulb and some LDRs...
				//  DSPing is so much nicer than soldering...)
				lfo_phaser1 = (1 - cosf(lfo_d_out * 1.8f) + 1) * 0.054f;
				lfo_phaser2 = (1 - cosf(lfo_d_nout * 1.8f) + 1) * .054f;
				for(x = 0; x < 4; x++) {
					allpass_r[x].set_delay(lfo_phaser1);
					allpass_l[x].set_delay(lfo_phaser2);
				}

				if(lslow > 0) {
					llfo_d_out = lfo3.clock();
					llfo_d_nout = 1 - llfo_d_out;
				}
				
				// additional delay lines in complex mode
				if(*static_cast<float*>(m_ports[real_param[n_complex]]) > 0.5f) {
					delay4.set_delay(llfo_d_out + 15);
					delay3.set_delay(llfo_d_nout + 25);
				}
				
				llfo_d_nout = lfo4.clock();
				llfo_out = llfo_d_out * spread + spread2;
				llfo_nout = llfo_d_nout * spread + spread2;
			}
			
			if(lslow > 0) {
				lright = (1 + 0.6f * llfo_out) * lower;
				lleft = (1 + 0.6f * llfo_nout) * lower;
			}
			else {
				lright = lleft = lower;
			}
			
			// emulate vertical horn characteristics
			// (sound is dampened when listened from aside)
			right = (3 + lfo_nout * 2.5f) * upper + 1.5f * upper_damp;
			left = (3 + lfo_out * 2.5f) * upper + 1.5f * upper_damp;

			//phaser...
			last_r = allpass_r[0].clock(
				allpass_r[1].clock(
				allpass_r[2].clock(
				allpass_r[3].clock(upper + last_r * 0.33f))));
			last_l = allpass_l[0].clock(
				allpass_l[1].clock(
				allpass_l[2].clock(
				allpass_l[3].clock(upper + last_l * 0.33f))));

			right += last_r;
			left += last_l;
			
			// rotating speakers can only develop in a live room -
			// wouldn't work without some early reflections.
			er_r = wand_r.clock(right + lright - (left * 0.3f) - er_l * er_feedback);
			er_r = DENORMALIZE(er_r);
			er_l = wand_l.clock(left + lleft - (right * .3f) - 
                          er_r_before * er_feedback);
			er_l = DENORMALIZE(er_l);
			er_r_before = er_r;
			

			// We use two additional delay lines in "complex" mode
			if(*static_cast<float*>(m_ports[real_param[n_complex]]) > 0.5f) {
				right = right * 0.3f + 1.5f * er_r + 
          delay1.clock(right) + delay3.clock(er_r);
				left = left * 0.3f + 1.5f * er_l + 
          delay2.clock(left) + delay4.clock(er_l);
			}
			else {
				right = right * 0.3f + 1.5f * er_r + delay1.clock(right) + lright;
				left = left * 0.3f + 1.5f * er_l + delay2.clock(left) + lleft;
			}
			
			right *= 0.033f;
			left *= 0.033f;
			
			// spread crossover (emulates mic positions)
			last_out1 = (left + cross1 * right) * *(float*)(m_ports[real_param[n_master]]);
			last_out2 = (right + cross1 * left) * *(float*)(m_ports[real_param[n_master]]);
		}
		else {
			last_out1 = last_out2 = mono * *(float*)(m_ports[real_param[n_master]]);
		}
		if(mute) {
			last_out1 = 0;
			last_out2 = 0;
		}
    
		(*out1++) = last_out1;
		(*out2++) = last_out2;
	}
  
  pthread_mutex_unlock(&m_lock);
  
}


char* AZR3::configure(const char* key, const char* value) {
  
  string param = "param";
  if (!strncmp(key, param.c_str(), param.size())) {
    long port = atol(key + param.size());
    float fvalue = atof(value);
    pthread_mutex_lock(&m_lock);
    setParameter(port, fvalue);
    pthread_mutex_unlock(&m_lock);
  }
}


void AZR3::setSampleRate(float sampleRate) {

	samplerate = sampleRate;
	
	warmth.setparam(2700, 1.2f, sampleRate);

  n1.set_samplerate(samplerate);
  vdelay1.set_samplerate(samplerate);
  vdelay2.set_samplerate(samplerate);
	vlfo.set_samplerate(samplerate);
	vlfo.set_rate(35, 0);
	split.setparam(400, 1.3f, samplerate);
	horn_filt.setparam(2500, .5f, samplerate);
	damp.setparam(200, .9f, samplerate);
  wand_r.set_samplerate(samplerate);
  wand_r.set_delay(35);
  wand_l.set_samplerate(samplerate);
  wand_l.set_delay(20);

  delay1.set_samplerate(samplerate);
  delay2.set_samplerate(samplerate);
  delay3.set_samplerate(samplerate);
  delay4.set_samplerate(samplerate);
	lfo1.set_samplerate(samplerate);
	lfo2.set_samplerate(samplerate);
	lfo3.set_samplerate(samplerate);
	lfo4.set_samplerate(samplerate);

	body_filt.setparam(190, 1.5f, samplerate);
	postbody_filt.setparam(1100, 1.5f, samplerate);
}


/*
	OK, this "factory sound" stuff looks weird,
	but it works just perfect. I wrote a mini tool
	which analyzes a Cubase program bank file and
	writes just these lines. This makes it very easy
	to implement "factory sounds".
*/
void AZR3::setFactorySounds() {
	int x = 0;
// 32 programs, ID=FLP5
	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.9300f,
			0.2000f,
			0.4800f,
			0.4300f,
			0.3300f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.3400f,
			0.3000f,
			1.0000f,
			0.6500f,
			0.0000f,
			1.0000f,
			0.1900f,
			0.5400f,
			0.0000f,
			0.2300f,
			0.1500f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			1.0000f,
			0.2100f,
			0.3400f,
			0.3700f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3010f,
			0.3510f,
			0.0000f,
			1.0000f,
			0.5900f,
			0.9400f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.4400f,
			1.0000f,
			0.2700f,
			0.1800f,
			1.0000f,
			0.0000f,
			0.0900f,
			0.4800f,
			0.0470f,
			0.6500f,
			0.5000f,
			0.4200f,
			1.0000f,
			1.0000f,
			0.4609f,
			0.6700f,
			0.0000f,
			0.0000f,
			1.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"muddy moods SPLIT");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.5200f,
			0.2000f,
			0.4000f,
			0.5000f,
			1.0000f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.7900f,
			0.2300f,
			1.0000f,
			1.0000f,
			1.0000f,
			1.0000f,
			1.0000f,
			1.0000f,
			1.0000f,
			1.0000f,
			1.0000f,
			1.0000f,
			1.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			1.0000f,
			0.2100f,
			0.3400f,
			0.3700f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3010f,
			0.3510f,
			0.0000f,
			1.0000f,
			0.2100f,
			0.6700f,
			0.0900f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.5000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0400f,
			0.4700f,
			0.0500f,
			0.6800f,
			0.5000f,
			0.5100f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"Volle Kante");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2200f,
			0.2000f,
			0.3000f,
			0.0000f,
			0.1000f,
			0.5000f,
			0.3700f,
			0.6000f,
			0.6600f,
			0.3900f,
			1.0000f,
			1.0000f,
			0.8800f,
			0.0000f,
			0.5700f,
			0.0000f,
			0.5500f,
			0.0000f,
			0.5100f,
			0.4600f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			1.0000f,
			0.2100f,
			0.3400f,
			0.3700f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3010f,
			0.3510f,
			0.0000f,
			1.0000f,
			0.3600f,
			0.4500f,
			0.0900f,
			0.0000f,
			0.0000f,
			0.4400f,
			1.0000f,
			0.1000f,
			0.2500f,
			1.0000f,
			0.0000f,
			0.0900f,
			0.4700f,
			0.0470f,
			0.6600f,
			0.4200f,
			0.3200f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"clean");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.5100f,
			0.2000f,
			0.0000f,
			0.7500f,
			0.2700f,
			1.0000f,
			0.3400f,
			0.6000f,
			0.3100f,
			0.5400f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0900f,
			0.4800f,
			0.0000f,
			0.3700f,
			0.1700f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			1.0000f,
			0.2100f,
			0.3400f,
			0.3700f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3010f,
			0.3510f,
			0.0000f,
			1.0000f,
			0.6300f,
			0.4100f,
			0.4400f,
			0.0000f,
			1.0000f,
			0.5600f,
			1.0000f,
			1.0000f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.0400f,
			0.6000f,
			0.0770f,
			0.7300f,
			1.0000f,
			0.6400f,
			0.0000f,
			1.0000f,
			0.4609f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"hollow SPLIT");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.3000f,
			0.3400f,
			0.3700f,
			1.0000f,
			0.6900f,
			0.6000f,
			0.0000f,
			0.5400f,
			1.0000f,
			0.2200f,
			1.0000f,
			0.0000f,
			0.0200f,
			0.0800f,
			0.0000f,
			0.2300f,
			0.1500f,
			0.1800f,
			1.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.2500f,
			0.4400f,
			1.0000f,
			0.0000f,
			0.0600f,
			0.5300f,
			0.0400f,
			0.7500f,
			0.5000f,
			0.1900f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"talking space");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			1.0000f,
			0.2000f,
			0.0000f,
			0.3600f,
			0.3900f,
			0.9700f,
			0.5300f,
			0.6000f,
			0.3400f,
			0.2100f,
			1.0000f,
			1.0000f,
			0.7800f,
			0.0000f,
			0.6500f,
			0.0000f,
			0.4961f,
			0.2300f,
			0.1200f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.6400f,
			0.4900f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.3100f,
			1.0000f,
			0.4500f,
			0.2700f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.4900f,
			0.0470f,
			0.6700f,
			0.2100f,
			0.5000f,
			1.0000f,
			1.0000f,
			0.4609f,
			0.0500f,
			0.0000f,
			0.0000f,
			1.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"fat amped SPLIT");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			1.0000f,
			0.2000f,
			0.6800f,
			0.6800f,
			0.7500f,
			0.1000f,
			0.5500f,
			0.6000f,
			0.0000f,
			0.2200f,
			1.0000f,
			0.6500f,
			1.0000f,
			1.0000f,
			0.4500f,
			0.0000f,
			0.4500f,
			1.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.4600f,
			0.1000f,
			0.3800f,
			0.2400f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0670f,
			0.7800f,
			0.5000f,
			0.2200f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"FLPs fiese Forfiesa");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.0000f,
			0.2000f,
			0.5000f,
			0.2200f,
			0.3900f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.4800f,
			1.0000f,
			1.0000f,
			0.4540f,
			0.2170f,
			0.3550f,
			0.1510f,
			0.3030f,
			0.0960f,
			0.1890f,
			0.0000f,
			1.0000f,
			0.5000f,
			0.3800f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.0000f,
			1.0000f,
			0.5100f,
			1.0000f,
			1.0000f,
			1.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7100f,
			1.0000f,
			0.1700f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"NHL");
	}

	{
		float mp[kNumParams]=
		{
			1.0000f,
			0.0000f,
			0.2000f,
			0.0000f,
			0.4300f,
			0.2000f,
			0.5000f,
			0.7000f,
			0.6000f,
			0.0000f,
			0.7000f,
			1.0000f,
			1.0000f,
			0.4300f,
			0.3050f,
			0.2400f,
			0.0990f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.6450f,
			0.0000f,
			0.7400f,
			0.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0300f,
			1.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"FLPs cool bass 1");
	}

	{
		float mp[kNumParams]=
		{
			1.0000f,
			0.0000f,
			0.2000f,
			0.0000f,
			0.5200f,
			0.1400f,
			0.5000f,
			0.7800f,
			0.6000f,
			0.0000f,
			0.7000f,
			1.0000f,
			1.0000f,
			0.4200f,
			0.3050f,
			0.1900f,
			0.1690f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.6450f,
			0.0000f,
			0.7400f,
			0.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"FLPs cool bass 2");
	}

	{
		float mp[kNumParams]=
		{
			1.0000f,
			0.0000f,
			0.2000f,
			0.0000f,
			0.5200f,
			0.2100f,
			0.5000f,
			0.7000f,
			0.6000f,
			0.0000f,
			0.4000f,
			1.0000f,
			1.0000f,
			0.2500f,
			0.1450f,
			0.0900f,
			0.0390f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.5400f,
			1.0000f,
			0.0000f,
			0.2600f,
			0.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.1000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"FLPs even cooler bass");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.7200f,
			0.2000f,
			0.0000f,
			1.0000f,
			0.3000f,
			0.0100f,
			0.5600f,
			0.6000f,
			0.0000f,
			0.2800f,
			1.0000f,
			0.1600f,
			0.5900f,
			1.0000f,
			0.0000f,
			0.9900f,
			0.3000f,
			0.4200f,
			0.8900f,
			0.2400f,
			1.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.3500f,
			1.0000f,
			0.3700f,
			0.6000f,
			1.0000f,
			1.0000f,
			0.1000f,
			0.3900f,
			0.0470f,
			1.0000f,
			1.0000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"cutter");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.7300f,
			0.0000f,
			1.0000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.1700f,
			1.0000f,
			0.7600f,
			0.3400f,
			0.0700f,
			0.1400f,
			0.5800f,
			0.2700f,
			1.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.5200f,
			0.2400f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.1100f,
			0.6800f,
			0.0570f,
			0.7700f,
			0.5000f,
			0.1100f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"MODEM");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.0400f,
			0.2000f,
			0.0000f,
			0.2100f,
			1.0000f,
			1.0000f,
			0.5200f,
			0.6000f,
			0.0000f,
			0.4500f,
			1.0000f,
			1.0000f,
			0.5900f,
			0.0000f,
			0.0000f,
			0.0500f,
			0.0000f,
			0.0200f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.7000f,
			0.2900f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.5700f,
			0.4700f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.2900f,
			0.6500f,
			0.1300f,
			0.7800f,
			0.5000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.9800f,
			1.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"sine vibra");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			1.0000f,
			0.2000f,
			0.4600f,
			0.5700f,
			0.3700f,
			0.2000f,
			0.6000f,
			0.6000f,
			0.4400f,
			0.2900f,
			1.0000f,
			1.0000f,
			0.8800f,
			0.3700f,
			0.7500f,
			0.0000f,
			0.5800f,
			0.5400f,
			0.0000f,
			0.0800f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.5300f,
			0.4100f,
			0.6500f,
			0.0000f,
			1.0000f,
			0.3900f,
			1.0000f,
			0.4500f,
			0.1900f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6800f,
			0.0470f,
			0.7200f,
			0.5000f,
			0.4200f,
			1.0000f,
			1.0000f,
			0.4297f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"some rotz SPLIT");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.3300f,
			0.2000f,
			1.0000f,
			0.7300f,
			0.2400f,
			1.0000f,
			0.4600f,
			0.6000f,
			0.0000f,
			0.3400f,
			1.0000f,
			1.0000f,
			0.2300f,
			0.0000f,
			0.1100f,
			0.6400f,
			0.0000f,
			0.4500f,
			0.8400f,
			0.4100f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.1600f,
			0.4800f,
			0.2100f,
			0.1000f,
			1.0000f,
			0.0000f,
			0.0400f,
			0.2500f,
			0.0570f,
			0.6100f,
			0.7100f,
			0.3800f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"thin");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.5500f,
			0.2000f,
			0.6000f,
			0.1800f,
			0.4400f,
			0.0000f,
			0.4900f,
			0.6000f,
			0.0000f,
			0.6000f,
			1.0000f,
			0.4400f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.2600f,
			0.4900f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.2800f,
			1.0000f,
			0.3400f,
			0.1900f,
			1.0000f,
			0.0000f,
			0.0400f,
			0.6400f,
			0.0770f,
			0.7700f,
			1.0000f,
			0.6900f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"angel hair");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.4500f,
			0.2000f,
			0.5300f,
			0.5600f,
			1.0000f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2400f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.2100f,
			0.2700f,
			0.6200f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0670f,
			0.7800f,
			0.7700f,
			0.4200f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"little duck");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.0000f,
			0.2000f,
			0.6200f,
			0.4500f,
			0.2400f,
			1.0000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.4500f,
			1.0000f,
			1.0000f,
			0.7900f,
			0.0000f,
			0.4300f,
			0.0000f,
			0.1800f,
			0.0000f,
			0.0000f,
			0.0800f,
			1.0000f,
			0.5700f,
			0.3500f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.4200f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"dead entertainer");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.9100f,
			0.2000f,
			0.4400f,
			0.6500f,
			0.3300f,
			0.6800f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.7900f,
			0.5800f,
			0.0000f,
			0.2600f,
			0.0000f,
			0.0000f,
			0.4400f,
			1.0000f,
			0.2100f,
			0.3400f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7200f,
			0.5000f,
			0.5000f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"resurrected entertainer");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.0100f,
			0.2000f,
			0.8500f,
			0.2300f,
			0.3600f,
			0.9900f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.3700f,
			1.0000f,
			1.0000f,
			0.4700f,
			0.3800f,
			0.2100f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.3000f,
			0.3000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"cheap B*ntempi");
	}

	{
		float mp[kNumParams]=
		{
			1.0000f,
			0.0000f,
			0.2000f,
			0.8600f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.5100f,
			0.6000f,
			0.0000f,
			0.6800f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0400f,
			0.0400f,
			1.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.6900f,
			1.0000f,
			0.9800f,
			0.1900f,
			1.0000f,
			0.0000f,
			0.0600f,
			0.6500f,
			0.0770f,
			0.7800f,
			0.5000f,
			0.2200f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"NDW lead");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"the rest is empty");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"make your own!");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"just turn the knobs-");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"-it's not very hard!");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"Still here?");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"Come on now!");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"You can't wait forever.");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"Give it a try.");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"Not?");
	}

	{
		float mp[kNumParams]=
		{
			0.0000f,
			0.2000f,
			0.2000f,
			0.0000f,
			0.0000f,
			0.7500f,
			0.5000f,
			0.6000f,
			0.6000f,
			0.0000f,
			0.2200f,
			0.0000f,
			1.0000f,
			1.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.3000f,
			0.3500f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.4000f,
			0.0000f,
			0.6600f,
			0.0000f,
			1.0000f,
			0.0000f,
			0.1000f,
			0.6500f,
			0.0470f,
			0.7800f,
			0.5000f,
			0.5000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
			0.0000f,
		};
		memcpy(programs[x].p,mp,sizeof(mp));
		strcpy(programs[x++].name,"--init--");
	}

}


bool AZR3::make_waveforms(int shape) {
  
  cerr<<__PRETTY_FUNCTION__<<" with shape "<<shape<<endl;
  
	long	i;
	float	amp = 0.5f;
	float	tw = 0, twfuzz;
	float	twmix = 0.5f;
	float	twdist = 0.7f;
	float	tws = (float)WAVETABLESIZE;
	
	
	if(shape == last_shape)
		return false;
	last_shape = shape;
	/*
    We don't just produce flat sine curves but slightly distorted
    and even a triangle wave can be choosen.
    
	  Though this makes things sound more interesting it's _not_ a
	  tonewheel emulation. If we wanted that we would have to calculate
	  or otherwise define different waves for every playable note. If
	  anyone wants to implement real tonewheels you will have to make
	  drastic changes:
	  - implement many wavetables and a choosing algorithm
	  - construct wavetable data either through calculations
	  or from real wave files. Tha latter is what they do at
	  n@tive 1nstrument5.
	*/
	for (i = 0; i < WAVETABLESIZE; i++) {
		float	ii = (float)i;
		
		if (shape == W_SINE1 || shape == W_SINE2 || shape == W_SINE3) {
			tw = amp *
				(sinf(ii * 2 * Pi / tws) +
         0.03f * sinf(ii * 8 * Pi / tws) +
         0.01f * sinf(ii * 12 * Pi / tws));
			
			if (shape == W_SINE2)
				twdist = 1;
			else if (shape == W_SINE3)
				twdist = 2;
			
			tw *= twdist;
			twfuzz = 2 * tw - tw * tw * tw;
			if (twfuzz > 1)
				twfuzz = 1;
			else if (twfuzz < -1)
				twfuzz = -1;
			tonewheel[i] = 0.5f * twfuzz / twdist;
		}
		else if (shape == W_TRI) {
			if (i < int(tws / 4) || i > int(tws * 0.75f))
				tw += 2 / tws;
			else
				tw -= 2 / tws;
			tonewheel[i] = tw;
		}
		else if (shape == W_SAW) {
			tw = sinf(ii * Pi / tws);
			if (i > int(tws / 2))	{
				tw = sinf((ii - tws / 2) * Pi / tws);
				tw = 1 - tw;
			}
			
			tonewheel[i] = tw - 0.5f;
		}
		else {
			tw = amp *
				(sinf(ii * 2 * Pi / tws) +
         0.03f * sinf(ii * 8 * Pi / tws) +
         0.01f * sinf(ii * 12 * Pi / tws));
			tonewheel[i]=tw;
		}
	}
	
	for (i = 0; i < WAVETABLESIZE; i++) {
		//		int	f=TONEWHEELSIZE/WAVETABLESIZE;
		int f = 1;
		int	icount;
		int i2[9];
		
		i2[0] = (int)(i * 1 * f);
		i2[1] = (int)(i * 2 * f);
		i2[2] = (int)(i * 3 * f);
		i2[3] = (int)(i * 4 * f);
		i2[4] = (int)(i * 6 * f);
		i2[5] = (int)(i * 8 * f);
		i2[6] = (int)(i * 10 * f);
		i2[7] = (int)(i * 12 * f);
		i2[8] = (int)(i * 16 * f);
		
		for (icount = 0; icount < 9; icount++) {
			while(i2[icount] >= WAVETABLESIZE)
				i2[icount] -= WAVETABLESIZE;
		}
		
		sin_16[i] = tonewheel[i2[0]];
		sin_8[i] = tonewheel[i2[1]];
		sin_513[i] = tonewheel[i2[2]];
		sin_4[i] = tonewheel[i2[3]];
		sin_223[i] = tonewheel[i2[4]];
		sin_2[i] = tonewheel[i2[5]];
		sin_135[i] = tonewheel[i2[6]];
		sin_113[i] = tonewheel[i2[7]];
		sin_1[i] = tonewheel[i2[8]];
	}
	
	return true;
}


void AZR3::setParameter (long index, float value) {
	if (index < 0 || index > kNumParams)
		return;
  
  p[index]=value;				// put value into edit buffer
  my_p=p;						// let machine use edit buffer


	{
		last_value[index]=value;
		switch(index) 
      {
      case n_1_db1:
      case n_1_db2:
      case n_1_db3:
      case n_1_db4:
      case n_1_db5:
      case n_1_db6:
      case n_1_db7:
      case n_1_db8:
      case n_1_db9:
        calc_waveforms(1);
        calc_click();
        break;
      case n_2_db1:
      case n_2_db2:
      case n_2_db3:
      case n_2_db4:
      case n_2_db5:
      case n_2_db6:
			case n_2_db7:
			case n_2_db8:
			case n_2_db9:
				calc_waveforms(2);
				calc_click();
				break;
			case n_3_db1:
			case n_3_db2:
			case n_3_db3:
			case n_3_db4:
			case n_3_db5:
				calc_waveforms(3);
				calc_click();
				break;
			case n_shape:
				if(make_waveforms(int(value * (W_NUMOF - 1) + 1) - 1)) {
					calc_waveforms(1);
					calc_waveforms(2);
					calc_waveforms(3);
				}
				break;
			case n_1_perc:
			case n_2_perc:
			case n_3_perc:
			case n_perc:
			case n_percvol:
			case n_percfade: {
        int v = (int)(my_p[n_perc] * 10);
        float pmult;
        if(v < 1)
          pmult = 0;
        else if(v < 2)
          pmult = 1;
        else if(v < 3)
          pmult = 2;
        else if(v < 4)
          pmult = 3;
        else if(v < 5)
          pmult = 4;
        else if(v < 6)
          pmult = 6;
        else if(v < 7)
          pmult = 8;
        else if(v < 8)
          pmult = 10;
        else if(v < 9)
          pmult = 12;
        else
          pmult = 16;
        
        n1.set_percussion(1.5f * my_p[n_percvol], pmult, my_p[n_percfade]);
      }
				break;
			case n_click:
				calc_click();
				break;
			case n_vol1:
				n1.set_volume(value * 0.3f, 0);
				break;
			case n_vol2:
				n1.set_volume(value * 0.4f, 1);
				break;
			case n_vol3:
				n1.set_volume(value * 0.6f, 2);
				break;
			case n_mono:
				if (value != mono_before) {
					if (value >= 0.5f)
						n1.set_numofvoices(1);
					else
						n1.set_numofvoices(NUMOFVOICES);
          
					n1.set_volume(*(float*)(m_ports[real_param[n_vol1]]) * 0.3f, 0);
					n1.set_volume(*(float*)(m_ports[real_param[n_vol2]]) * 0.3f, 1);
					n1.set_volume(*(float*)(m_ports[real_param[n_vol3]]) * 0.6f, 2);
				}
				mono_before = value;
				break;
			case n_1_vibrato:
				vibchanged1 = true;
				break;
			case n_1_vmix:
				if (my_p[n_1_vibrato] == 1) {
					vmix1 = value;
					vibchanged1 = true;
				}
				break;
			case n_2_vibrato:
				vibchanged2 = true;
				break;
			case n_2_vmix:
				if (my_p[n_2_vibrato] == 1) {
					vmix2 = value;
					vibchanged2 = true;
				}
				break;
			case n_drive:
				if (value > 0)
					do_dist = true;
				else
					do_dist = false;
				dist = 2 * (0.1f + value);
				sin_dist = sinf(dist);
				i_dist = 1 / dist;
				dist4 = 4 * dist;
				dist8 = 8 * dist;
				break;
      case n_mrvalve:
				odchanged = true;
				break;
			case n_mix:
				odmix = value;
				if (my_p[n_mrvalve] == 1)
					odchanged = true;
				break;
			case n_tone:
				fuzz_filt.setparam(800 + value * 3000, 0.7f, samplerate);
				break;
        
        /* done as control rate port 1 mapped to modwheel
			case n_speed:
				if (value > 0.5f)
					fastmode = true;
				else
					fastmode = false;
				break;
        */
        
        /* These are all done as control rate ports  2 - 6 
			case n_l_slow:
				lslow = value * 10;
				break;
			case n_l_fast:
				lfast = value * 10;
				break;
			case n_u_slow:
				uslow = value * 10;
				break;
			case n_u_fast:
				ufast = value * 10;
				break;
			case n_belt:
				ubelt_up = (value * 3 + 1) * 0.012f;
				ubelt_down = (value * 3 + 1) * 0.008f;
				lbelt_up = (value * 3 + 1) * 0.0045f;
				lbelt_down = (value * 3 + 1) * 0.0035f;
				break;
        */
        
        
			case n_spread:
				lfos_ok = false;
				break;
			case n_splitpoint:
				splitpoint = (long)(value * 128);
				break;
		}

  }
}


// make one of the three waveform sets with four complete waves
// per set. "number" is 1..3 and references the waveform set
void AZR3::calc_waveforms(int number) {
  
  cerr<<__PRETTY_FUNCTION__<<" with number "<<number<<endl;
  
	int i, c;
	float* t;
	float	this_p[kNumParams];

	for (c = 0; c < kNumParams; c++)
		this_p[c] = my_p[c];
	if (number == 2) {
		c = n_2_db1;
		t = &wavetable[WAVETABLESIZE * TABLES_PER_CHANNEL];
	}
	else if (number == 3) {
		t = &wavetable[WAVETABLESIZE * TABLES_PER_CHANNEL * 2];
		c = n_3_db1;
	}
	else {
		t = &wavetable[0];
		c = n_1_db1;
	}

  // weight to each drawbar
	this_p[c] *= 1.5f;
	this_p[c+1] *= 1.0f;
	this_p[c+2] *= 0.8f;
	this_p[c+3] *= 0.8f;
	this_p[c+4] *= 0.8f;
	this_p[c+5] *= 0.8f;
	this_p[c+6] *= 0.8f;
	this_p[c+7] *= 0.6f;
	this_p[c+8] *= 0.6f;

	for (i = 0; i < WAVETABLESIZE; i++) {
		t[i] = t[i + WAVETABLESIZE] = t[i + WAVETABLESIZE*2] = 
      t[i+WAVETABLESIZE * 3] = t[i+WAVETABLESIZE * 4] =
      t[i+WAVETABLESIZE * 5] = t[i+WAVETABLESIZE * 6] =
			t[i+WAVETABLESIZE * 7] =
			sin_16[i] * this_p[c] + 
      sin_8[i] * this_p[c+1] +
			sin_513[i] * this_p[c+2];

    /*
      This is very important for a warm sound:
      The "tone wheels" are a limited resource and they
      supply limited pitch heights. If a drawbar register
      is forced to play a tune above the highest possible
      note it will simply be transposed one octave down.
      In addition it will appear less loud; that's what
      d2, d4 and d8 are for.
    */
#define d2 0.5f
#define d4 0.25f
#define d8 0.125f
		if(number == 3) {
			t[i] 
        += sin_4[i] * this_p[c+3] + 
        sin_223[i] * this_p[c+4];
			t[i + WAVETABLESIZE * 1] +=
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4];
			t[i + WAVETABLESIZE * 2] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4];
			t[i + WAVETABLESIZE * 3] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4];
			t[i + WAVETABLESIZE * 4] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4];
			t[i + WAVETABLESIZE * 5] += 
        sin_4[i] * this_p[c + 3] +
        sin_223[int(i / 2)] * d2 * this_p[c + 4];
			t[i + WAVETABLESIZE * 6] += 
        sin_4[i] * this_p[c+3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4];
			t[i + WAVETABLESIZE * 7] += 
        sin_4[int(i / 2)] * d2 * this_p[c + 3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4];
		}
		else {
			t[i] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
				sin_2[i] * this_p[c + 5] +
				sin_135[i] * this_p[c + 6] + 
        sin_113[i] * this_p[c + 7] +
				sin_1[i] * this_p[c + 8];
			t[i + WAVETABLESIZE] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
				sin_2[i] * this_p[c + 5] +
        sin_135[i] * this_p[c + 6] + 
        sin_113[i] * this_p[c + 7] +
        sin_1[int(i / 2)] * d2 * this_p[c + 8];
			t[i + WAVETABLESIZE * 2] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
        sin_2[i] * this_p[c + 5] +
        sin_135[i] * this_p[c + 6] + 
        sin_113[int(i / 2)] * d2 * this_p[c + 7] +
        sin_1[int(i / 2)] * d2 * this_p[c + 8];
			t[i + WAVETABLESIZE * 3] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
        sin_2[i] * this_p[c + 5] +
        sin_135[int(i / 2)] * d2 * this_p[c + 6] + 
        sin_113[int(i / 2)] * d2 * this_p[c + 7] +
        sin_1[int(i / 2)] * d2 * this_p[c + 8];
			t[i + WAVETABLESIZE * 4] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[i] * this_p[c + 4] +
        sin_2[int(i / 2)] * d2 * this_p[c + 5] +
        sin_135[int(i / 2)] * d2 * this_p[c + 6] + 
        sin_113[int(i / 2)] * d2 * this_p[c + 7] +
        sin_1[int(i / 4)] * d4 * this_p[c + 8];
			t[i + WAVETABLESIZE * 5] += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4] +
        sin_2[int(i / 2)] * d2 * this_p[c + 5] +
        sin_135[int(i / 2)] * d2 * this_p[c + 6] + 
        sin_113[int(i / 4)] * d4 * this_p[c + 7] +
        sin_1[int(i / 4)] * d4 * this_p[c + 8];
			t[i + WAVETABLESIZE * 6]  += 
        sin_4[i] * this_p[c + 3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4] +
        sin_2[int(i / 2)] * d2 * this_p[c + 5] +
        sin_135[int(i / 4)] * 0 * this_p[c + 6] + 
        sin_113[int(i / 4)] * d4 * this_p[c + 7] +
        sin_1[int(i / 4)] * d4 * this_p[c + 8];
			t[i + WAVETABLESIZE * 7] += 
        sin_4[int(i / 2)] * d2 * this_p[c + 3] + 
        sin_223[int(i / 2)] * d2 * this_p[c + 4] +
        sin_2[int(i / 4)] * d4 * this_p[c + 5] +
        sin_135[int(i / 4)] * 0 * this_p[c + 6] + 
        sin_113[int(i / 4)] * d4 * this_p[c + 7] +
        sin_1[int(i / 8)] * d8 * this_p[c + 8];
		} 
	}
  /*
    The grown up source code viewer will find that sin_135 is only
    folded once (/2). Well, I had terrible aliasing problems when
    folding it twice (/4), and the easiest solution was to set it to
    zero instead. You can't claim you actually heard it, can you?
  */
	wavetable[WAVETABLESIZE * 12] = 0;
}


void AZR3::calc_click() {
  /*
    Click is not just click - it has to follow the underlying
    note pitch. However, the click emulation is just "try and
    error". Improve it if you can, but PLEAZE tell me how you
    did it...
  */
	click[0] = my_p[n_click] *
    (my_p[n_1_db1] + my_p[n_1_db2] + my_p[n_1_db3] + my_p[n_1_db4] +
     my_p[n_1_db5] + my_p[n_1_db6] + my_p[n_1_db7] + my_p[n_1_db8] +
     my_p[n_1_db9]) / 9;

	click[1] = my_p[n_click] *
	(my_p[n_2_db1] + my_p[n_2_db2] + my_p[n_2_db3] + my_p[n_2_db4] +
   my_p[n_2_db5] + my_p[n_2_db6] + my_p[n_2_db7]+my_p[n_2_db8] +
   my_p[n_2_db9]) / 9;

	click[2] = my_p[n_click] *
	(my_p[n_3_db1] + my_p[n_3_db2] + my_p[n_3_db3] + my_p[n_3_db4] + 
   my_p[n_1_db5]) / 22;
}


unsigned char* AZR3::event_clock(unsigned long offset) {
  /*
  static unsigned char evt[4];
  static int state = 0;
  if (state == 0) {
    ++state;
    evt[0] = 1;
    evt[1] = 65;
    evt[2] = 64;
    evt[3] = 0;
    return evt;
  }
  if (state == 1) {
    ++state;
    evt[0] = 1;
    evt[1] = 80;
    evt[2] = 64;
    evt[3] = 0;
    return evt;
  }
  if (state == 2) {
    ++state;
    evt[0] = 1;
    evt[1] = 84;
    evt[2] = 64;
    evt[3] = 0;
    return evt;
  }
  */
  
  LV2_MIDI* midi = static_cast<LV2_MIDI*>(m_ports[29]);
  
  // Are there any events left in the buffer?
  if (midi_ptr - midi->data >= midi->size)
    return 0;
  
  // Is next event occuring on this frame?
  double& timestamp = *reinterpret_cast<double*>(midi_ptr);
  if (timestamp > offset)
    return 0;
  
  // It is! Return it if it is a channel event!
  midi_ptr += sizeof(double);
  size_t& eventsize = *reinterpret_cast<size_t*>(midi_ptr);
  midi_ptr += sizeof(size_t);
  unsigned char* old_ptr = midi_ptr;
  midi_ptr += eventsize;
  if ((old_ptr[0] & 0xF0) >= 0x80 && (old_ptr[0] & 0xF0) <= 0xE0)
    return old_ptr;
  
  return 0;
}


void initialise() __attribute__((constructor));
void initialise() {
  register_lv2_inst<AZR3>("http://ll-plugins.nongnu.org/lv2/dev/azr3/0.0.0");
}
