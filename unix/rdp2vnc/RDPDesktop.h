/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2004-2008 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2017 Peter Astrand <astrand@cendio.se> for Cendio AB
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

#ifndef __RDPDESKTOP_H__
#define __RDPDESKTOP_H__

#include <memory>

#include <rfb/SDesktop.h>
#include <unixcommon.h>

struct RDPCursor;
class Geometry;
class RDPPixelBuffer;
class RDPClient;

// number of XKb indicator leds to handle
#define RDPDESKTOP_N_LEDS 3

class RDPDesktop : public rfb::SDesktop
{
public:
  RDPDesktop(Geometry* geometry, RDPClient* client);
  virtual ~RDPDesktop();
  void poll();
  // -=- SDesktop interface
  virtual void start(rfb::VNCServer* vs);
  virtual void stop();
  virtual void terminate();
  bool isRunning();
  virtual void queryConnection(network::Socket* sock,
                               const char* userName);
  virtual void pointerEvent(const rfb::Point& pos, int buttonMask);
  virtual void keyEvent(rdr::U32 keysym, rdr::U32 xtcode, bool down);
  virtual void clientCutText(const char* str);
  virtual unsigned int setScreenLayout(int fb_width, int fb_height,
                                       const rfb::ScreenSet& layout);
  virtual void handleClipboardRequest();
  virtual void handleClipboardData(const char* data);
protected:
  friend class RDPClient;
  Geometry* geometry;
  std::unique_ptr<RDPPixelBuffer> pb;
  rfb::VNCServer* server;
  RDPClient *client;
  bool running;
  int ledMasks[RDPDESKTOP_N_LEDS];
  unsigned ledState;
  const unsigned short *codeMap;
  unsigned codeMapLen;
  std::unique_ptr<RDPCursor> firstCursor;
  rfb::ScreenSet computeScreenLayout();
  bool setFirstCursor(std::unique_ptr<RDPCursor> &cursor);
  bool resize();
};

#endif // __RDPDESKTOP_H__
