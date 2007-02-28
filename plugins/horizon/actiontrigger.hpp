#ifndef ACTIONTRIGGER_HPP
#define ACTIONTRIGGER_HPP

#include <vector>

#include "lv2-midiport.h"


class Action;
class Mixer;


class ActionTrigger {
public:
  
  ActionTrigger(Mixer& mixer);
  
  void run(const LV2_MIDI* midi_port, uint32_t nframes);
  
protected:
  
  struct ActionInfo {
    ActionInfo() : action(0), running(false) { }
    Action* action;
    bool running;
  };
  
  std::vector<ActionInfo> m_key_table;
  std::vector<Action*> m_actions;
  Mixer& m_mixer;
};


#endif