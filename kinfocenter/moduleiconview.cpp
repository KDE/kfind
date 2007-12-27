/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2003 Daniel Molkentin <molkentin@kde.org>

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

#include "moduleiconview.h"
#include "modules.h"
#include "global.h"

#include <Qt3Support/Q3Header>
#include <QPixmap>
#include <QKeyEvent>
#include <Q3PtrList>
#include <QApplication>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kservicegroup.h>
#include <kiconloader.h>
#include <kdebug.h>

#include "moduleiconview.moc"

ModuleIconView::ModuleIconView(ConfigModuleList *list, QWidget * parent)
  : K3ListView(parent)
  , _path(KCGlobal::baseGroup())
  , _modules(list)
{
  setSorting(1, true);
  addColumn(QString());

  // Needed to enforce a cut of the items label rather than
  // showing a horizontal scrollbar
  setResizeMode(LastColumn);

  header()->hide();

  // This is intentionally _not_ connected with executed(), since
  // honoring doubleclick doesn't make any sense here (changed by
  // large user demand)
  connect(this, SIGNAL(clicked(Q3ListViewItem*)),
          this, SLOT(slotItemSelected(Q3ListViewItem*)));
}

void ModuleIconView::makeSelected(ConfigModule *m)
{
  if (!m) return;

  for (Q3ListViewItem *i = firstChild(); i; i = i->nextSibling())
  {
     if(static_cast<ModuleIconItem*>(i)->module() == m)
     {
        setSelected(i, true);
        break;
     }
  }
}

void ModuleIconView::makeVisible(ConfigModule *m)
{
  if (!m) return;
  QString tmp = _modules->findModule(m);
  if (tmp.isEmpty())
     return;

  _path = tmp;
  fill();
}

void ModuleIconView::fill()
{
  clear();

  QPixmap icon;
  // add our "up" icon if we aren't top level
  if (_path != KCGlobal::baseGroup())
  {
     icon = loadIcon( "go-previous" );
     // go-back node
     ModuleIconItem *i = new ModuleIconItem(this, i18nc("go back", "Back"), icon);
     i->setOrderNo(0);
     int last_slash = _path.lastIndexOf('/', -2);
     if (last_slash == -1)
        i->setTag(QString());
     else
        i->setTag(_path.left(last_slash+1));
  }

  int c = 0;
  QStringList submenus = _modules->submenus(_path);
  for (QStringList::Iterator it = submenus.begin(); it != submenus.end(); ++it )
  {
     QString path = (*it);

     KServiceGroup::Ptr group = KServiceGroup::group(path);
     if (!group || !group->isValid())
        continue;

     icon = loadIcon( group->icon() );

     ModuleIconItem *i = new ModuleIconItem(this, group->caption(), icon);
     i->setTag(path);
     i->setOrderNo(++c);
  }

  c = 0;
  Q3PtrList<ConfigModule> moduleList = _modules->modules(_path);
  for (ConfigModule *module=moduleList.first(); module != 0; module=moduleList.next())
  {
     icon = loadIcon( module->icon() );

     ModuleIconItem *i = new ModuleIconItem(this, module->moduleName(), icon, module);
     i->setOrderNo(++c);
  }
}

void ModuleIconView::slotItemSelected(Q3ListViewItem* item)
{
  QApplication::restoreOverrideCursor();
  if (!item) return;

  if (static_cast<ModuleIconItem*>(item)->module())
  {
     emit moduleSelected(static_cast<ModuleIconItem*>(item)->module());
  }
  else
  {
     _path = static_cast<ModuleIconItem*>(item)->tag();
     fill();
     setCurrentItem(firstChild());
  }
}

void ModuleIconView::keyPressEvent(QKeyEvent *e)
{
  if(   e->key() == Qt::Key_Return
     || e->key() == Qt::Key_Enter
     || e->key() == Qt::Key_Space)
  {
     if (currentItem())
        slotItemSelected(currentItem());
  }
  else
  {
     K3ListView::keyPressEvent(e);
  }
}

QPixmap ModuleIconView::loadIcon( const QString &name )
{
  QPixmap icon = DesktopIcon( name, KCGlobal::iconSize() );

  if(icon.isNull())
     icon = DesktopIcon( "folder", KCGlobal::iconSize() );

  return icon;
}
