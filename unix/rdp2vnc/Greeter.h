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
// Geometry.h
//

#ifndef __GREETER_H__
#define __GREETER_H__

#include <memory>
#include <rfb/Rect.h>
#include <rfb/Configuration.h>

#include <rdp2vnc/RDPClient.h>

class Greeter
{
public:
  Greeter(int argc_, char** argv_, bool& stopSignal_);
  void handle(int infd, int outfd);
  std::unique_ptr<RDPClient>& getRDPClient();
private:
  int argc;
  char** argv;
  bool& stopSignal;
  std::unique_ptr<RDPClient> client;
};

#endif // __GREETER_H__

