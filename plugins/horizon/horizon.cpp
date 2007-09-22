/****************************************************************************

    horizon.cpp

    Copyright (C) 2007 Lars Luthman <lars.luthman@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <semaphore.h>

#include "action.hpp"
#include "actiontrigger.hpp"
#include "effect.hpp"
#include "horizon.peg"
#include "lv2advanced.hpp"
#include "mixer.hpp"
#include "sample.hpp"


using namespace std;


class Horizon : public LV2::Advanced {
public:
  
  
  Horizon(double rate, const char* bundle_path, 
	  const LV2_Host_Feature* const* f)
    : LV2::Advanced(h_n_ports),
      m_trigger(m_mixer) {
    
    sem_init(&m_lock, 0, 1);
  }
  
  
  ~Horizon() {
    sem_destroy(&m_lock);
    for (unsigned i = 0; i < m_samples.size(); ++i)
      delete m_samples[i];
  }
  
  
  void run(uint32_t nframes) {
    if (!sem_trywait(&m_lock)) {
      m_mixer.set_buffers(p(h_left), p(h_right));
      m_trigger.run(p<LV2_MIDI>(h_midi_input), nframes);
      sem_post(&m_lock);
    }
    else {
      memset(p(h_left), 0, sizeof(float) * nframes);
      memset(p(h_right), 0, sizeof(float) * nframes);
    }
  }


  char* command(uint32_t argc, const char*const* argv) {
    
    cerr<<"Command with "<<argc<<" parameters:"<<endl;
    for (unsigned i = 0; i < argc; ++i)
      cerr<<"  "<<argv[i]<<endl;
    
    // load a new sample
    if (argc == 2 && !strcmp(argv[0], "load_sample")) {
      if (load_sample(argv[1])) {
	cerr<<"Loaded sample from "<<argv[1]<<endl;
	return 0;
      }
      else
	return strdup((string("Failed to load ") + argv[1]).c_str());
    }
    
    // delete a loaded sample
    else if (argc == 2 && !strcmp(argv[0], "delete_sample")) {
      if (delete_sample(argv[1])) {
	cerr<<"Deleted sample "<<argv[1]<<endl;
	return 0;
      }
      else
	return strdup((string("Failed to delete ") + argv[1]).c_str());
    }
    
    // rename a loaded sample
    else if (argc == 3 && !strcmp(argv[0], "rename_sample")) {
      if (rename_sample(argv[1], argv[2])) {
	cerr<<"Renamed sample "<<argv[1]<<" to "<<argv[2]<<endl;
	return 0;
      }
      else
	return strdup((string("Failed to rename ") + argv[1]).c_str());
    }
    
    // add a splitpoint
    else if (argc == 3 && !strcmp(argv[0], "add_splitpoint")) {
      if (add_splitpoint(argv[1], atol(argv[2]))) {
	cerr<<"Added splitpoint "<<argv[2]<<" in sample "<<argv[1]<<endl;
	return 0;
      }
      else
	return strdup((string("Failed to add splitpoint ") + argv[2] + 
		       " in sample " + argv[1]).c_str());
    }
    
    // remove a splitpoint
    else if (argc == 3 && !strcmp(argv[0], "remove_splitpoint")) {
      if (remove_splitpoint(argv[1], atol(argv[2]))) {
	cerr<<"Removed splitpoint "<<argv[2]<<" in sample "<<argv[1]<<endl;
	return 0;
      }
      else
	return strdup((string("Failed to remove splitpoint ") + argv[2] + 
		       " in sample " + argv[1]).c_str());
    }

    // move a splitpoint
    else if (argc == 4 && !strcmp(argv[0], "move_splitpoint")) {
      if (move_splitpoint(argv[1], atol(argv[2]), atol(argv[3]))) {
	cerr<<"Moved splitpoint "<<argv[2]<<" to "<<argv[3]
	    <<"in sample "<<argv[1]<<endl;
	return 0;
      }
      else
	return strdup((string("Failed to move splitpoint ") + argv[2] + " to " +
		       argv[3] + " in sample " + argv[1]).c_str());
    }
    
    // play a preview of a sample region
    else if (argc == 4 && !strcmp(argv[0], "play_preview")) {
      if (play_preview(argv[1], atol(argv[2]), atol(argv[3])))
	return 0;
      else
	return strdup((string("Failed to play preview ") + argv[1] + 
		       " [" + argv[2] + ", " + argv[3] + ")").c_str());
    }
    
    // stop a preview of a sample region
    else if (argc == 2 && !strcmp(argv[0], "stop_preview")) {
      if (stop_preview(argv[1]))
	return 0;
      else
	return strdup("Failed to stop preview ");
    }
    
    // add a static effect to a sample
    else if (argc == 4 && !strcmp(argv[0], "add_static_effect")) {
      if (add_static_effect(argv[1], atol(argv[2]), argv[3])) {
	cerr<<"Added static effect "<<argv[3]<<" at position "<<argv[2]
	    <<" in the stack for "<<argv[1]<<endl;
	return 0;
      }
      else
	return strdup((string("Failed to add static effect ") + argv[3] +
		       " at position " + argv[2] + " in the stack for " +
		       argv[1]).c_str());
    }
    
    // remove a static effect from a sample
    else if (argc == 3 && !strcmp(argv[0], "remove_static_effect")) {
      if (remove_static_effect(argv[1], atol(argv[2]))) {
	cerr<<"Removed static effect "<<argv[2]<<" in stack for "
	    <<argv[1]<<endl;
	return 0;
      }
      else
	return strdup((string("Failed to remove static effect ") + argv[2] +
		       " from stack for " + argv[1]).c_str());
    }
    
    // bypass a static effect
    else if (argc == 4 && !strcmp(argv[0], "bypass_static_effect")) {
      if (bypass_static_effect(argv[1], atol(argv[2]), atoi(argv[3]) != 0)) {
	cerr<<(atoi(argv[3]) ? 
	       (string("Disabled static effect ") + argv[2] + " in stack for " +
		argv[1]).c_str() :
	       (string("Enabled static effect ") + argv[2] + "in stack for " +
		argv[1]).c_str())<<endl;
	return 0;
      }
      else
	return strdup((string("Failed to ") + 
		       (atoi(argv[3]) ? "disable" : "enable") + 
		       " static effect " + argv[2] + " in the stack for " +
		       argv[1]).c_str());
    }
    
    return strdup("Unknown command!");
  }
  
protected:
  
  bool load_sample(const std::string& filename) {
    
    Sample* sample = new Sample(filename);
    if (!sample->is_valid()) {
      delete sample;
      return false;
    }
    
    m_samples.push_back(sample);
    int n = m_samples.size() - 1;
    
    const SampleBuffer& buf = sample->get_processed_buffer();
    if (buf.get_channels() == 1)
      feedback("ssifs", "sample_loaded", sample->get_name().c_str(), 
		long(buf.get_length()), buf.get_rate(), 
		buf.get_shm_name(0).c_str());
    else if (buf.get_channels() == 2)
      feedback("ssifss", "sample_loaded", sample->get_name().c_str(), 
		long(buf.get_length()), buf.get_rate(), 
		buf.get_shm_name(0).c_str(), buf.get_shm_name(1).c_str());
    
    Action* a1 = new Action(*m_samples[n]->get_chunks()[0]);
    Action* a2 = new Action(*m_samples[n]->get_chunks()[1]);
    Action* a3 = new Action(*m_samples[n]->get_chunks()[2]);
    Action* a4 = new Action(*m_samples[n]->get_chunks()[3]);
    m_trigger.add_action(a1);
    m_trigger.map_action(a1, 60 + n * 4);
    m_trigger.add_action(a2);
    m_trigger.map_action(a2, 61 + n * 4);
    m_trigger.add_action(a3);
    m_trigger.map_action(a3, 62 + n * 4);
    m_trigger.add_action(a4);
    m_trigger.map_action(a4, 63 + n * 4);
    
    return true;
  }
  

  bool delete_sample(const std::string& name) {
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == name) {
	sem_wait(&m_lock);
	m_mixer.stop();
	const std::vector<Chunk*>& chunks = m_samples[i]->get_chunks();
	for (unsigned j = 0; j < chunks.size(); ++j)
	  m_trigger.remove_actions_for_chunk(chunks[j]);
	sem_post(&m_lock);
	delete m_samples[i];
	m_samples.erase(m_samples.begin() + i);
	feedback("ss", "sample_deleted", name.c_str());
	return true;
      }
    }
    return false;
  }
  
  
  bool rename_sample(const std::string& old_name, const std::string& new_name) {
    
    // check that the new name isn't used already
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == new_name)
	return false;
    }
    
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == old_name) {
	m_samples[i]->set_name(new_name);
	feedback("sss", "sample_renamed", old_name.c_str(), new_name.c_str());
	return true;
      }
    }
    
    return false;
  }
  
  
  bool add_splitpoint(const std::string& name, size_t frame) {
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == name) {
	bool success = false;
	sem_wait(&m_lock);
	m_mixer.stop();
	if (m_samples[i]->add_splitpoint(frame))
	  success = true;
	sem_post(&m_lock);
	if (success)
	  feedback("ssi", "splitpoint_added", name.c_str(), frame);
	return success;
      }
    }
    return false;
  }
  
  
  bool remove_splitpoint(const std::string& name, size_t frame) {
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == name) {
	bool success = false;
	sem_wait(&m_lock);
	m_mixer.stop();
	if (m_samples[i]->remove_splitpoint(frame))
	  success = true;
	sem_post(&m_lock);
	if (success)
	  feedback("ssi", "splitpoint_removed", name.c_str(), frame);
	return success;
      }
    }
    return false;
  }
  
  
  bool move_splitpoint(const std::string& name, size_t frame, size_t newframe) {
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == name) {
	bool success = false;
	sem_wait(&m_lock);
	m_mixer.stop();
	if (m_samples[i]->move_splitpoint(frame, newframe))
	  success = true;
	sem_post(&m_lock);
	if (success)
	  feedback("ssii", "splitpoint_moved", name.c_str(), frame, newframe);
	return success;
      }
    }
    return false;
  }

  
  bool play_preview(const std::string& name, size_t start, size_t end) {
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == name) {
	sem_wait(&m_lock);
	m_mixer.play_preview(m_samples[i]->get_processed_buffer(), start, end);
	sem_post(&m_lock);
	return true;
      }
    }
    return false;
  }
  
  
  bool stop_preview(const std::string& name) {
    sem_wait(&m_lock);
    m_mixer.stop();
    sem_post(&m_lock);
    return true;
  }
  
  
  bool add_static_effect(const std::string& sample, size_t pos,
			 const std::string& effect_uri) {
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == sample) {
	const Effect* e = 0;
	if ((e = m_samples[i]->add_static_effect(pos, effect_uri))) {
	  feedback("ssis", "static_effect_added", 
		    sample.c_str(), pos, e->get_name().c_str());
	  const SampleBuffer& buf = m_samples[i]->get_processed_buffer();
	  if (buf.get_channels() == 1) {
	    feedback("sss", "sample_modified", sample.c_str(), 
		      buf.get_shm_name(0).c_str());
	  }
	  else if (buf.get_channels() == 2) {
	    feedback("ssss", "sample_modified", sample.c_str(),
		      buf.get_shm_name(0).c_str(), buf.get_shm_name(1).c_str());
	  }
	  return true;
	}
	else
	  return false;
      }
    }
    return false;
  }
  
  
  bool remove_static_effect(const std::string& sample, size_t pos) {
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == sample) {
	if (m_samples[i]->remove_static_effect(pos)) {
	  feedback("ssi", "static_effect_removed", sample.c_str(), pos);
	  const SampleBuffer& buf = m_samples[i]->get_processed_buffer();
	  if (buf.get_channels() == 1) {
	    feedback("sss", "sample_modified", sample.c_str(), 
		      buf.get_shm_name(0).c_str());
	  }
	  else if (buf.get_channels() == 2) {
	    feedback("ssss", "sample_modified", sample.c_str(),
		      buf.get_shm_name(0).c_str(), buf.get_shm_name(1).c_str());
	  }
	  return true;
	}
	else
	  return false;
      }
    }
    return false;
  }
  
  
  bool bypass_static_effect(const std::string& sample, size_t pos, bool bpass) {
    for (unsigned i = 0; i < m_samples.size(); ++i) {
      if (m_samples[i]->get_name() == sample) {
	if (m_samples[i]->bypass_static_effect(pos, bpass)) {
	  feedback("ssii", "static_effect_bypassed", sample.c_str(), pos,
		    bpass ? 1 : 0);
	  const SampleBuffer& buf = m_samples[i]->get_processed_buffer();
	  if (buf.get_channels() == 1) {
	    feedback("sss", "sample_modified", sample.c_str(), 
		      buf.get_shm_name(0).c_str());
	  }
	  else if (buf.get_channels() == 2) {
	    feedback("ssss", "sample_modified", sample.c_str(),
		      buf.get_shm_name(0).c_str(), buf.get_shm_name(1).c_str());
	  }
	  return true;
	}
	else
	  return false;
      }
    }
    return false;
  }

  
  Mixer m_mixer;
  ActionTrigger m_trigger;
  vector<Sample*> m_samples;
  
  // XXX this should be more fine-grained
  sem_t m_lock;
  
};


static LV2::RegisterAdvanced<Horizon> reg(h_uri);
