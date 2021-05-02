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
// RDPClient.h
//

#ifndef __RDPCLIENT_H__
#define __RDPCLIENT_H__

#include <memory>
#include <thread>
#include <inttypes.h>
#include <unistd.h>

#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/utils/signal.h>

#include <freerdp/client/file.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/channels.h>
#include <freerdp/channels/channels.h>

#include <rfb/Rect.h>
#include <rfb/Configuration.h>

struct RDPCursor;
class RDPDesktop;
class RDPPointerImpl;
class RDPClient
{
public:
  RDPClient(int argc, char** argv);
  ~RDPClient();
  bool init();
  bool start();
  bool stop();
  bool waitConnect();
  bool registerFileDescriptors(fd_set* rfdset, fd_set* wfdset);
  bool processsEvents();
  bool startThread();
  void setRDPDesktop(RDPDesktop* desktop);
  int width();
  int height();
  rdr::U8* getBuffer();
  void pointerEvent(const rfb::Point& pos, int buttonMask);
  void keyEvent(rdr::U32 keysym, rdr::U32 xtcode, bool down);
private:
  friend class RDPDesktop;
  static BOOL rdpBeginPaint(rdpContext* context);
  static BOOL rdpEndPaint(rdpContext* context);
  static BOOL rdpPreConnect(freerdp* instance);
  static BOOL rdpPostConnect(freerdp* instance);
  static void rdpPostDisconnect(freerdp* instance);
  static BOOL rdpClientNew(freerdp* instance, rdpContext* context);
  static BOOL rdpPointerNew(rdpContext* context, rdpPointer* pointer);
  static void rdpPointerFree(rdpContext* context, rdpPointer* pointer);
  static BOOL rdpPointerSet(rdpContext* context, const rdpPointer* pointer);
  static BOOL rdpPointerSetPosition(rdpContext* context, UINT32 x, UINT32 y);
  bool beginPaint();
  bool endPaint();
  bool preConnect();
  bool postConnect();
  void postDisconnect();
  bool pointerSet(RDPPointerImpl* pointer);
  bool pointerSetPosition(uint32_t x, uint32_t y);
  void eventLoop();

  int argc;
  char** argv;
  rdpContext* context;
  freerdp* instance;
  RDPDesktop *desktop;
  bool hasConnected;
  int oldButtonMask;
  std::unique_ptr<std::thread> thread_;
  std::unique_ptr<RDPCursor> firstCursor;
};

#endif // __RDPCLIENT_H__

