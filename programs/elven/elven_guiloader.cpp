/****************************************************************************
    
    elven_guiloader.cpp - A program that loads LV2 GUI plugins and can be
                          controlled via OSC
    
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
#include <dlfcn.h>

#include <gtkmm.h>
#include <gtkmm/plug.h>

#include "lv2-gtk2gui.h"
#include "lv2uiclient.hpp"


using namespace std;
using namespace Gtk;
using namespace sigc;


void set_control(LV2UI_Controller c, uint32_t port, float value) {
  static_cast<LV2UIClient*>(c)->send_control(port, value);
}


void* extension_data(LV2UI_Controller c, const char* URI) {
  return 0;
}


int main(int argc, char** argv) {
  
  // initialise GTK
  Gtk::Main kit(argc, argv);
  
  if (argc < 6) {
    cerr<<"Not enough parameters."<<endl;
    return 1;
  }
  
  const char* filename = argv[1];
  const char* osc_url = argv[2];
  const char* uri = argv[3];
  const char* bundle_path = argv[4];
  const char* name = argv[5];
  int socket_id = -1;
  if (argc > 6)
    socket_id = atoi(argv[6]);
  
  LV2UI_ControllerDescriptor cdesc = {
    &set_control,
    &extension_data
  };
  
  // open the module
  void* module = dlopen(filename, RTLD_LAZY);
  if (!module) {
    cerr<<"Could not load "<<filename<<endl;
    return 1;
  }
  
  // get the GUI descriptor
  LV2UI_UIDescriptorFunction func = 
    (LV2UI_UIDescriptorFunction)dlsym(module, "lv2ui_descriptor");
  if (!func) {
    cerr<<"Could not find symbol lv2ui_descriptor in "<<filename<<endl;
    return 1;
  }
  const LV2UI_UIDescriptor* desc = func(uri);
  if (!desc) {
    cerr<<"lv2ui_descriptor() returned NULL"<<endl;
    return 1;
  }
  
  // create an OSC server
  LV2UIClient osc(osc_url, bundle_path, uri, name, true);
  if (!osc.is_valid()) {
    cerr<<"Could not start an OSC server"<<endl;
    return 1;
  }
  
  // create a GUI instance
  GtkWidget* cwidget;
  LV2UI_Controller ctrl = static_cast<LV2UI_Controller>(&osc);
  LV2UI_Handle ui = desc->instantiate(&cdesc, ctrl, uri, bundle_path, &cwidget);
  if (!ui || !cwidget) {
    cerr<<"Could not create an UI"<<endl;
    return 1;
  }
  
  // connect OSC server to UI
  if (desc->set_control) {
    osc.control_received.connect(bind<0>(desc->set_control, ui));
  }
  
  // show the GUI instance and start the main loop
  if (socket_id == -1) {
    Window win;
    win.set_title(name);
    win.add(*Glib::wrap(cwidget));
    win.show_all();
    osc.send_update_request();
    kit.run(win);
  }
  else {
    Plug plug(socket_id);
    plug.add(*Glib::wrap(cwidget));
    osc.send_update_request();
    kit.run(plug);
  }
  
  // clean up
  desc->cleanup(ui);
  dlclose(module);
  
  return 0;
}
