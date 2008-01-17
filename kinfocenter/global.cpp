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

#include "global.h"

#include <kservicegroup.h>
#include <kdebug.h>
#include <kapplication.h>
#include <kuser.h>

#include <QObject>
#include <Qt3Support/Q3Accel>
#include <QByteArray>
#include <QWidget>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>

bool KCGlobal::_root = false;
bool KCGlobal::_infocenter = false;
QStringList KCGlobal::_types;
QString KCGlobal::_uname = "";
QString KCGlobal::_hname = "";
QString KCGlobal::_kdeversion = "";
QString KCGlobal::_isystem = "";
QString KCGlobal::_irelease = "";
QString KCGlobal::_iversion = "";
QString KCGlobal::_imachine = "";
IndexViewMode KCGlobal::_viewmode = Icon;
KIconLoader::StdSizes KCGlobal::_iconsize = KIconLoader::SizeMedium;
QString KCGlobal::_baseGroup = "";

void KCGlobal::init()
{
  char buf[256];
  buf[0] = '\0';
  if (!gethostname(buf, sizeof(buf)))
    buf[sizeof(buf)-1] ='\0';
  QString hostname(buf);

  setHostName(hostname);
  setUserName(KUser().loginName());
  setRoot(getuid() == 0);

  setKDEVersion(KDE::versionString());

  struct utsname info;
  uname(&info);

  setSystemName(info.sysname);
  setSystemRelease(info.release);
  setSystemVersion(info.version);
  setSystemMachine(info.machine);
}

void KCGlobal::setType(const QByteArray& s)
{
  QString string = s.toLower();
  _types = string.split( ',');
}

QString KCGlobal::baseGroup()
{
  if ( _baseGroup.isEmpty() )
  {
    KServiceGroup::Ptr group = KServiceGroup::baseGroup( _infocenter ? "info" : "settings" );
    if (group)
    {
      _baseGroup = group->relPath();
      kDebug(1208) << "Found basegroup = " << _baseGroup;
      return _baseGroup;
    }
    // Compatibility with old behaviour, in case of missing .directory files.
    if (_baseGroup.isEmpty())
    {
      if (_infocenter)
      {
        kWarning() << "No K menu group with X-KDE-BaseGroup=info found ! Defaulting to Settings/Information/" ;
        _baseGroup = QLatin1String("Settings/Information/");
      }
      else
      {
        kWarning() << "No K menu group with X-KDE-BaseGroup=settings found ! Defaulting to Settings/" ;
        _baseGroup = QLatin1String("Settings/");
      }
    }
  }
  return _baseGroup;
}

void KCGlobal::repairAccels( QWidget * tw )
{
  const QList<QObject*> &l = tw->children();
  for (QList<QObject*>::const_iterator it( l.begin() ); it != l.end(); ++it)
  {
    QObject *obj = *it;

    if (qobject_cast<Q3Accel*>(obj))
      qobject_cast<Q3Accel*>(obj)->repairEventFilter();
  }
}
