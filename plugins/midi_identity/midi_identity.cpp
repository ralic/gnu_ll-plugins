/****************************************************************************
    
    midi_identity.cpp - A trivial MIDI THRU plugin
    
    Copyright (C) 2006-2007 Lars Luthman <lars.luthman@gmail.com>
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
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

#include <cstring>

#include "lv2plugin.hpp"
#include "lv2-midifunctions.h"


/** This is the class that contains all the code and data for the MIDI 
    identity plugin. */
class MIDIIdentity : public LV2::Plugin<MIDIIdentity> {
public:
  
  MIDIIdentity(double, const char*, const LV2_Feature* const*) 
    : LV2::Plugin<MIDIIdentity>(2) {
    
  }
  
  
  void run(uint32_t nframes) {
    
    LV2_MIDIState in = { static_cast<LV2_MIDI*>(m_ports[0]), nframes, 0 };
    LV2_MIDIState out = { static_cast<LV2_MIDI*>(m_ports[1]), nframes, 0 };
    
    out.midi->size = 0;
    out.midi->event_count = 0;
    
    double event_time;
    uint32_t event_size;
    unsigned char* event;

    while (lv2midi_get_event(&in, &event_time, &event_size, &event) < nframes){
      lv2midi_put_event(&out, event_time, event_size, event);
      lv2midi_step(&in);
    }
    
  }
  
};


static unsigned _ = MIDIIdentity::register_class("http://ll-plugins.nongnu.org/lv2/dev/midi_identity/0.0.0");
