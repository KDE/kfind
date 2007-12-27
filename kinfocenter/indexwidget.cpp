/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2003 Frauke Oster <frauke.oster@t-online.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "indexwidget.h"
#include "moduletreeview.h"
#include "moduleiconview.h"

#include <Qt3Support/Q3CheckListItem>

#include "indexwidget.moc"

IndexWidget::IndexWidget(ConfigModuleList *modules, QWidget *parent ,const char *)
  : QStackedWidget(parent)
  , _tree(0L)
  , _icon(0L)
  , _modules(modules)
  , viewMode(Icon)
{
  activateView(Icon);
}

IndexWidget::~IndexWidget() {}

void IndexWidget::reload()
{
  if (_icon)
    _icon->fill();
}

Q3ListViewItem *IndexWidget::firstTreeViewItem()
{
  if (_tree)
    return _tree->firstChild();
  else
	return 0L;
}

void IndexWidget::moduleSelected(ConfigModule *m)
{
  const QObject *obj = sender();
  if(!m) return;

  emit moduleActivated(m);

  if (obj->inherits("ModuleIconView") && _tree)
	{
	  _tree->makeVisible(m);

	  _tree->disconnect(SIGNAL(moduleSelected(ConfigModule*)));
	  _tree->makeSelected(m);
	  connect(_tree, SIGNAL(moduleSelected(ConfigModule*)),
			  this, SLOT(moduleSelected(ConfigModule*)));
	}
  else if (obj->inherits("ModuleTreeView") && _icon)
	{
	  _icon->makeVisible(m);

	  _icon->disconnect(SIGNAL(moduleSelected(ConfigModule*)));
	  _icon->makeSelected(m);
	  connect(_icon, SIGNAL(moduleSelected(ConfigModule*)),
			 this, SLOT(moduleSelected(ConfigModule*)));
	}
}

void IndexWidget::makeSelected(ConfigModule *module)
{
  if (_icon)
  {
   _icon->disconnect(SIGNAL(moduleSelected(ConfigModule*)));
   _icon->makeSelected(module);
   connect(_icon, SIGNAL(moduleSelected(ConfigModule*)),
		  this, SLOT(moduleSelected(ConfigModule*)));
  }
  if (_tree)
  {
    _tree->disconnect(SIGNAL(moduleSelected(ConfigModule*)));
    _tree->makeSelected(module);
    connect(_tree, SIGNAL(moduleSelected(ConfigModule*)),
		  this, SLOT(moduleSelected(ConfigModule*)));
  }
}

void IndexWidget::makeVisible(ConfigModule *module)
{
  if (_icon)
    _icon->makeVisible(module);
  if (_tree)
    _tree->makeVisible(module);
}

void IndexWidget::activateView(IndexViewMode mode)
{
  viewMode = mode;

  if (mode == Icon)
  {
    if (!_icon)
    {
      _icon=new ModuleIconView(_modules, this);
      addWidget (_icon);
      _icon->fill();
	  connect(_icon, SIGNAL(moduleSelected(ConfigModule*)),
		  this, SLOT(moduleSelected(ConfigModule*)));
    }
    setCurrentWidget( _icon );
  }
  else
  {
    if (!_tree)
    {
      _tree=new ModuleTreeView(_modules, this);
      addWidget (_tree);
      _tree->fill();
      connect(_tree, SIGNAL(moduleSelected(ConfigModule*)),
		  this, SLOT(moduleSelected(ConfigModule*)));
	  connect(_tree, SIGNAL(categorySelected(Q3ListViewItem*)),
		  this, SIGNAL(categorySelected(Q3ListViewItem*)));
    }
    setCurrentWidget( _tree );
  }
}
