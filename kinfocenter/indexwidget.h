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

#ifndef __indexwidget_h__
#define __indexwidget_h__

#include <QStackedWidget>
//Added by qt3to4:
#include <QResizeEvent>

#include "global.h"

class ConfigModuleList;
class ConfigModule;
class ModuleTreeView;
class ModuleIconView;
class Q3ListViewItem;
class IndexWidget : public QStackedWidget
{
  Q_OBJECT
  
public:   
  IndexWidget(ConfigModuleList *list, QWidget *parent, const char *name=0);
  virtual ~IndexWidget();

  Q3ListViewItem *firstTreeViewItem();
public Q_SLOTS:
  void makeVisible(ConfigModule *module);
  void makeSelected(ConfigModule *module);
  void activateView(IndexViewMode);
  void reload();

protected Q_SLOTS:
  void moduleSelected(ConfigModule *);

Q_SIGNALS:
  void moduleActivated(ConfigModule *module);
  void categorySelected(Q3ListViewItem *);

private:
  ModuleTreeView   *_tree;
  ModuleIconView   *_icon;
  ConfigModuleList *_modules;
  IndexViewMode    viewMode;
};

#endif
