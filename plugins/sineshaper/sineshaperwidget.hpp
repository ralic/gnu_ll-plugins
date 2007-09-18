/****************************************************************************
    
    sineshaperwidget.hpp - A GUI for the Sineshaper LV2 synth plugin
    
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

#ifndef SINESHAPERWIDGET_HPP
#define SINESHAPERWIDGET_HPP

#include <string>
#include <vector>

#include <gtkmm.h>

#include "skindial_gtkmm.hpp"


class SineshaperWidget : public Gtk::HBox {
public:
  
  SineshaperWidget(const std::string& bundle);
  
  void set_control(uint32_t port, float value);
  
  sigc::signal<void, uint32_t, float> signal_control_changed;
  
protected:
  
  Gtk::Widget* init_tuning_controls();
  Gtk::Widget* init_osc2_controls();
  Gtk::Widget* init_vibrato_controls();
  Gtk::Widget* init_portamento_controls();
  Gtk::Widget* init_tremolo_controls();
  Gtk::Widget* init_envelope_controls();
  Gtk::Widget* init_amp_controls();
  Gtk::Widget* init_delay_controls();
  Gtk::Widget* init_shaper_controls();
  Gtk::Widget* init_preset_list();
  
  Gtk::Widget* create_knob(Gtk::Table* table, int col, const std::string& name, 
			   float min, float max, SkinDial::Mapping mapping,
			   float center, uint32_t port);
  Gtk::Widget* create_spin(Gtk::Table* table, int col, const std::string& name, 
			   float min, float max, uint32_t port);
  
  
  Glib::RefPtr<Gdk::Pixbuf> m_dialg;
  std::vector<Gtk::Adjustment*> m_adjs;
  
};


#endif
