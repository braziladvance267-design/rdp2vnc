/* Copyright 2021 Dinglan Peng
 *    
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

//
// RDPPixelBuffer.h
//

#ifndef __RPDPIXELBUFFER_H__
#define __RDPPIXELBUFFER_H__

#include <rfb/PixelBuffer.h>
#include <rfb/VNCServer.h>

//
// RDPPixelBuffer is an Image-based implementation of FullFramePixelBuffer.
//

class RDPClient;
class RDPPixelBuffer : public rfb::FullFramePixelBuffer
{
public:
  RDPPixelBuffer(const rfb::Rect &rect, RDPClient* client_);
  virtual ~RDPPixelBuffer();
private:
  RDPClient* client;
};

#endif // __RDPPIXELBUFFER_H__

