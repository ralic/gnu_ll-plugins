/****************************************************************************
    
    control2midi.cpp - A plugin that converts control input to MIDI events
    
    Copyright (C) 2006  Lars Luthman <lars.luthman@gmail.com>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA  02110-1301  USA

****************************************************************************/

#include <iostream>

#include "lv2plugin.hpp"
#include "lv2-miditype.h"


/** This is the class that contains all the code and data for the Sineshaper
    synth plugin. */
class Control2MIDI : public LV2Plugin {
public:
  
  Control2MIDI(unsigned long, const char*, const LV2_Host_Feature**) 
    : LV2Plugin(5),
      m_last_value(0),
      m_last_cc(0) { 
    
  }
  
  void run(unsigned long sample_count) {
    
    float value = *static_cast<float*>(m_ports[0]);
    float min = *static_cast<float*>(m_ports[1]);
    float max = *static_cast<float*>(m_ports[2]);
    float cc = *static_cast<float*>(m_ports[3]);
    LV2_MIDI* midi = static_cast<LV2_MIDI*>(m_ports[4]);
    
    unsigned char ccc = (unsigned char)cc;
    
    if (max - min < 0.001)
      max = min + 0.001;
    if (value < min)
      value = min;
    else if (value > max)
      value = max;

    midi->event_count = 0;
    
    unsigned char cvalue = (unsigned char)((value - min) * 127 / (max - min));
    if ((ccc != m_last_cc || cvalue != m_last_value) &&
        midi->capacity >= sizeof(double) + sizeof(size_t) + 3) {

      *(double*)(midi->data) = 0;
      *(size_t*)(midi->data + sizeof(double)) = 3;
      unsigned char bytes[] = { 0xB0, ccc, cvalue };
      memcpy(midi->data + sizeof(double) + sizeof(size_t), bytes, 3);
      midi->event_count = 1;
      
      m_last_cc = ccc;
      m_last_value = cvalue;
    }
    
    midi->size = sizeof(double) + sizeof(size_t) + 3;
    
  }
  
protected:
  
  unsigned char m_last_value;
  unsigned char m_last_cc;
  
};


void initialise() __attribute__((constructor));
void initialise() {
  register_lv2<Control2MIDI>("http://ll-plugins.nongnu.org/lv2/dev/control2midi/0.0.0");
}
