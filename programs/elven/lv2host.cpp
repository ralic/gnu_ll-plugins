#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>

#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <turtleparser.hpp>
#include <query.hpp>
#include <namespaces.hpp>

#include "lv2host.hpp"
#include "lv2-midifunctions.h"


using namespace std;
using namespace PAQ;


namespace {
  
  string absolutise(const string& reluri, const string& baseuri) {
    string result = baseuri + "/" + reluri.substr(1, reluri.length() - 2);
    return result;
  }
  
}


LV2Host::LV2Host(const string& uri, unsigned long frame_rate) 
  : m_handle(0),
    m_inst_desc(0),
    m_midimap(128, -1),
    m_from_jack(0) {
  
  pthread_mutex_init(&m_mutex, 0);
  
  // get the directories to look in
  vector<string> search_dirs;
  const char* lv2p_c = getenv("LV2_PATH");
  if (!lv2p_c) {
    search_dirs.push_back(string(getenv("HOME")) + "/.lv2");
    search_dirs.push_back("/usr/lib/lv2");
    search_dirs.push_back("/usr/local/lib/lv2");
  }
  else {
    string lv2_path = lv2p_c;
    int split;
    while ((split = lv2_path.find(':')) != string::npos) {
      search_dirs.push_back(lv2_path.substr(0, split));
      lv2_path = lv2_path.substr(split + 1);
    }
    search_dirs.push_back(lv2_path);
  }
  
  string uriref = string("<") + uri + ">";
  string library;
  string ttlfile;
  string plugindir;
  bool isInstrument = false;

  vector<QueryResult> qr;
  
  // iterate over all directories
  for (unsigned i = 0; i < search_dirs.size(); ++i) {
    
    // open the directory
    DIR* d = opendir(search_dirs[i].c_str());
    if (d == 0) {
      cerr<<"Could not open "<<search_dirs[i]<<": "<<strerror(errno)<<endl;
      continue;
    }
    
    // iterate over all directory entries
    dirent* e;
    while (e = readdir(d)) {
      if (strlen(e->d_name) >= 4) {
        
        // is it named like an LV2 bundle?
        if (strcmp(e->d_name + (strlen(e->d_name) - 4), ".lv2"))
          continue;
        
        // is it actually a directory?
        plugindir = search_dirs[i] + "/" + e->d_name;
        struct stat stat_info;
        if (stat(plugindir.c_str(), &stat_info))
          continue;
        if (!S_ISDIR(stat_info.st_mode))
          continue;
        
        // parse the manifest to get the library filename and the data filename
        ttlfile = plugindir + "/manifest.ttl";
        {
          
          ifstream ifs((plugindir + "/manifest.ttl").c_str());
          string text, line;
          while (getline(ifs, line))
            text += line + "\n";
          TurtleParser tp;
          RDFData data;
          if (!tp.parse_ttl(text, data)) {
            cerr<<"Could not parse "<<plugindir<<"/manifest.ttl"<<endl;
            continue;
          }
          Variable binary, datafile;
          Namespace lv2("<http://lv2plug.in/ontology#>");
          qr = select(binary)
            .where(uriref, lv2("binary"), binary)
            .run(data);
          if (qr.size() == 0)
            continue;
          else
            library = absolutise(qr[0][binary]->name, plugindir);
          qr = select(datafile)
            .where(uriref, rdfs("seeAlso"), datafile)
            .run(data);
          if (qr.size() > 0)
            ttlfile = absolutise(qr[0][datafile]->name, plugindir);
          break;
        }
      }
    }
    
    closedir(d);
    if (library != "")
      break;
  }
  
  if (library == "") {
    cerr<<"Could not find plugin "<<uri<<endl;
    return;
  }
  
  // parse the datafile to get port info
  {
    
    ifstream ifs(ttlfile.c_str());
    string text, line;
    while (getline(ifs, line))
      text += line + "\n";
    TurtleParser tp;
    RDFData data;
    if (!tp.parse_ttl(text, data)) {
      cerr<<"Could not parse "<<ttlfile<<endl;
      return;
    }
    Variable symbol, index, portclass, porttype, port;
    Namespace lv2("<http://lv2plug.in/ontology#>");
    Namespace ll("<http://ll-plugins.nongnu.org/lv2/namespace#>");
    Namespace llext("<http://ll-plugins.nongnu.org/lv2/ext/>");
    
    qr = select(index, symbol, portclass, porttype)
      .where(uriref, lv2("port"), port)
      .where(port, rdf("type"), portclass)
      .where(port, lv2("index"), index)
      .where(port, lv2("symbol"), symbol)
      .where(port, lv2("datatype"), porttype)
      .run(data);
    m_ports.resize(qr.size());
    for (unsigned j = 0; j < qr.size(); ++j) {
      
      // index
      size_t p = atoi(qr[j][index]->name.c_str());
      if (p >= m_ports.size())
        return;
      
      // symbol
      m_ports[p].symbol = qr[j][symbol]->name;
      
      // direction and rate
      string pclass = qr[j][portclass]->name;
      if (pclass == lv2("ControlRateInputPort")) {
        m_ports[p].direction = InputPort;
        m_ports[p].rate = ControlRate;
      }
      else if (pclass == lv2("ControlRateOutputPort")) {
        m_ports[p].direction = OutputPort;
        m_ports[p].rate = ControlRate;
      }
      else if (pclass == lv2("AudioRateInputPort")) {
        m_ports[p].direction = InputPort;
        m_ports[p].rate = AudioRate;
      }
      else if (pclass == lv2("AudioRateOutputPort")) {
        m_ports[p].direction = OutputPort;
        m_ports[p].rate = AudioRate;
      }
      else {
        cerr<<"Unknown port class: "<<pclass<<endl;
        return;
      }
      
      // type
      string type = qr[j][porttype]->name;
      if (type == lv2("float"))
        m_ports[p].midi = false;
      else if (type == llext("miditype"))
        m_ports[p].midi = true;
      else {
        cerr<<"Unknown datatype: "<<type<<endl;
        return;
      }
      
      m_ports[p].default_value = 0;
      m_ports[p].min_value = 0;
      m_ports[p].max_value = 1;
    }
    
    // default values
    Variable default_value;
    qr = select(index, default_value)
      .where(uriref, lv2("port"), port)
      .where(port, lv2("index"), index)
      .where(port, lv2("default"), default_value)
      .run(data);
    for (unsigned j = 0; j < qr.size(); ++j) {
      unsigned p = atoi(qr[j][index]->name.c_str());
      if (p >= m_ports.size())
        return;
      m_ports[p].default_value = 
        atof(qr[j][default_value]->name.c_str());
    }
    
    // minimum values
    Variable min_value;
    qr = select(index, min_value)
      .where(uriref, lv2("port"), port)
      .where(port, lv2("index"), index)
      .where(port, lv2("minimum"), min_value)
      .run(data);
    for (unsigned j = 0; j < qr.size(); ++j) {
      unsigned p = atoi(qr[j][index]->name.c_str());
      if (p >= m_ports.size())
        return;
      m_ports[p].min_value = 
        atof(qr[j][min_value]->name.c_str());
    }
    
    // maximum values
    Variable max_value;
    qr = select(index, max_value)
      .where(uriref, lv2("port"), port)
      .where(port, lv2("index"), index)
      .where(port, lv2("maximum"), max_value)
      .run(data);
    for (unsigned j = 0; j < qr.size(); ++j) {
      unsigned p = atoi(qr[j][index]->name.c_str());
      if (p >= m_ports.size())
        return;
      m_ports[p].max_value = 
        atof(qr[j][max_value]->name.c_str());
    }
    
    // check range sanity
    for (unsigned long i = 0; i < m_ports.size(); ++i) {
      if (m_ports[i].direction == OutputPort)
        continue;
      if (m_ports[i].min_value > m_ports[i].default_value)
        m_ports[i].min_value = m_ports[i].default_value;
      if (m_ports[i].max_value < m_ports[i].default_value)
        m_ports[i].max_value = m_ports[i].default_value;
      if (m_ports[i].max_value - m_ports[i].min_value <= 0)
        m_ports[i].max_value = m_ports[i].min_value + 10;
    }
    
    // MIDI controller bindings
    Variable controller, cc_number;
    qr = select(index, cc_number)
      .where(uriref, lv2("port"), port)
      .where(port, lv2("index"), index)
      .where(port, ll("defaultMidiController"), controller)
      .where(controller, ll("controllerType"), ll("CC"))
      .where(controller, ll("controllerNumber"), cc_number)
      .run(data);
    for (unsigned j = 0; j < qr.size(); ++j) {
      unsigned p = atoi(qr[j][index]->name.c_str());
      unsigned cc = atoi(qr[j][cc_number]->name.c_str());
      if (p < m_ports.size() && cc < 128)
        m_midimap[cc] = p;
    }
    
    // default MIDI port
    qr = select(index)
      .where(uriref, ll("defaultMidiPort"), index)
      .run(data);
    if (qr.size() > 0)
      m_default_midi_port = atoi(qr[0][index]->name.c_str());
    else {
      m_default_midi_port = -1;
      for (unsigned long i = 0; i < m_ports.size(); ++i) {
        if (m_ports[i].midi && m_ports[i].direction == InputPort && 
            m_ports[i].rate == ControlRate) {
          m_default_midi_port = i;
          break;
        }
      }
    }
    
    // is the instrument extension used?
    Variable extension_predicate;
    qr = select(extension_predicate)
      .where(uriref, extension_predicate, ll("instrument-ext"))
      .run(data);
    if (qr.size() > 0 && (qr[0][extension_predicate]->name == lv2("requiredHostFeature") ||
                          qr[0][extension_predicate]->name == lv2("optionalHostFeature")))
      isInstrument = true;
    
    // standalone GUI path
    Variable gui_path;
    qr = select(gui_path)
      .where(uriref, ll("standaloneGui"), gui_path)
      .run(data);
    if (qr.size() > 0)
      m_standalonegui = absolutise(qr[0][gui_path]->name, plugindir);
    
    m_bundledir = plugindir;
    
    // default preset path
    Variable preset_path;
    qr = select(preset_path)
      .where(uriref, ll("defaultPresets"), preset_path)
      .run(data);
    if (qr.size() > 0) {
      string presetfile = absolutise(qr[0][preset_path]->name, plugindir);
      cerr<<"Loading presets from "<<presetfile<<endl;
      ifstream ifs(presetfile.c_str());
      string text, line;
      while (getline(ifs, line))
        text += line + "\n";
      TurtleParser tp;
      RDFData data;
      if (!tp.parse_ttl(text, data)) {
        cerr<<"Could not parse presets from "<<presetfile<<endl;
      }
      else {
        Variable bank;
        qr = select(bank).where(uriref, ll("hasBank"), bank).run(data);
        if (qr.size() > 0) {
          string bankuri = qr[0][bank]->name;
          cerr<<"Found preset bank "<<bankuri<<endl;
          Variable preset, name, program;
          vector<QueryResult> qr2 = select(preset, name, program)
            .where(uriref, ll("hasSetting"), preset)
            .where(preset, doap("name"), name)
            .where(preset, ll("midiProgram"), program)
            .where(preset, ll("inBank"), bankuri)
            .run(data);
          for (int j = 0; j < qr2.size(); ++j) {
            string preseturi = qr2[j][preset]->name;
            cerr<<"Found the preset \""<<qr2[j][name]->name<<"\" "
                <<" with MIDI program number "<<qr2[j][program]->name<<endl;
            int pnum = atoi(qr2[j][program]->name.c_str());
            m_presets[pnum].name = qr2[j][name]->name;
            m_presets[pnum].values.clear();
            Variable pv, port, value;
            vector<QueryResult> qr3 = select(port, value)
              .where(preseturi, ll("hasPortValue"), pv)
              .where(pv, ll("forPort"), port)
              .where(pv, rdf("value"), value)
              .run(data);
            for (int k = 0; k < qr3.size(); ++k) {
              int p = atoi(qr3[k][port]->name.c_str());
              float v = atof(qr3[k][value]->name.c_str());
              //cerr<<p<<": "<<v<<endl;
              m_presets[pnum].values[p] = v;
            }
          }
        }
      }
    }

  }
  
  // if we got this far the data is OK. time to load the library
  m_libhandle = dlopen(library.c_str(), RTLD_NOW);
  if (!m_libhandle) {
    cerr<<"Could not dlopen "<<library<<": "<<dlerror()<<endl;
    return;
  }
  
  // get the descriptor
  LV2_Descriptor_Function dfunc = get_symbol<LV2_Descriptor_Function>("lv2_descriptor");
  if (!dfunc) {
    cerr<<library<<" has no LV2 descriptor function"<<endl;
    dlclose(m_libhandle);
    return;
  }
  for (unsigned long j = 0; m_desc = dfunc(j); ++j) {
    if (uri == m_desc->URI)
      break;
  }
  if (!m_desc) {
    cerr<<library<<" does not contain the plugin "<<uri<<endl;
    dlclose(m_libhandle);
    return;
  }
  
  // get the instrument descriptor (if it is an instrument)
  if (isInstrument) {
    m_inst_desc = 0;
    if (m_desc->extension_data)
      m_inst_desc = (LV2_InstrumentDescriptor*)(m_desc->extension_data("<http://ll-plugins.nongnu.org/lv2/namespace#instrument-ext>"));
    if (!m_inst_desc) {
      cerr<<library<<" does not contain the instrument "<<uri<<endl;
      return;
    }
  }
  
  // instantiate the plugin
  LV2_Host_Feature instrument_feature = {
    "<http://ll-plugins.nongnu.org/lv2/namespace#instrument-ext>",
    0
  };
  const LV2_Host_Feature* features[2];
  memset(features, 0, 2 * sizeof(LV2_Host_Feature*));
  if (isInstrument)
    features[0] = &instrument_feature;
  m_handle = m_desc->instantiate(m_desc, frame_rate, plugindir.c_str(), features);
  
  // list available programs
  /*
  bool default_program_set = false;
  unsigned long default_program;
  const LV2_ProgramDescriptor* pdesc;
  for (unsigned long index = 0; (pdesc = get_program(index)); ++index) {
    cerr<<"Program "<<pdesc->number<<": "<<pdesc->name<<endl;
    if (!default_program_set) {
      cerr<<"Queuing as default."<<endl;
      queue_program(pdesc->number);
      default_program_set = true;
    }
  }
  */
  
  if (!m_handle) {
    cerr<<"Could not instantiate plugin "<<uri<<endl;
    dlclose(m_libhandle);
    return;
  }
}
  
  
LV2Host::~LV2Host() {
  if (m_handle) {
    if (m_desc->cleanup)
      m_desc->cleanup(m_handle);
    dlclose(m_libhandle);
  }
}


bool LV2Host::is_valid() const {
  return (m_handle != 0);
}


const vector<LV2Port>& LV2Host::get_ports() const {
  assert(m_handle != 0);
  return m_ports;
}


vector<LV2Port>& LV2Host::get_ports() {
  assert(m_handle);
  return m_ports;
}


long LV2Host::get_default_midi_port() const {
  return m_default_midi_port;
}


void LV2Host::activate() {
  assert(m_handle);
  if (m_desc->activate)
    m_desc->activate(m_handle);
}


void LV2Host::run(unsigned long nframes) {
  assert(m_handle);
  for (size_t i = 0; i < m_ports.size(); ++i)
    m_desc->connect_port(m_handle, i, m_ports[i].buffer);
  
  // read commands from ringbuffer
  EventQueue::Type t;
  const EventQueue::Event& e = m_to_jack.get_event();
  while ((t = m_to_jack.read_event()) != EventQueue::None) {
    switch (t) {
      
    case EventQueue::Program: 
      select_program(e.program.program);
      break;
      
    case EventQueue::Control: {
      const unsigned long& port = e.control.port;
      const float& value = e.control.value;
      *static_cast<float*>(m_ports[port].buffer) = value;
      //queue_control(port, value, false);
      break;
    }
      
    case EventQueue::ConfigRequest:
      if (m_from_jack) {
        //if (m_program_is_valid)
        //  e.config.sender->write_program(m_program);
        for (unsigned long p = 0; p < m_ports.size(); ++p) {
          if (!m_ports[p].midi && m_ports[p].rate == ControlRate &&
              m_ports[p].direction == InputPort) {
            e.config.sender->
              write_control(p, *static_cast<float*>(m_ports[p].buffer));
          }
        }
        e.config.sender->write_passthrough("cend");
      }
      break;
      
    case EventQueue::Passthrough:
      if (m_from_jack)
        m_from_jack->write_passthrough(e.passthrough.msg, e.passthrough.ptr);
      break;
      
    case EventQueue::Midi: {
      LV2_MIDI* midi = static_cast<LV2_MIDI*>(m_ports[e.midi.port].buffer);
      LV2_MIDIState state = { midi, nframes, 0 };
      // XXX DANGER! Can't set the timestamp to 0 since other events may
      // have been written to the buffer already!
      lv2midi_put_event(&state, 0, e.midi.size, e.midi.data);
      break;
    }

    default:
      break;
    }
  }
  
  m_desc->run(m_handle, nframes);
}


void LV2Host::deactivate() {
  assert(m_handle);
  if (m_desc->deactivate)
    m_desc->deactivate(m_handle);
}


char* LV2Host::configure(const char* key, const char* value) {
  cerr<<"Calling configure(\""<<key<<"\", \""<<value<<"\")"<<endl;
  if (m_inst_desc && m_inst_desc->configure) {
    char* result = m_inst_desc->configure(m_handle, key, value);
    if (!result) {
      m_configuration[key] = value;
      m_program_is_valid;
    }
    else {
      cerr<<"ERROR CONFIGURING PLUGIN: "<<result<<endl;
      free(result);
    }
    
    return result;
  }
  else
    cerr<<"This plugin has no configure() callback"<<endl;
  return 0;
}


void LV2Host::select_program(unsigned long program) {
  map<uint32_t, LV2Preset>::const_iterator iter = m_presets.find(program);
  if (iter != m_presets.end()) {
    map<uint32_t, float>::const_iterator piter;
    for (piter = iter->second.values.begin(); 
         piter != iter->second.values.end(); ++piter) {
      *(float*)m_ports[piter->first].buffer = piter->second;
      queue_control(piter->first, piter->second, false);
    }
  }
}


const std::vector<int>& LV2Host::get_midi_map() const {
  return m_midimap;
}


const std::string& LV2Host::get_gui_path() const {
  return m_standalonegui;
}


const std::string& LV2Host::get_bundle_dir() const {
  return m_bundledir;
}


void LV2Host::queue_program(unsigned long program, bool to_jack) {
  if (to_jack) {
    //cerr<<__PRETTY_FUNCTION__<<endl;
    pthread_mutex_lock(&m_mutex);
    m_to_jack.write_program(program);
    pthread_mutex_unlock(&m_mutex);
  }
  else if (m_from_jack)
    m_from_jack->write_program(program);
}


void LV2Host::queue_control(unsigned long port, float value, bool to_jack) {
  if (port < m_ports.size()) {
    if (to_jack) {
      pthread_mutex_lock(&m_mutex);
      m_to_jack.write_control(port, value);
      pthread_mutex_unlock(&m_mutex);
    }
    else if (m_from_jack)
      m_from_jack->write_control(port, value);
  }
}


void LV2Host::queue_midi(uint32_t port, uint32_t size, 
                         const unsigned char* midi) {
  if (port < m_ports.size() && m_ports[port].midi) {
    if (size <= 4) {
      pthread_mutex_lock(&m_mutex);
      m_to_jack.write_midi(port, size, midi);
      pthread_mutex_unlock(&m_mutex);
    }
    else
      cerr<<"Can only write MIDI events smaller than 5 bytes"<<endl;
  }
  else
    cerr<<"Trying to write MIDI to invalid port "<<port<<endl;
}


void LV2Host::queue_config_request(EventQueue* sender) {
  m_to_jack.write_config_request(sender);
}


void LV2Host::queue_passthrough(const char* msg, void* ptr) {
  m_to_jack.write_passthrough(msg, ptr);
}


void LV2Host::set_program(uint32_t program) {
  map<uint32_t, LV2Preset>::const_iterator iter = m_presets.find(program);
  if (iter == m_presets.end()) {
    cerr<<"Could not find program "<<program<<endl;
  }
  else {
    map<uint32_t, float>::const_iterator piter = iter->second.values.begin();
    for ( ; piter != iter->second.values.end(); ++iter)
      queue_control(piter->first, piter->second, true);
  }
}


void LV2Host::set_event_queue(EventQueue* q) {
  m_from_jack = q;
}


const std::map<std::string, std::string>& LV2Host::get_config() const {
  return m_configuration;
}


const std::map<uint32_t, LV2Preset>& LV2Host::get_presets() const {
  return m_presets;
}



LV2Host* LV2Host::m_current_object(0);

