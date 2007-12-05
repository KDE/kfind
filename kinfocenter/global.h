/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#ifndef __global_h__
#define __global_h__

#include <kicontheme.h>



enum IndexViewMode {Icon, Tree};

class KCGlobal
{
public:

  static void init();

  static bool isInfoCenter() { return _infocenter; }
  static bool root() { return _root; }
  static QStringList types() { return _types; }
  static QString userName() { return _uname; }
  static QString hostName() { return _hname; }
  static QString kdeVersion() { return _kdeversion; }
  static QString systemName() { return _isystem; }
  static QString systemRelease() { return _irelease; }
  static QString systemVersion() { return _iversion; }
  static QString systemMachine() { return _imachine; }
  static IndexViewMode viewMode() { return _viewmode; }
  static KIconLoader::StdSizes iconSize() { return _iconsize; }
  static QString baseGroup();

  static void setIsInfoCenter(bool b) { _infocenter = b; }
  static void setRoot(bool r) { _root = r; }
  static void setType(const QByteArray& s);
  static void setUserName(const QString& n){ _uname = n; }
  static void setHostName(const QString& n){ _hname = n; }
  static void setKDEVersion(const QString& n){ _kdeversion = n; }
  static void setSystemName(const QString& n){ _isystem = n; }
  static void setSystemRelease(const QString& n){ _irelease = n; }
  static void setSystemVersion(const QString& n){ _iversion = n; }
  static void setSystemMachine(const QString& n){ _imachine = n; }
  static void setViewMode(IndexViewMode m) { _viewmode = m; }
  static void setIconSize(KIconLoader::StdSizes s) { _iconsize = s; }

  static void repairAccels( QWidget * tw );

private:
  static bool _root;
  static bool _infocenter;
  static QStringList _types;
  static QString _uname, _hname, _isystem, _irelease, _iversion, _imachine, _kdeversion;
  static IndexViewMode _viewmode;
  static KIconLoader::StdSizes _iconsize;
  static QString _baseGroup;
};

#endif
