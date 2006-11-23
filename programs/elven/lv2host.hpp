#ifndef LV2HOST_HPP
#define LV2HOST_HPP

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <pthread.h>
#include <dlfcn.h>

#include "lv2-instrument.h"
#include "ringbuffer.hpp"
#include "eventqueue.hpp"


enum PortDirection {
  InputPort,
  OutputPort
};


enum PortRate {
  ControlRate,
  AudioRate
};


/** A struct that holds information about a port in a LV2 plugin. */
struct LV2Port {
  void* buffer;
  std::string symbol;
  PortDirection direction;
  PortRate rate;
  float default_value;
  float min_value;
  float max_value;
  bool midi;
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
  
  bool is_valid() const;
  
  const std::vector<LV2Port>& get_ports() const;
  std::vector<LV2Port>& get_ports();
  long get_default_midi_port() const;
  
  void activate();
  void run(unsigned long nframes);
  void deactivate();
  
  char* configure(const char* key, const char* value);
  void select_program(unsigned long program);
  
  const std::vector<int>& get_midi_map() const;
  
  const std::string& get_gui_path() const;
  const std::string& get_bundle_dir() const;
  
  void queue_program(unsigned long program, bool to_jack = true);
  void queue_control(unsigned long port, float value, bool to_jack = true);
  void queue_midi(uint32_t port, uint32_t size, const unsigned char* midi);
  void queue_config_request(EventQueue* sender);
  void queue_passthrough(const char* msg, void* ptr);
  void set_program(uint32_t program);
  
  void set_event_queue(EventQueue* q);
  
  const std::map<std::string, std::string>& get_config() const;
  
  const std::map<uint32_t, LV2Preset>& get_presets() const;
  
protected:
  
  template <typename T, typename S> T nasty_cast(S ptr) {
    union {
      S s;
      T t;
    } u;
    u.s = ptr;
    return u.t;
  }
  
  template <typename T> T get_symbol(const std::string& name) {
    return nasty_cast<T>(dlsym(m_libhandle, name.c_str()));
  }
  
  template <typename R, typename A> R call_symbol(const std::string& name, A a) {
    typedef R (*FuncType)(A);
    FuncType func = get_symbol<FuncType>(name);
    if (!func) {
      std::cerr<<"Could not find symbol "<<name<<std::endl;
      return R();
    }
    return func(a);
  }
  
  static LV2Host* m_current_object;
  
  void* m_libhandle;
  LV2_Handle m_handle;
  const LV2_Descriptor* m_desc;
  const LV2_InstrumentDescriptor* m_inst_desc;
  
  std::vector<LV2Port> m_ports;
  std::vector<int> m_midimap;
  long m_default_midi_port;
  std::string m_standalonegui;
  std::string m_bundledir;
  
  std::map<std::string, std::string> m_configuration;
  unsigned long m_program;
  bool m_program_is_valid;
  bool m_new_program;
  
  // big lock
  pthread_mutex_t m_mutex;
  
  EventQueue m_to_jack;
  EventQueue* m_from_jack;
  
  std::map<uint32_t, LV2Preset> m_presets;
};


#endif