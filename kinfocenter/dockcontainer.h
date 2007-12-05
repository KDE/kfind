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

#ifndef __dockcontainer_h__
#define __dockcontainer_h__

#include <QStackedWidget>
#include <QLabel>
#include <kvbox.h>

class ConfigModule;
class ModuleWidget;
class ProxyWidget;

class DockContainer : public QStackedWidget
{
  Q_OBJECT

public:
  DockContainer(QWidget *parent=0);
  virtual ~DockContainer();

  void setBaseWidget(QWidget *widget);
  QWidget *baseWidget() { return _basew; }

  bool dockModule(ConfigModule *module);
  ConfigModule *module() { return _module; }

public Q_SLOTS:
  void removeModule();

protected Q_SLOTS:
  void quickHelpChanged();

protected:
  void deleteModule();
  ProxyWidget* loadModule( ConfigModule *module );

Q_SIGNALS:
  void newModule(const QString &name, const QString& docPath, const QString &quickhelp);
  void changedModule(ConfigModule *module);

private:
  QWidget      *_basew;
  QLabel       *_busyw;
  ModuleWidget *_modulew;
  ConfigModule *_module;

};

#endif
