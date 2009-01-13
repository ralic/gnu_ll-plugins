/****************************************************************************

    chunk.cpp

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

#include "chunk.hpp"

  
Chunk::Chunk(const Sample& sample, uint32_t start, uint32_t end,
	     const std::string& name)
  : m_sample(sample),
    m_start(start),
    m_end(end),
    m_name(name) {

}
  

uint32_t Chunk::get_start() const {
  return m_start;
}
  

uint32_t Chunk::get_end() const {
  return m_end;
}


const Sample& Chunk::get_sample() const {
  return m_sample;
}


const std::string& Chunk::get_name() const {
  return m_name;
}