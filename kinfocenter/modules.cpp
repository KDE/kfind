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

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "modules.h"
#include "global.h"
#include "proxywidget.h"
#include "kcrootonly.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kservicegroup.h>
#include <k3process.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kauthorized.h>
#include <kservicetypetrader.h>
#include <kcmoduleloader.h>
#include <kvbox.h>

#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <Q3PtrList>

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#include <QX11Info>
#include <QX11EmbedWidget>
#endif
#include <unistd.h>
#include <sys/types.h>

#include "modules.moc"

template class Q3PtrList<ConfigModule>;


ConfigModule::ConfigModule(const KService::Ptr &s)
  : KCModuleInfo(s), _changed(false), _module(0), _embedWidget(0),
    _rootProcess(0), _embedLayout(0), _embedFrame(0)
{
}

ConfigModule::~ConfigModule()
{
  deleteClient();
}

ProxyWidget *ConfigModule::module()
{
  if (_module)
    return _module;
#ifdef __GNUC__
#warning I expect KControl in its current form to disappear, if not a real port is needed
#endif
  // root KCMs are gone, clean up all of the related code
  bool run_as_root = false; //needsRootPrivileges() && (getuid() != 0);

  KCModule *modWidget = 0;

  modWidget = KCModuleLoader::loadModule(*this,/*KCModuleLoader::None*/(KCModuleLoader::ErrorReporting)0);

  if (modWidget)
    {

      _module = new ProxyWidget(modWidget, moduleName(), run_as_root);
      connect(_module, SIGNAL(changed(bool)), this, SLOT(clientChanged(bool)));
      connect(_module, SIGNAL(closed()), this, SLOT(clientClosed()));
      connect(_module, SIGNAL(helpRequest()), this, SIGNAL(helpRequest()));
      connect(_module, SIGNAL(runAsRoot()), this, SLOT(runAsRoot()));

      return _module;
    }

  return 0;
}

void ConfigModule::deleteClient()
{
#ifdef Q_WS_X11
  if (_embedWidget)
    XKillClient(QX11Info::display(), _embedWidget->containerWinId());
#endif
  delete _rootProcess;
  _rootProcess = 0;

  delete _embedWidget;
  _embedWidget = 0;
  delete _embedFrame;
  _embedFrame = 0;
  kapp->syncX();

  if(_module)
    _module->close(true);
  _module = 0;

  delete _embedLayout;
  _embedLayout = 0;

  _changed = false;
}

void ConfigModule::clientClosed()
{
  deleteClient();

  emit changed(this);
  emit childClosed();
}


void ConfigModule::clientChanged(bool state)
{
  setChanged(state);
  emit changed(this);
}


void ConfigModule::runAsRoot()
{
  if (!_module)
    return;

  delete _rootProcess;
  delete _embedWidget;
  delete _embedLayout;

  // create an embed widget that will embed the
  // kcmshell4 running as root
  _embedLayout = new QVBoxLayout(_module->parentWidget());
  _embedFrame = new KVBox( _module->parentWidget() );
#ifdef __GNUC__
#warning "KDE4 porting ";
#endif
  //_embedFrame->setFrameStyle( QFrame::Box | QFrame::Raised );
  QPalette pal( Qt::red );
  pal.setColor( QPalette::Background,
		_module->parentWidget()->palette().color( QPalette::Background ) );
#ifdef __GNUC__
#warning "KDE4 porting"
#endif
  //_embedFrame->setPalette( pal );
  //_embedFrame->setLineWidth( 2 );
  //_embedFrame->setMidLineWidth( 2 );
  _embedLayout->addWidget(_embedFrame,1);
  _embedWidget = new QX11EmbedWidget(_embedFrame );

  _module->hide();
  _embedFrame->show();
  QLabel *_busy = new QLabel(i18n("<big>Loading...</big>"), _embedWidget);
  _busy->setAlignment(Qt::AlignCenter);
  _busy->setTextFormat(Qt::RichText);
  _busy->setGeometry(0,0, _module->width(), _module->height());
  _busy->show();

  // prepare the process to run the kcmshell4
  QString cmd = service()->exec().trimmed();
  bool kdeshell = false;
  if (cmd.left(5) == "kdesu")
    {
      cmd = cmd.remove(0,5).trimmed();
      // remove all kdesu switches
      while( cmd.length() > 1 && cmd[ 0 ] == '-' )
        {
          int pos = cmd.indexOf( ' ' );
          cmd = cmd.remove( 0, pos ).trimmed();
        }
    }

  if (cmd.left(8) == "kcmshell4")
    {
      cmd = cmd.remove(0,8).trimmed();
      kdeshell = true;
    }

  // run the process
  QString kdesu = KStandardDirs::findExe("kdesu");
  if (!kdesu.isEmpty())
    {
      _rootProcess = new K3Process;
      *_rootProcess << kdesu;
#ifdef __GNUC__
#warning "--nonewdcop doesn't exist now";
#endif
      //*_rootProcess << "--nonewdcop";
      // We have to disable the keep-password feature because
      // in that case the modules is started through kdesud and kdesu
      // returns before the module is running and that doesn't work.
      // We also don't have a way to close the module in that case.
      *_rootProcess << "--n"; // Don't keep password.
      if (kdeshell) {
         *_rootProcess << QString("kcmshell4 %1 --embed %2 --lang %3").arg(cmd).arg(_embedWidget->winId()).arg(KGlobal::locale()->language());
      }
      else {
         *_rootProcess << QString("%1 --embed %2 --lang %3").arg(cmd).arg(_embedWidget->winId()).arg( KGlobal::locale()->language() );
      }

      connect(_rootProcess, SIGNAL(processExited(K3Process*)), this, SLOT(rootExited(K3Process*)));

      if ( !_rootProcess->start(K3Process::NotifyOnExit) )
      {
          delete _rootProcess;
          _rootProcess = 0L;
      }

      return;
    }

  // clean up in case of failure
  delete _embedFrame;
  _embedWidget = 0;
  delete _embedLayout;
  _embedLayout = 0;
  _module->show();
}


void ConfigModule::rootExited(K3Process *)
{

  if (_embedWidget->containerWinId())
    XDestroyWindow(QX11Info::display(), _embedWidget->containerWinId());

  delete _embedWidget;
  _embedWidget = 0;

  delete _rootProcess;
  _rootProcess = 0;

  delete _embedLayout;
  _embedLayout = 0;

  delete _module;
  _module=0;

  _changed = false;
  emit changed(this);
  emit childClosed();
}

const KAboutData *ConfigModule::aboutData() const
{
  if (!_module) return 0;
  return _module->aboutData();
}


ConfigModuleList::ConfigModuleList()
{
  setAutoDelete(true);
  subMenus.setAutoDelete(true);
}

void ConfigModuleList::readDesktopEntries()
{
  readDesktopEntriesRecursive( KCGlobal::baseGroup() );
}

bool ConfigModuleList::readDesktopEntriesRecursive(const QString &path)
{
  KService::List list = KServiceTypeTrader::self()->query( "KCModule", "[X-KDE-ParentApp] == 'kinfocenter'" );

  if( list.isEmpty() )
	  return false;

  Menu *menu = new Menu;
  subMenus.insert(path, menu);

  foreach(const KService::Ptr &s, list)
  {
     if (s->isType(KST_KService))
     {
        if (!KAuthorized::authorizeControlModule(s->menuId()))
           continue;

        ConfigModule *module = new ConfigModule(s);
        if (module->library().isEmpty())
        {
           delete module;
           continue;
        }

        append(module);
        menu->modules.append(module);
     }
     else if (s->isType(KST_KServiceGroup) &&
		     readDesktopEntriesRecursive(s->entryPath()) )
        	menu->submenus.append(s->entryPath());

  }
  return true;
}

Q3PtrList<ConfigModule> ConfigModuleList::modules(const QString &path)
{
  Menu *menu = subMenus.find(path);
  if (menu)
     return menu->modules;

  return Q3PtrList<ConfigModule>();
}

QStringList ConfigModuleList::submenus(const QString &path)
{
  Menu *menu = subMenus.find(path);
  if (menu)
     return menu->submenus;

  return QStringList();
}

QString ConfigModuleList::findModule(ConfigModule *module)
{
  Q3DictIterator<Menu> it(subMenus);
  Menu *menu;
  for(;(menu = it.current());++it)
  {
     if (menu->modules.containsRef(module))
        return it.currentKey();
  }
  return QString();
}
