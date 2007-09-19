/****************************************************************************
    
    lv2host.hpp - Simple LV2 plugin loader for Elven
    
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

#ifndef LV2HOST_HPP
#define LV2HOST_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <pthread.h>
#include <dlfcn.h>

#include <sigc++/slot.h>
#include <sigc++/signal.h>

#include "lv2.h"
#include "lv2-command.h"
#include "ringbuffer.hpp"


enum PortDirection {
  InputPort,
  OutputPort,
  NoDirection
};


enum PortType {
  AudioType,
  ControlType,
  MidiType,
  NoType
};


/** A struct that holds information about a port in a LV2 plugin. */
struct LV2Port {
  void* buffer;
  std::string symbol;
  PortDirection direction;
  PortType type;
  float default_value;
  float min_value;
  float max_value;
  float value;
};


struct LV2Preset {
  std::string name;
  std::map<uint32_t, float> values;
};


/** A class that loads a single LV2 plugin. */
class LV2Host {
public:
  
  LV2Host(const std::string& uri, unsigned long frame_rate);
  ~LV2Host();
  
  /** Returns true if the plugin was loaded OK. */
  bool is_valid() const;
  
  /** Returns a vector of objects containing information about the plugin's 
      ports. */
  const std::vector<LV2Port>& get_ports() const;

  /** Returns a vector of objects containing information about the plugin's 
      ports. */
  std::vector<LV2Port>& get_ports();
  
  /** Returns the index of the default MIDI port, or -1 if there is no default
      MIDI port. */
  long get_default_midi_port() const;
  
  /** Return the MIDI controller mappings. */
  const std::vector<int>& get_midi_map() const;
  
  /** Return the path to the SVG icon file. */
  const std::string& get_icon_path() const;
  
  /** Return the path to the GUI plugin module. */
  const std::string& get_gui_path() const;
  
  /** Return the URI for the GUI plugin. */
  const std::string& get_gui_uri() const;
  
  /** Return the path to the plugin bundle. */
  const std::string& get_bundle_dir() const;
  
  /** Return the name of the plugin. */
  const std::string& get_name() const;
  
  /** Returns all found presets. */
  const std::map<unsigned char, LV2Preset>& get_presets() const;
  
  /** Activate the plugin. The plugin must be activated before you call the
      run() function. */
  void activate();
  
  /** Tell the plugin to produce @c nframes frames of signal. */
  void run(unsigned long nframes);
  
  /** Deactivate the plugin. */
  void deactivate();
  
  /** Send a command to the plugin. */
  char* command(uint32_t argc, const char* const* argv);
  
  /** Write to a plugin port. */
  void write_port(uint32_t index, uint32_t buffer_size, const void* buffer);
  
  /** Set a control port value. */
  void set_control(uint32_t index, float value);
  
  /** Set the plugin program. */
  void set_program(uint32_t program);
  
  /** Queue a MIDI event. */
  void queue_midi(uint32_t port, uint32_t size, const unsigned char* midi);
  
  /** List all available plugins. */
  static void list_plugins();
  
  sigc::signal<void, uint32_t, uint32_t, const void*> signal_port_event;
  
  sigc::signal<void, uint32_t, const char* const*> signal_feedback;
  
protected:
  
  static std::vector<std::string> get_search_dirs();
  
  typedef sigc::slot<bool, const std::string&> scan_callback_t;
  
  struct MidiEvent {
    MidiEvent(uint32_t p, uint32_t s, const unsigned char* d) 
      : port(p), event_size(s), data(0), written(false) {
      data = new unsigned char[event_size];
      std::memcpy(data, d, event_size);
    }
    uint32_t port;
    uint32_t event_size;
    unsigned char* data;
    bool written;
  };
  
  static bool scan_manifests(const std::vector<std::string>& search_dirs, 
                             scan_callback_t callback);
                      
  bool match_uri(const std::string& bundle);

  bool match_partial_uri(const std::string& bundle);
  
  static bool print_uri(const std::string& bundle);
  
  bool load_plugin();
  
  void feedback(uint32_t argc, const char* const* argv);
  
  static void feedback_wrapper(void* me, uint32_t argc, 
			       const char* const* argv);
  
  template <typename T> T get_symbol(const std::string& name) {
    union {
      void* s;
      T t;
    } u;
    u.s = dlsym(m_libhandle, name.c_str());
    return u.t;
  }

  
  std::string m_uri;
  std::string m_bundle;
  std::vector<std::string> m_rdffiles;
  std::string m_binary;
  uint32_t m_rate;
  
  void* m_libhandle;
  LV2_Handle m_handle;
  const LV2_Descriptor* m_desc;
  const LV2_CommandDescriptor* m_comm_desc;
  
  LV2_CommandHostDescriptor m_comm_host_desc;
  
  std::vector<LV2Port> m_ports;
  bool m_ports_updated;
  std::vector<int> m_midimap;
  long m_default_midi_port;
  std::string m_iconpath;
  std::string m_plugingui;
  std::string m_guiuri;
  std::string m_bundledir;
  std::string m_name;
  
  unsigned long m_program;
  bool m_program_is_valid;
  bool m_new_program;
  
  std::vector<MidiEvent*> m_midi_events;
  
  // big lock
  pthread_mutex_t m_mutex;
  
  std::map<unsigned char, LV2Preset> m_presets;
};


#endif
