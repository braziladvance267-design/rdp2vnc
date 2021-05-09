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

#include <memory>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>
#include <clocale>
#include <cassert>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>
#include <rfb/VNCServerST.h>
#include <rfb/Configuration.h>
#include <rfb/Timer.h>
#include <network/TcpSocket.h>
#include <network/UnixSocket.h>

#include <xkbcommon/xkbcommon.h>

#include <rdp2vnc/Terminal.h>
#include <rdp2vnc/Geometry.h>
#include <rdp2vnc/tmt.h>
#include <rdp2vnc/font.h>
#include <rdp2vnc/key.h>

static const int glyphWidth = 16;
static const int glyphHeight = 16;

using namespace std;
using namespace rfb;
using namespace network;

static rfb::LogWriter vlog("Terminal");

TerminalDesktop::TerminalDesktop(Geometry* geometry_)
  : geometry(geometry_), server(NULL), running(false), vt(NULL)
{
  std::setlocale(LC_ALL, "en_US.utf8");
  PixelFormat format(32, 24, false, true, 255, 255, 255, 16, 8, 0);
  pb.reset(new ManagedPixelBuffer(format, geometry->width(), geometry->height()));
  int stride;
  uint8_t* buffer = pb->getBufferRW(geometry->getRect(), &stride);
  int width = geometry->width();
  int height = geometry->height();
  for (int i = 0; i < height; ++i) {
    for (int j = 0; j < width; ++j) {
      buffer[(i * stride + j) * 4 + 0] = 255;
      buffer[(i * stride + j) * 4 + 1] = 255;
      buffer[(i * stride + j) * 4 + 2] = 255;
      buffer[(i * stride + j) * 4 + 3] = 0;
    }
  }
}

TerminalDesktop::~TerminalDesktop() {
  if (vt) {
    tmt_close(vt);
  }
  close(inPipeFd[0]);
  close(inPipeFd[1]);
  close(outPipeFd[0]);
  close(outPipeFd[1]);
}

bool TerminalDesktop::initTerminal(int lines, int cols) {
  this->lines = lines;
  this->cols = cols;
  vt = tmt_open(lines, cols, callbackTMT, this, NULL);
  if (!vt) {
    return false;
  }
  if (pipe(inPipeFd) < 0 || pipe(outPipeFd) < 0) {
    return false;
  }
  return true;
}

void TerminalDesktop::start(VNCServer* vs) {
  server = vs;
  server->setPixelBuffer(pb.get(), computeScreenLayout());
  running = true;
}

void TerminalDesktop::stop() {
  running = false;

  //server->setPixelBuffer(0);
}

void TerminalDesktop::terminate() {
}

bool TerminalDesktop::isRunning() {
  return running;
}

void TerminalDesktop::queryConnection(network::Socket* sock,
                                      const char* userName)
{
  assert(isRunning());
}

void TerminalDesktop::pointerEvent(const Point& pos, int buttonMask) {
}

void TerminalDesktop::keyEvent(rdr::U32 keysym, rdr::U32 xtcode, bool down) {
  if (down) {
    pressedKeys.insert(keysym);
  } else {
    pressedKeys.erase(keysym);
  }
  if (down) {
    const char* str = NULL;
    auto it = keysymTMTKeyMap.find(keysym);
    if (keysym == XKB_KEY_Tab && (pressedKeys.count(XKB_KEY_Shift_L) || pressedKeys.count(XKB_KEY_Shift_R))) {
      str = TMT_KEY_BACK_TAB;
    } else if (it != keysymTMTKeyMap.end()) {
      str = it->second;
    }
    if (str != NULL) {
      write(inPipeFd[1], str, strlen(str));
    } else {
      char buffer[7];
      int len = xkb_keysym_to_utf8(keysym, buffer, 7);
      if (len > 0) {
        write(inPipeFd[1], buffer, len - 1);
      }
    }
  }
}

void TerminalDesktop::clientCutText(const char* str) {
}

ScreenSet TerminalDesktop::computeScreenLayout()
{
  ScreenSet layout;

  // Make sure that we have at least one screen
  if (layout.num_screens() == 0)
    layout.add_screen(rfb::Screen(0, 0, 0, geometry->width(),
                                  geometry->height(), 0));

  return layout;
}

unsigned int TerminalDesktop::setScreenLayout(int fb_width, int fb_height,
                                         const rfb::ScreenSet& layout)
{
  return rfb::resultProhibited;
}

void TerminalDesktop::handleClipboardRequest() {
}

void TerminalDesktop::handleClipboardAnnounce(bool available) {
}

void TerminalDesktop::handleClipboardData(const char* data) {
}

void TerminalDesktop::callbackTMT(tmt_msg_t m, TMT* vt, const void* a, void* p) {
  if (!p) {
    return;
  }
  TerminalDesktop* desktop = (TerminalDesktop*)p;
  desktop->processTerminalEvent(m, vt, a);
}

void TerminalDesktop::processTerminalEvent(tmt_msg_t m, TMT* vt, const void* arg) {
  const TMTSCREEN* screen = tmt_screen(vt);
  const TMTPOINT* cursor = tmt_cursor(vt);
  switch (m) {
    case TMT_MSG_BELL:
      if (server) {
        try {
          server->bell();
        } catch (rdr::Exception& e) {
          vlog.error("Bell: %s", e.str());
        }
      }
      break;
    case TMT_MSG_UPDATE:
      if (pb) {
        for (size_t r = 0; r < screen->nline; ++r) {
          if (screen->lines[r]->dirty) {
            for (size_t c = 0; c < screen->ncol; ++c) {
              Rect glyphRect = terminalPosToRFBRect(c, r);
              int stride;
              uint8_t* buffer = pb->getBufferRW(glyphRect, &stride);
              wchar_t ch = screen->lines[r]->chars[c].c;
              size_t glyphIndex = ch < glyphBitmapSize ? ch : 0;
              for (int i = 0; i < glyphHeight; ++i) {
                uint16_t line = glyphBitmap[glyphIndex][i];
                for (int j = 0; j < glyphWidth; ++j) {
                  uint8_t pixel = (line & (1 << j)) ? 0 : 255;
                  buffer[(i * stride + j) * 4 + 0] = pixel;
                  buffer[(i * stride + j) * 4 + 1] = pixel;
                  buffer[(i * stride + j) * 4 + 2] = pixel;
                  buffer[(i * stride + j) * 4 + 3] = 0;
                }
              }
              if (server) {
                try {
                  server->add_changed(Region(terminalLineToRFBRect(r)));
                } catch (rfb::Exception& e) {
                  vlog.error("Add change: %s", e.str());
                }
              }
            }
          }
        }
      }
      tmt_clean(vt);
      break;
  }
}

rfb::Rect TerminalDesktop::terminalPosToRFBRect(int x, int y) {
  int padx = (geometry->width() - cols * glyphWidth) / 2;
  int pady = (geometry->height() - lines * glyphHeight) / 2;
  int x1 = padx + glyphWidth * x;
  int y1 = pady + glyphHeight * y;
  int x2 = x1 + glyphWidth;
  int y2 = y1 + glyphHeight;
  return Rect(x1, y1, x2, y2);
}

rfb::Rect TerminalDesktop::terminalLineToRFBRect(int line) {
  int padx = (geometry->width() - cols * glyphWidth) / 2;
  int pady = (geometry->height() - lines * glyphHeight) / 2;
  int x1 = padx;
  int y1 = pady + glyphHeight * line;
  int x2 = geometry->width() - padx;
  int y2 = y1 + glyphHeight;
  return Rect(x1, y1, x2, y2);
}

int TerminalDesktop::getInFd() {
  return inPipeFd[0];
}

int* TerminalDesktop::getOutFds() {
  return outPipeFd;
}

TMT* TerminalDesktop::getTerminal() {
  return vt;
}

bool runTerminal(TerminalDesktop* desktop, rfb::VNCServerST* server,
  std::list<SocketListener *>& listeners, bool* caughtSignal,
  const std::function<void (int infd, int outfd)>& terminalHandler) {

  int inFd = desktop->getInFd();
  int* outFds = desktop->getOutFds();
  fcntl(outFds[0], F_SETFL, fcntl(outFds[0], F_GETFL, 0) | O_NONBLOCK);
  char buffer[2048];
  atomic_bool hasTerminalFinished = false;

  thread handlerThread([&] {
    terminalHandler(inFd, outFds[1]);
    hasTerminalFinished = true;
  });
  handlerThread.detach();

  while (true) {
    if (*caughtSignal) {
      return false;
    }
    struct timeval tv;
    fd_set rfds, wfds;
    std::list<Socket*> sockets;
    std::list<Socket*>::iterator i;
        
    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    for (std::list<SocketListener*>::iterator i = listeners.begin();
         i != listeners.end();
         i++)
      FD_SET((*i)->getFd(), &rfds);
    FD_SET(outFds[0], &rfds);

    server->getSockets(&sockets);
    int clients_connected = 0;
    for (i = sockets.begin(); i != sockets.end(); i++) {
      if ((*i)->isShutdown()) {
        server->removeSocket(*i);
        delete (*i);
      } else {
        FD_SET((*i)->getFd(), &rfds);
        if ((*i)->outStream().hasBufferedData())
          FD_SET((*i)->getFd(), &wfds);
        clients_connected++;
      }
    }

    tv.tv_sec = 0;
    tv.tv_usec = 10000;
    // Do the wait...
    int n = select(FD_SETSIZE, &rfds, &wfds, 0, &tv);

    if (n < 0) {
      if (errno == EINTR) {
        vlog.debug("Interrupted select() system call");
        continue;
      } else {
        throw rdr::SystemException("select", errno);
      }
    }

    if (FD_ISSET(outFds[0], &rfds)) {
      ssize_t len = read(outFds[0], buffer, sizeof(buffer));
      if (len > 0) {
        TMT* vt = desktop->getTerminal();
        if (vt) {
          tmt_write(vt, buffer, len);
        }
      }
    }


    for (std::list<SocketListener*>::iterator i = listeners.begin();
         i != listeners.end();
         i++) {
      if (FD_ISSET((*i)->getFd(), &rfds)) {
        Socket* sock = (*i)->accept();
        if (sock) {
          server->addSocket(sock);
        } else {
          vlog.status("Client connection rejected");
        }
      }
    }

    Timer::checkTimeouts();

    // Client list could have been changed.
    server->getSockets(&sockets);

    // Nothing more to do if there are no client connections.
    if (sockets.empty())
      continue;

    // Process events on existing VNC connections
    for (i = sockets.begin(); i != sockets.end(); i++) {
      if (FD_ISSET((*i)->getFd(), &rfds)) {
        server->processSocketReadEvent(*i);
      }
      if (FD_ISSET((*i)->getFd(), &wfds)) {
        server->processSocketWriteEvent(*i);
      }
    }
    if (hasTerminalFinished) {
      return true;
    }
  }
}