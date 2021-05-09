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
// DesktopMux.h
//

#ifndef __DESKTOPMUX_H__
#define __DESKTOPMUX_H__

#include <rfb/SDesktop.h>

class DesktopMux : public rfb::SDesktop
{
public:
  virtual void start(rfb::VNCServer* vs) {
    if (!desktop) {
      return;
    }
    desktop->start(vs);
  }
  virtual void stop() {
    if (!desktop) {
      return;
    }
    desktop->stop();
  }
  virtual void queryConnection(network::Socket* sock, const char* userName) {
    if (!desktop) {
      return;
    }
    desktop->queryConnection(sock, userName);
  }
  virtual void terminate() {
    if (!desktop) {
      return;
    }
    desktop->terminate();
  }
  virtual unsigned int setScreenLayout(int fb_width, int fb_height, const rfb::ScreenSet& layout) {
    if (!desktop) {
      return rfb::resultProhibited;
    }
    return desktop->setScreenLayout(fb_width, fb_height, layout);
  }
  virtual void handleClipboardRequest() {
    if (!desktop) {
      return;
    }
    desktop->handleClipboardRequest();
  }
  virtual void handleClipboardAnnounce(bool available) {
    if (!desktop) {
      return;
    }
    desktop->handleClipboardAnnounce(available);
  }
  virtual void handleClipboardData(const char* data) {
    if (!desktop) {
      return;
    }
    desktop->handleClipboardData(data);
  }
  virtual void keyEvent(rdr::U32 keysym, rdr::U32 keycode, bool down) {
    if (!desktop) {
      return;
    }
    desktop->keyEvent(keysym, keycode, down);
  }
  virtual void pointerEvent(const rfb::Point& pos, int buttonMask) {
    if (!desktop) {
      return;
    }
    desktop->pointerEvent(pos, buttonMask);
  }
  virtual void clientCutText(const char* str) {
    if (!desktop) {
      return;
    }
    desktop->clientCutText(str);
  }
  SDesktop* desktop;
};

#endif // __DESKTOPMUX_H__

