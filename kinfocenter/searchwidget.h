/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2004 Daniel Molkentin <molkentin@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  along with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __searchwidget_h__
#define __searchwidget_h__

#include <QWidget>
#include <Qt3Support/Q3PtrList>

#include "modules.h"

class KListWidget;
class KLineEdit;
class QListWidgetItem;

class KeywordListEntry
{
 public:
  KeywordListEntry(const QString& name, ConfigModule* module);

  void addModule(ConfigModule* module);

  QString moduleName() { return _name; }
  Q3PtrList<ConfigModule> modules() { return _modules; }

 private:
  QString _name;
  Q3PtrList<ConfigModule> _modules;

};

class SearchWidget : public QWidget
{
  Q_OBJECT

public:
  SearchWidget(QWidget *parent);

  void populateKeywordList(ConfigModuleList *list);

Q_SIGNALS:
  void moduleSelected(ConfigModule *);

protected:
  void populateKeyListBox(const QString& regexp);
  void populateResultListBox(const QString& keyword);

protected Q_SLOTS:
  void slotSearchTextChanged(const QString &);
  void slotKeywordSelected(const QString &);
  void slotModuleSelected(QListWidgetItem *item);
  void slotModuleClicked(QListWidgetItem *item);

private:
  KListWidget  *_keyList, *_resultList;
  KLineEdit *_input;
  Q3PtrList<KeywordListEntry> _keywords;
};

#endif
