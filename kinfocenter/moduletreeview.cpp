/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 1999 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

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

#include "moduletreeview.h"
#include "global.h"
#include "modules.h"

#include <KApplication>
#include <klocale.h>
#include <kiconloader.h>
#include <kservicegroup.h>
#include <kdebug.h>

#include <Qt3Support/Q3Header>
#include <Qt3Support/Q3WhatsThis>
#include <QImage>
#include <QPainter>
#include <QBitmap>
#include <QPixmap>
#include <Q3PtrList>
#include <QKeyEvent>

#include "moduletreeview.moc"

static QPixmap appIcon(const QString &iconName)
{
     QString path;
     QPixmap normal = KIconLoader::global()->loadIcon(iconName, KIconLoader::Small, 0, KIconLoader::DefaultState, QStringList(), &path, true);
     // make sure they are not larger than KIconLoader::SizeSmall
     if (normal.width() > KIconLoader::SizeSmall || normal.height() > KIconLoader::SizeSmall)
     {
         QImage tmp = normal.toImage();
         tmp = tmp.scaled(KIconLoader::SizeSmall, KIconLoader::SizeSmall, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
         normal = QPixmap::fromImage(tmp);
     }
     return normal;
}

class ModuleTreeWhatsThis : public Q3WhatsThis
{
public:
    ModuleTreeWhatsThis( ModuleTreeView* tree)
        : Q3WhatsThis( tree ), treeView( tree ) {}
    ~ModuleTreeWhatsThis(){}


    QString text( const QPoint & p) {
        ModuleTreeItem* i = (ModuleTreeItem*)  treeView->itemAt( p );
        if ( i && i->module() )  {
            return i->module()->comment();
        } else if ( i ) {
            return i18n("The %1 configuration group. Click to open it.", i->text(0) );
        }
        return i18n("This treeview displays all available control modules. Click on one of the modules to receive more detailed information.");
    }

private:
    ModuleTreeView* treeView;
};

ModuleTreeView::ModuleTreeView(ConfigModuleList *list, QWidget * parent)
  : K3ListView(parent)
  , _modules(list)
{
  addColumn(QString());
  setColumnWidthMode (0, Q3ListView::Maximum);
  setAllColumnsShowFocus(true);
  setResizeMode(Q3ListView::AllColumns);
  setRootIsDecorated(true);
  setHScrollBarMode(AlwaysOff);
  header()->hide();

  new ModuleTreeWhatsThis( this );

  connect(this, SIGNAL(clicked(Q3ListViewItem*)),
                  this, SLOT(slotItemSelected(Q3ListViewItem*)));
}

void ModuleTreeView::fill()
{
  clear();

  QStringList subMenus = _modules->submenus(KCGlobal::baseGroup());
  for(QStringList::ConstIterator it = subMenus.begin();
      it != subMenus.end(); ++it)
  {
     QString path = *it;
     ModuleTreeItem*  menu = new ModuleTreeItem(this);
     menu->setGroup(path);
     fill(menu, path);
  }

  ConfigModule *module;
  Q3PtrList<ConfigModule> moduleList = _modules->modules(KCGlobal::baseGroup());
  for (module=moduleList.first(); module != 0; module=moduleList.next())
  {
     new ModuleTreeItem(this, module);
  }
}

void ModuleTreeView::fill(ModuleTreeItem *parent, const QString &parentPath)
{
  QStringList subMenus = _modules->submenus(parentPath);
  for(QStringList::ConstIterator it = subMenus.begin();
      it != subMenus.end(); ++it)
  {
     QString path = *it;
     ModuleTreeItem*  menu = new ModuleTreeItem(parent);
     menu->setGroup(path);
     fill(menu, path);
  }

  ConfigModule *module;
  Q3PtrList<ConfigModule> moduleList = _modules->modules(parentPath);
  for (module=moduleList.first(); module != 0; module=moduleList.next())
  {
     new ModuleTreeItem(parent, module);
  }
}



QSize ModuleTreeView::sizeHint() const
{
    return Q3ListView::sizeHint().boundedTo(
	QSize( fontMetrics().maxWidth()*35, QWIDGETSIZE_MAX) );
}

void ModuleTreeView::makeSelected(ConfigModule *module)
{
  ModuleTreeItem *item = static_cast<ModuleTreeItem*>(firstChild());

  updateItem(item, module);
}

void ModuleTreeView::updateItem(ModuleTreeItem *item, ConfigModule *module)
{
  while (item)
    {
          if (item->childCount() != 0)
                updateItem(static_cast<ModuleTreeItem*>(item->firstChild()), module);
          if (item->module() == module)
                {
                  setSelected(item, true);
                  break;
                }
          item = static_cast<ModuleTreeItem*>(item->nextSibling());
    }
}

/*
void ModuleTreeView::expandItem(QListViewItem *item, QPtrList<QListViewItem> *parentList)
{
  while (item)
    {
      setOpen(item, parentList->contains(item));

          if (item->childCount() != 0)
                expandItem(item->firstChild(), parentList);
      item = item->nextSibling();
    }
}
*/
void ModuleTreeView::makeVisible(ConfigModule *module)
{
  QString path = _modules->findModule(module);
  if (path.startsWith(KCGlobal::baseGroup()))
     path = path.mid(KCGlobal::baseGroup().length());

  QStringList groups = path.split( '/');

  ModuleTreeItem *item = 0;
  QStringList::ConstIterator it;
  for (it=groups.begin(); it != groups.end(); ++it)
  {
     if (item)
        item = static_cast<ModuleTreeItem*>(item->firstChild());
     else
        item = static_cast<ModuleTreeItem*>(firstChild());

     while (item)
     {
        if (item->tag() == *it)
        {
           setOpen(item, true);
           break;
        }
        item = static_cast<ModuleTreeItem*>(item->nextSibling());
     }
     if (!item)
        break; // Not found (?)
  }

  // make the item visible
  if (item)
    ensureItemVisible(item);
}

void ModuleTreeView::slotItemSelected(Q3ListViewItem* item)
{
  if (!item) return;

  if (static_cast<ModuleTreeItem*>(item)->module())
    {
      emit moduleSelected(static_cast<ModuleTreeItem*>(item)->module());
      return;
    }
  else
    {
      emit categorySelected(item);
    }

  setOpen(item, !item->isOpen());

  /*
  else
    {
      QPtrList<QListViewItem> parents;

      QListViewItem* i = item;
      while(i)
        {
          parents.append(i);
          i = i->parent();
        }

      //int oy1 = item->itemPos();
      //int oy2 = mapFromGlobal(QCursor::pos()).y();
      //int offset = oy2 - oy1;

      expandItem(firstChild(), &parents);

      //int x =mapFromGlobal(QCursor::pos()).x();
      //int y = item->itemPos() + offset;
      //QCursor::setPos(mapToGlobal(QPoint(x, y)));
    }
  */
}

void ModuleTreeView::keyPressEvent(QKeyEvent *e)
{
  if (!currentItem()) return;

  if(e->key() == Qt::Key_Return
     || e->key() == Qt::Key_Enter
        || e->key() == Qt::Key_Space)
    {
      //QCursor::setPos(mapToGlobal(QPoint(10, currentItem()->itemPos()+5)));
      slotItemSelected(currentItem());
    }
  else
    K3ListView::keyPressEvent(e);
}


ModuleTreeItem::ModuleTreeItem(Q3ListViewItem *parent, ConfigModule *module)
  : Q3ListViewItem(parent)
  , _module(module)
  , _tag(QString())
  , _maxChildIconWidth(0)
{
  if (_module)
        {
          setText(0, ' ' + module->moduleName());
          setPixmap(0, appIcon(module->icon()));
        }
}

ModuleTreeItem::ModuleTreeItem(Q3ListView *parent, ConfigModule *module)
  : Q3ListViewItem(parent)
  , _module(module)
  , _tag(QString())
  , _maxChildIconWidth(0)
{
  if (_module)
        {
          setText(0, ' ' + module->moduleName());
          setPixmap(0, appIcon(module->icon()));
        }
}

ModuleTreeItem::ModuleTreeItem(Q3ListViewItem *parent, const QString& text)
  : Q3ListViewItem(parent, ' ' + text)
  , _module(0)
  , _tag(QString())
  , _maxChildIconWidth(0)
  {}

ModuleTreeItem::ModuleTreeItem(Q3ListView *parent, const QString& text)
  : Q3ListViewItem(parent, ' ' + text)
  , _module(0)
  , _tag(QString())
  , _maxChildIconWidth(0)
  {}

void ModuleTreeItem::setPixmap(int column, const QPixmap& pm)
{
  if (!pm.isNull())
  {
    ModuleTreeItem* p = dynamic_cast<ModuleTreeItem*>(parent());
    if (p)
      p->regChildIconWidth(pm.width());
  }

  Q3ListViewItem::setPixmap(column, pm);
}

void ModuleTreeItem::regChildIconWidth(int width)
{
  if (width > _maxChildIconWidth)
    _maxChildIconWidth = width;
}

void ModuleTreeItem::paintCell( QPainter * p, const QColorGroup & cg, int column, int width, int align )
{
  if (!pixmap(0))
  {
    int offset = 0;
    ModuleTreeItem* parentItem = dynamic_cast<ModuleTreeItem*>(parent());
    if (parentItem)
    {
      offset = parentItem->maxChildIconWidth();
    }

    if (offset > 0)
    {
      QPixmap pixmap(offset, offset);
      pixmap.fill(Qt::color0);
      pixmap.setMask(pixmap.createHeuristicMask());
      QBitmap mask( pixmap.size(), true );
      pixmap.setMask( mask );
      Q3ListViewItem::setPixmap(0, pixmap);
    }
  }

  Q3ListViewItem::paintCell( p, cg, column, width, align );
}


void ModuleTreeItem::setGroup(const QString &path)
{
  KServiceGroup::Ptr group = KServiceGroup::group(path);
  QString defName = path.left(path.length()-1);
  int pos = defName.lastIndexOf('/');
  if (pos >= 0)
     defName = defName.mid(pos+1);
  if (group && group->isValid())
  {
     setPixmap(0, appIcon(group->icon()));
     setText(0, ' ' + group->caption());
     setTag(defName);
     setCaption(group->caption());
  }
  else
  {
     // Should not happen: Installation problem
     // Let's try to fail softly.
     setText(0, ' ' + defName);
     setTag(defName);
  }
}
