/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

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

#ifndef MODULES_H
#define MODULES_H

#include <kcmoduleinfo.h>
#include <QObject>
#include <Qt3Support/Q3Dict>
//Added by qt3to4:
#include <QVBoxLayout>
#include <Q3PtrList>
#include <kvbox.h>

template<class ConfigModule> class Q3PtrList;
class QStringList;
class KAboutData;
class KCModule;
class ProxyWidget;
class K3Process;
class QX11EmbedWidget;
class QVBoxLayout;
class KVBox;

class ConfigModule : public QObject, public KCModuleInfo
{
  Q_OBJECT

public:

  ConfigModule(const KService::Ptr &s);
  ~ConfigModule();

  bool isChanged() { return _changed; }
  void setChanged(bool changed) { _changed = changed; }

  bool isActive() { return _module != 0; }
  ProxyWidget *module();
  const KAboutData *aboutData() const;


public Q_SLOTS:

  void deleteClient();


private Q_SLOTS:

  void clientClosed();
  void clientChanged(bool state);
  void runAsRoot();
  void rootExited(K3Process *proc);


Q_SIGNALS:

  void changed(ConfigModule *module);
  void childClosed();
  void helpRequest();


private:

  bool         _changed;
  ProxyWidget *_module;
  QX11EmbedWidget  *_embedWidget;
  K3Process    *_rootProcess;
  QVBoxLayout *_embedLayout;
  KVBox       *_embedFrame;

};

class ConfigModuleList : public Q3PtrList<ConfigModule>
{
public:

  ConfigModuleList();

  void readDesktopEntries();
  bool readDesktopEntriesRecursive(const QString &path);

  /**
   * Returns all submenus of the submenu identified by path
   */
  Q3PtrList<ConfigModule> modules(const QString &path);

  /**
   * Returns all modules of the submenu identified by path
   */
  QStringList submenus(const QString &path);

  /**
   * Returns the path of the submenu the module is in
   */
  QString findModule(ConfigModule *module);

protected:

  class Menu
  {
  public:
    Q3PtrList<ConfigModule> modules;
    QStringList submenus;
  };

  Q3Dict<Menu> subMenus;
};

#endif
