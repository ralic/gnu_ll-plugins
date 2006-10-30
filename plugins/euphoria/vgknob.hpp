#ifndef VGKNOB_HPP
#define VGKNOB_HPP

#include <gtkmm.h>


class VGKnob : public Gtk::DrawingArea {
public:
  
  VGKnob(float min, float max, float value, 
         float red, float green, float blue, bool integer, bool logarithmic);
  
  void set_value(float value);
  
  Gtk::Adjustment& get_adjustment();
  
protected:
  
  bool on_expose_event(GdkEventExpose* event);
  bool on_motion_notify_event(GdkEventMotion* event);
  bool on_button_press_event(GdkEventButton* event);
  bool on_scroll_event(GdkEventScroll* event);
  
  int draw_digit(Cairo::RefPtr<Cairo::Context>& cc, char digit);
  void draw_string(Cairo::RefPtr<Cairo::Context>& cc, const std::string& str,
                   float x, float y);
  double map_to_adj(double knob);
  double map_to_knob(double adj);
  
  Gtk::Adjustment m_adj;
  int m_click_offset;
  float m_value_offset;
  float m_red, m_green, m_blue;
  bool m_integer;
  bool m_logarithmic;
  float m_step;
};


#endif