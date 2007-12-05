/*
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
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

#ifndef __TOPLEVEL_H__
#define __TOPLEVEL_H__

#include <kxmlguiwindow.h>
#include <Qt3Support/Q3CheckListItem>


class QSplitter;
class QTabWidget;

class KToggleAction;
class QAction;

class DockContainer;
class IndexWidget;
class SearchWidget;
class HelpWidget;
class ConfigModule;
class ConfigModuleList;
class ModuleTitle;

class TopLevel : public KXmlGuiWindow
{
  Q_OBJECT

public:
  TopLevel();
  ~TopLevel();

protected:
  void setupActions();

protected Q_SLOTS:
  void activateModule(ConfigModule *);
  void categorySelected(Q3ListViewItem *category);
  void newModule(const QString &name, const QString& docPath, const QString &quickhelp);
  void activateIconView();
  void activateTreeView();

  void reportBug();
  void aboutModule();

  void activateSmallIcons();
  void activateMediumIcons();
  void activateLargeIcons();
  void activateHugeIcons();

  void deleteDummyAbout();

  void slotHelpRequest();

  void changedModule(ConfigModule *changed);

  bool queryClose();

private:

  QString handleAmpersand( QString ) const;

  QSplitter      *_splitter;
  QTabWidget     *_tab;
  DockContainer  *_dock;
  ModuleTitle    *_title;

  KToggleAction *tree_view, *icon_view;
  KToggleAction *icon_small, *icon_medium, *icon_large, *icon_huge;
  QAction *report_bug, *about_module;

  IndexWidget  *_indextab;
  SearchWidget *_searchtab;
  HelpWidget   *_helptab;

  ConfigModule     *_active;
  ConfigModuleList *_modules;

  /**
   * if someone wants to report a bug
   * against a module with no about data
   * we construct one for him
   **/
  KAboutData *dummyAbout;
};

#endif
