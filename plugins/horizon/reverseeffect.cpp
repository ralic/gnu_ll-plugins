/****************************************************************************

    reverseeffect.cpp

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

#include "reverseeffect.hpp"


ReverseEffect::ReverseEffect()
  : Effect("Reverse") {
  
}
  

void ReverseEffect::process(const float* input, float* output, size_t nframes) {
  for (size_t i = 0; i < nframes / 2; ++i) {
    float tmp = input[i];
    output[i] = input[nframes - i - 1];
    output[nframes - i - 1] = tmp;
  }
}
