#include <cstdio>
#include <iostream>

#include "samplemodel.hpp"
#include "sampleview.hpp"


using namespace std;
using namespace Gtk::Menu_Helpers;


SampleView::SampleView() 
  : m_model(0),
    m_scroll_adj(0, 0, 0),
    m_scale(4),
    m_active_frame(0) {
  
  m_bg.set_rgb(55000, 55000, 60000);
  m_bgl.set_rgb(65000, 65000, 65000);
  m_bgd.set_rgb(20000, 20000, 40000);
  m_fg.set_rgb(0, 20000, 65000);
  Glib::RefPtr<Gdk::Colormap> cmap = Gdk::Colormap::get_system();
  cmap->alloc_color(m_bg);
  cmap->alloc_color(m_bgl);
  cmap->alloc_color(m_bgd);
  cmap->alloc_color(m_fg);
  
  add_events(Gdk::EXPOSURE_MASK | Gdk::BUTTON1_MOTION_MASK | 
             Gdk::BUTTON_PRESS_MASK | Gdk::SCROLL_MASK);  
  
  m_scroll_adj.signal_value_changed().
    connect(mem_fun(*this, &SampleView::queue_draw));
  
  m_menu.items().
    push_back(MenuElem("Add splitpoint", 
                       mem_fun(*this, &SampleView::do_add_splitpoint)));
  m_menu.items().
    push_back(MenuElem("Remove splitpoint", 
                       mem_fun(*this, &SampleView::do_remove_splitpoint)));
}


void SampleView::set_model(SampleModel* model) {
  m_model = model;
  queue_draw();
  if (m_model)
    m_scroll_adj.set_upper(model->get_length() / pow(2.0, m_scale) + 1);
  else
    m_scroll_adj.set_upper(0);
  m_scroll_adj.set_value(0);
}


Gtk::Adjustment& SampleView::get_scroll_adjustment() {
  return m_scroll_adj;
}

  
bool SampleView::on_expose_event(GdkEventExpose* event) {
  
  Glib::RefPtr<Gdk::Window> win = get_window();
  Glib::RefPtr<Gdk::GC> gc = Gdk::GC::create(win);
  gc->set_foreground(m_bg);
  int w = get_width();
  int h = get_height();
  
  // draw background
  win->draw_rectangle(gc, true, 0, 0, w, h);
  gc->set_foreground(m_bgd);
  win->draw_line(gc, w - 1, 0, w - 1, h - 1);
  win->draw_line(gc, 0, h - 1, w - 1, h - 1);
  gc->set_foreground(m_bgl);
  win->draw_line(gc, 0, 0, w, 0);
  win->draw_line(gc, 0, 0, 0, h);
  
  if (!m_model)
    return true;
  
  // draw audio
  int m = h / 2;
  gc->set_foreground(m_fg);
  const SampleModel::PeakData* peak = 0;
  float scale = 0;
  size_t scroll = size_t(m_scroll_adj.get_value());
  size_t lines = get_width();
  if (scroll + lines > m_model->get_length() / pow(2.0, m_scale))
    lines = size_t(m_model->get_length() / pow(2.0, m_scale) - scroll);
  
  // if we are zoomed in closer than 1:1, draw each sample as a tiny box
  if (m_scale < 0) {
    scale = pow(2.0, m_scale);
    const float* data = m_model->get_data(0);
    const int offset = int(scroll * scale);
    for (size_t i = 0; i < lines * scale; ++i) {
      win->draw_rectangle(gc, true, int(i / scale), 
			  m + int((m - 1) * data[i + offset]) - 1, 3, 3); 
    }
  }
  
  // if the scale is 1:1 < 1:16, use the actual data
  else if (m_scale < 4) {
    scale = pow(2.0, m_scale);
    const float* data = m_model->get_data(0);
    for (size_t i = scroll; i < scroll + lines; ++i) {
      float min = data[int(i*scale)];
      float max = data[int(i*scale)];
	for (size_t j = 0; j < scale; ++j) {
	  min = min < data[int(i*scale + j)] ? min : data[int(i*scale + j)];
	  max = max > data[int(i*scale + j)] ? max : data[int(i*scale + j)];
	}
	
	win->draw_line(gc, i - scroll, m + int((m - 1) * min),
		       i - scroll, m + int((m - 1) * max));
    }
  }
  
  // else, select a peak level depending on the scale
  else {
    if (m_scale < 8) {
      scale = pow(2.0, m_scale - 4);
      peak = m_model->get_peak_data()[0];
    }
    
    else if (m_scale < 12) {
      scale = pow(2.0, m_scale - 8);
      peak = m_model->get_peak_data()[1];
    }
    
    else {
      scale = pow(2.0, m_scale - 12);
      peak = m_model->get_peak_data()[2];
    }
    
    if (peak) {
      size_t scroll = size_t(m_scroll_adj.get_value());
      for (size_t i = scroll; i < scroll + lines; ++i) {
	float min = peak[int(i*scale)].min;
	float max = peak[int(i*scale)].max;
	for (size_t j = 0; j < scale; ++j) {
	  min = min < peak[int(i*scale + j)].min ? 
	    min : peak[int(i*scale + j)].min;
	  max = max > peak[int(i*scale + j)].max ? 
	    max : peak[int(i*scale + j)].max;
	}
	
	win->draw_line(gc, i - scroll, m + int((m - 1) * min),
		       i - scroll, m + int((m - 1) * max));
      }
    }
  }
  
  // draw splitpoints
  const vector<size_t>& seg = m_model->get_splitpoints();
  for (size_t i = 0; i < seg.size(); ++i) {
    int x = int(seg[i] / pow(2.0, m_scale) - scroll);
    if (x < 0)
      continue;
    else if (x > get_width())
      break;
    gc->set_foreground(m_bgd);
    win->draw_line(gc, x - 1, 0, x - 1, h - 1);
    gc->set_foreground(m_bgl);
    win->draw_line(gc, x, 0, x, h - 1);
  }

  return true;
}


bool SampleView::on_motion_notify_event(GdkEventMotion* event) {
  return true;
}


bool SampleView::on_button_press_event(GdkEventButton* event) {
  
  if (event->button == 3) {
    m_active_frame = size_t((m_scroll_adj.get_value() + event->x) * 
			    pow(2.0, m_scale));
    m_menu.popup(event->button, event->time);
  }
  
  return true;
}


bool SampleView::on_scroll_event(GdkEventScroll* event) {
  double frame = (m_scroll_adj.get_value() + event->x + 0.5) * pow(2.0, m_scale);
  if (event->direction == GDK_SCROLL_UP && m_scale > -4)
    --m_scale;
  else if (event->direction == GDK_SCROLL_DOWN && m_scale < 16)
    ++m_scale;
  if (m_model) {
    m_scroll_adj.set_upper(m_model->get_length() / pow(2.0, m_scale) + 1);
    double new_scroll = frame / pow(2.0, m_scale) - event->x - 0.5;
    new_scroll = new_scroll < 0 ? 0 : new_scroll;
    double max = m_scroll_adj.get_upper() - m_scroll_adj.get_page_size();
    new_scroll = new_scroll > max ? max : new_scroll;
    m_scroll_adj.set_value(new_scroll);
    queue_draw();
  }

  return true;
}


void SampleView::on_size_allocate(Gtk::Allocation& a) {
  DrawingArea::on_size_allocate(a);
  m_scroll_adj.set_page_size(a.get_width());
}


void SampleView::do_add_splitpoint() {
  if (m_model) {
    if (m_active_frame < m_model->get_length())
      m_signal_add_splitpoint(m_active_frame);
  }
}


void SampleView::do_remove_splitpoint() {
  cerr<<"Remove splitpoint at frame "<<m_active_frame
      <<", signal not sent"<<endl;
}


sigc::signal<void, size_t>& SampleView::signal_add_splitpoint() {
  return m_signal_add_splitpoint;
}


sigc::signal<void, size_t>& SampleView::signal_remove_splitpoint() {
  return m_signal_remove_splitpoint;
}
