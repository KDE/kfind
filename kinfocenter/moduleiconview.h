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

#ifndef __moduleiconview_h__
#define __moduleiconview_h__

#include <k3listview.h>
//Added by qt3to4:
#include <QPixmap>
#include <QKeyEvent>

class ConfigModule;
class ConfigModuleList;

class ModuleIconItem : public K3ListViewItem
{

public:
  ModuleIconItem(Q3ListView *parent, const QString& text, const QPixmap& pm, ConfigModule *m = 0)
	: K3ListViewItem(parent, text)
	, _tag(QString())
	, _module(m)
	{
	  setPixmap(0, pm);
	}

  void setConfigModule(ConfigModule* m) { _module = m; }
  void setTag(const QString& t) { _tag = t; }
  void setOrderNo(int order)
  {
    QString s;
    setText(1, s.sprintf( "%02d", order ) );
  }

  ConfigModule* module() { return _module; }
  QString tag() { return _tag; }


private:
  QString       _tag;
  ConfigModule *_module;
};

class ModuleIconView : public K3ListView
{
  Q_OBJECT

public:
  explicit ModuleIconView(ConfigModuleList *list, QWidget * parent = 0);
  
  void makeSelected(ConfigModule* module);
  void makeVisible(ConfigModule *module);
  void fill();

Q_SIGNALS:
  void moduleSelected(ConfigModule*);

protected Q_SLOTS:
  void slotItemSelected(Q3ListViewItem*);

protected:
  void keyPressEvent(QKeyEvent *);
  QPixmap loadIcon( const QString &name );
  
private:
  QString           _path; 
  ConfigModuleList *_modules;

};



#endif
