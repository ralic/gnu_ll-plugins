/****************************************************************************
    
    reverb.hpp - a reverb effect
    
    Copyright (C) 2007 Lars Luthman <larsl@users.sourceforge.net>
    
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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307  USA

****************************************************************************/

#ifndef REVERB_HPP
#define REVERB_HPP

#include "householderfdn.hpp"


namespace {

  
  class Reverb {
  public:
    
    Reverb(uint32_t rate);
    
    void run(float* left, float* right, uint32_t nframes, 
	     float time, float mix, float damp);
    
  private:
    
    HouseholderFDN m_hhfdn;
    uint32_t m_rate;
    
  };


  Reverb::Reverb(uint32_t rate)
    : m_hhfdn(rate),
      m_rate(rate) {

  }
  
  
  void Reverb::run(float* left, float* right, uint32_t nframes, 
		   float time, float mix, float damp) {
    m_hhfdn.run(left, right, nframes, time, mix, damp);
  }
  
}


#endif
