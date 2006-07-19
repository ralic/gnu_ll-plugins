#include <iostream>

#include <gtkmm.h>

#include "lv2uiclient.hpp"
#include "panelfx.xpm"
#include "knob.hpp"
#include "switch.hpp"

using namespace Gtk;
using namespace Gdk;
using namespace Glib;


void add_knob(Fixed& fbox, LV2UIClient& lv2, unsigned long port, 
              float min, float max, float value, 
              RefPtr<Pixmap>& bg, int xoffset, int yoffset) {
  RefPtr<Pixmap> spix = Pixmap::create(bg, 44, 44);
  spix->draw_drawable(GC::create(spix), bg, xoffset, yoffset,0, 0, 44, 44);
  Knob* knob = manage(new Knob(min, max, value));
  fbox.put(*knob, xoffset, yoffset);
  knob->get_window()->set_back_pixmap(spix, false);
  lv2.connect_adjustment(&knob->get_adjustment(), port);
}


void add_switch(Fixed& fbox, LV2UIClient& lv2, unsigned long port,
                int xoffset, int yoffset, Switch::Type type) {
  Switch* sw = manage(new Switch(type));
  fbox.put(*sw, xoffset, yoffset);
  lv2.connect_adjustment(&sw->get_adjustment(), port);
}


int main(int argc, char** argv) {
  
  LV2UIClient lv2(argc, argv, true);
  if (!lv2.is_valid()) {
    cerr<<"Could not start OSC listener. You are not running the "<<endl
        <<"program manually, are you? It's supposed to be started by "<<endl
        <<"the plugin host."<<endl;
    return 1;
  }

  Main kit(argc, argv);
  Gtk::Window window;
  window.set_title(string("AZR-3: ") + lv2.get_identifier());
  Fixed fbox;
  fbox.set_has_window(true);
  window.add(fbox);
  window.show_all();
  RefPtr<Pixbuf> pixbuf = Pixbuf::create_from_xpm_data(panelfx);
  fbox.set_size_request(pixbuf->get_width(), pixbuf->get_height());
  RefPtr<Pixmap> pixmap = Pixmap::create(fbox.get_window(), 
                                         pixbuf->get_width(), 
                                         pixbuf->get_height());
  RefPtr<Bitmap> bitmap;
  pixbuf->render_pixmap_and_mask(pixmap, bitmap, 10);
  fbox.get_window()->set_back_pixmap(pixmap, false);
  
  fbox.get_window()->clear();
  
  add_switch(fbox, lv2, 0, 302, 332, Switch::Green);
  add_switch(fbox, lv2, 1, 323, 356, Switch::BigRed);
  add_knob(fbox, lv2, 2, 0, 1, 0.5, pixmap, 352, 352);
  add_knob(fbox, lv2, 3, 0, 1, 0.5, pixmap, 396, 352);
  add_knob(fbox, lv2, 4, 0, 1, 0.5, pixmap, 440, 352);
  add_knob(fbox, lv2, 5, 0, 1, 0.5, pixmap, 484, 352);
  add_knob(fbox, lv2, 6, 0, 1, 0.5, pixmap, 528, 352);
  add_switch(fbox, lv2, 8, 443, 331, Switch::Mini);
  
  lv2.show_received.connect(mem_fun(window, &Gtk::Window::show_all));
  lv2.hide_received.connect(mem_fun(window, &Gtk::Window::hide));
  lv2.quit_received.connect(&Main::quit);
  window.signal_delete_event().connect(bind_return(hide(&Main::quit), true));
  
  lv2.send_update_request();
  
  kit.run();

  return 0;
}