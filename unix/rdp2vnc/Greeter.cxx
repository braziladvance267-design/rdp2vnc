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

#include <string>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>

#include <ncurses.h>

#include <rdp2vnc/RDPClient.h>
#include <rdp2vnc/Greeter.h>

using namespace std;
using namespace rfb;

static rfb::LogWriter vlog("Greeter");

Greeter::Greeter(int argc_, char** argv_, bool& stopSignal_)
  : argc(argc_), argv(argv_), stopSignal(stopSignal_) {
}

void Greeter::handle(int infd, int outfd) {
  FILE* infile = fdopen(infd, "rb");
  FILE* outfile = fdopen(outfd, "wb");
  newterm("ansi-generic", outfile, infile);
  printw("test");
  refresh();
  getch();
  endwin();
  if (stopSignal) {
    return;
  }
  client.reset(new RDPClient(argc, argv, stopSignal));
  if (!client->init() || !client->start() || !client->waitConnect()) {
    client.reset(nullptr);
  }
}

std::unique_ptr<RDPClient>& Greeter::getRDPClient() {
  return client;
}