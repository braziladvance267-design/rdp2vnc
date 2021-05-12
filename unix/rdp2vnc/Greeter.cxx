/* Copyright Dinglan Peng
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
#include <tuple>
#include <iostream>
#include <rfb/Logger_stdio.h>
#include <rfb/LogWriter.h>

#include <ncurses.h>

#include <rdp2vnc/RDPClient.h>
#include <rdp2vnc/Terminal.h>
#include <rdp2vnc/Greeter.h>
#include <rdp2vnc/Geometry.h>

using namespace std;
using namespace rfb;

static rfb::LogWriter vlog("Greeter");

Greeter::Greeter(int argc_, char** argv_, bool& stopSignal_, TerminalDesktop& desktop_)
  : argc(argc_), argv(argv_), stopSignal(stopSignal_), desktop(desktop_) {
}

bool readLine(int infd, int outfd, const string &prompt, string &line, bool visible = true) {
  if (write(outfd, prompt.c_str(), prompt.length()) <= 0) {
    return false;
  }
  char ch;
  char mask = '*';
  while (true) {
    if (read(infd, &ch, 1) <= 0) {
      return false;
    }
    if (ch == '\r' || ch == '\n') {
      break;
    } else if (ch == '\b' || ch == 127) {
      if (!line.empty()) {
        if (write(outfd, "\e[1D \e[1D", 9) <= 0) {
          return false;
        }
        line = line.substr(0, line.length() - 1);
      }
    } else if (ch == '\e') {
      // ignore CSI
      while (true) {
        if (read(infd, &ch, 1) <= 0) {
          return false;
        }
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
          break;
        }
        switch (ch) {
        case '@':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '_':
        case '`':
        case '{':
        case '|':
        case '}':
        case '~':
          break;
        }
      }
    } else {
      line += ch;
      if (write(outfd, visible ? &ch : &mask, 1) <= 0) {
        return false;
      }
    }
  }
  return true;
}

void Greeter::handle(int infd, int outfd) {
  //if (fork() == 0) {
  //  dup2(infd, 0);
  //  dup2(outfd, 1);
  //  execlp("/usr/bin/nano", "/usr/bin/nano", NULL);
  //}
  //while (true) {}
  //return;
  // Firstly we try to log in the RDP server without credentials
  client.reset(new RDPClient(argc, argv, stopSignal));
  if (client->init(nullptr, nullptr, nullptr, -1, -1) &&
      client->start() && client->waitConnect()) {
    return;
  }
  client.reset(nullptr);
  //FILE* infile = fdopen(infd, "rb");
  //FILE* outfile = fdopen(outfd, "wb");
  string strBanner = "\e[37;44;4;1m欢迎使用Vlab。请输入用户名及密码以登录系统。\r\n"
    "请注意为Linux或Windows系统的用户名密码而非学号或工号和密码！\r\n";
  string strUsername = "用户名：";
  string strPassword = "\r\n密码：";
  string strResolution = "\r\n分辨率：";
  string strInvisible = "*";
  string strWait = "\r\n登录中，请稍候…\r\n";
  string strFailed = "登录失败！请重试。\r\n";
  string strInvalidResolution = "您输入的分辨率无效！\r\n";
  write(outfd, strBanner.c_str(), strBanner.length());
  const int maxRetryTimes = 5;
  for (int i = 0; i < maxRetryTimes; ++i) {
    string domain, username, password, resolution;
    
    if (!readLine(infd, outfd, strUsername, username) || !readLine(infd, outfd, strPassword, password, false)) {
      return;
    }

    int width;
    int height;
    tie(width, height) = desktop.getRequestedDesktopSize();
    if (!(width > 0 && width < 10000 && height > 0 && height < 10000)) {
      auto &&geometry = desktop.getGeometry();
      width = geometry.width();
      height = geometry.height();
    }
    resolution = to_string(width) + "x" + to_string(height);

    if (!readLine(infd, outfd, strResolution + resolution, resolution)) {
      return;
    }

    if (write(outfd, strWait.c_str(), strWait.length()) <= 0) {
      return;
    }

    size_t pos = username.find('\\');
    if (pos != string::npos) {
      domain = username.substr(0, pos);
    }
    pos = resolution.find('x');
    if (pos != string::npos && pos != resolution.length() - 1) {
      try {
        width = stoi(resolution.substr(0, pos));
        height = stoi(resolution.substr(pos + 1));
      } catch (std::exception& e) {
        if (write(outfd, strInvalidResolution.c_str(), strInvalidResolution.length()) <= 0) {
          return;
        }
        width = -1;
        height = -1;
      }
    }
    char* domainCstr = domain.empty() ? nullptr : strdup(domain.c_str());
    char* usernameCstr = strdup(username.c_str());
    char* passwordCstr = strdup(password.c_str());
    client.reset(new RDPClient(argc, argv, stopSignal));
    if (!client->init(domainCstr, usernameCstr, passwordCstr, width, height) ||
        !client->start() || !client->waitConnect()) {
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