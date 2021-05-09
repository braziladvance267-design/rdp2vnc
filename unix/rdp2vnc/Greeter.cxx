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
#include <iostream>
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
  //FILE* infile = fdopen(infd, "rb");
  //FILE* outfile = fdopen(outfd, "wb");
  string strBanner = "请输入用户名及密码以登录系统。\r\n";
  string strUsername = "用户名：";
  string strPassword = "\r\n密码：";
  string strInvisible = "*";
  string strNewline = "\r\n";
  string strFailed = "登录失败！请重试。\r\n";
  string strBackspace = "\b \b";
  write(outfd, strBanner.c_str(), strBanner.length());
  const int maxRetryTimes = 5;
  for (int i = 0; i < maxRetryTimes; ++i) {
    string domain, username, password;
    if (write(outfd, strUsername.c_str(), strUsername.length()) <= 0) {
      return;
    }
    char ch;
    while (true) {
      if (read(infd, &ch, 1) <= 0) {
        return;
      }
      if (ch == '\r' || ch == '\n') {
        break;
      } else if (ch == '\b') {
        if (!username.empty()) {
          if (write(outfd, strBackspace.c_str(), strBackspace.length()) <= 0) {
            return;
          }
          username = username.substr(0, username.length() - 1);
        }
      } else {
        username += ch;
        if (write(outfd, &ch, 1) <= 0) {
          return;
        }
      }
    }

    if (write(outfd, strPassword.c_str(), strPassword.length()) <= 0) {
      return;
    }
    while (true) {
      if (read(infd, &ch, 1) <= 0) {
        return;
      }
      if (ch == '\r' || ch == '\n') {
        break;
      } else if (ch == '\b') {
        if (!password.empty()) {
          if (write(outfd, strBackspace.c_str(), strBackspace.length()) <= 0) {
            return;
          }
          password = password.substr(0, password.length() - 1);
        }
      } else {
        password += ch;
        if (write(outfd, strInvisible.c_str(), strInvisible.length()) <= 0) {
          return;
        }
      }
    }

    if (write(outfd, strNewline.c_str(), strNewline.length()) <= 0) {
      return;
    }

    size_t pos = username.find('\\');
    if (pos != string::npos) {
      domain = username.substr(0, pos);
    }
    char* domainCstr = domain.empty() ? nullptr : strdup(domain.c_str());
    char* usernameCstr = strdup(username.c_str());
    char* passwordCstr = strdup(password.c_str());
    client.reset(new RDPClient(argc, argv, stopSignal));
    if (!client->init(domainCstr, usernameCstr, passwordCstr) || !client->start() || !client->waitConnect()) {
      client.reset(nullptr);
      if (write(outfd, strFailed.c_str(), strFailed.length()) <= 0) {
        return;
      }
      continue;
    }
    return;
  }
}

std::unique_ptr<RDPClient>& Greeter::getRDPClient() {
  return client;
}