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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
  MA  02110-1301, USA.
*/

#include <kaboutapplicationdialog.h>
#include <KApplication>
#include <kactioncollection.h>
#include <kbugreport.h>
#include <kaboutdata.h>
#include <kconfig.h>
#include <kcomponentdata.h>
#include <kicon.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kstandardaction.h>
#include <ktoggleaction.h>
#include <kwindowsystem.h>
#include <kxmlguifactory.h>
#include <QSplitter>

#include <QTabWidget>


#include "indexwidget.h"
#include "searchwidget.h"
#include "helpwidget.h"
#include "aboutwidget.h"
#include "proxywidget.h"
#include "moduletreeview.h"
#include <stdio.h>

#include "toplevel.h"
#include "toplevel.moc"

TopLevel::TopLevel()
  : KXmlGuiWindow( 0, Qt::WStyle_ContextHelp  )
  , _active(0), dummyAbout(0)
{
  setCaption(QString());

  report_bug = 0;

  // read settings
  KConfigGroup config(KGlobal::config(), "Index");
  QString viewmode = config.readEntry("ViewMode", "Tree");

  if (viewmode == "Tree")
    KCGlobal::setViewMode(Tree);
  else
    KCGlobal::setViewMode(Icon);

  QString size = config.readEntry("IconSize", "Medium");
  if (size == "Small")
    KCGlobal::setIconSize(KIconLoader::SizeSmall);
  else if (size == "Large")
    KCGlobal::setIconSize(KIconLoader::SizeLarge);
  else if (size == "Huge")
    KCGlobal::setIconSize(KIconLoader::SizeHuge);
  else
    KCGlobal::setIconSize(KIconLoader::SizeMedium);

  // initialize the entries
  _modules = new ConfigModuleList();
  _modules->readDesktopEntries();

  for ( ConfigModule* m = _modules->first(); m; m = _modules->next() )
      connect( m, SIGNAL( helpRequest() ), this, SLOT( slotHelpRequest() ) );

  // create the layout box
  _splitter = new QSplitter( Qt::Horizontal, this );

  // create the left hand side (the tab view)
  _tab = new QTabWidget( _splitter );

  _tab->setWhatsThis( i18n("Choose between Index, Search and Quick Help") );

  // index tab
  _indextab = new IndexWidget(_modules, 0L);
  connect(_indextab, SIGNAL(moduleActivated(ConfigModule*)),
                  this, SLOT(activateModule(ConfigModule*)));
  _tab->addTab(_indextab, KIcon("kinfocenter"), i18n("&Index"));

  connect(_indextab, SIGNAL(categorySelected(Q3ListViewItem*)),
                  this, SLOT(categorySelected(Q3ListViewItem*)));

  // search tab
  _searchtab = new SearchWidget(0L);
  _searchtab->populateKeywordList(_modules);
  connect(_searchtab, SIGNAL(moduleSelected(ConfigModule *)),
                  this, SLOT(activateModule(ConfigModule *)));

  _tab->addTab(_searchtab, KIcon("edit-find"), i18n("Sear&ch"));

  // help tab
  _helptab = new HelpWidget(0L);
  _tab->addTab(_helptab, KIcon("help-contents"), i18n("Hel&p"));

  _tab->setSizePolicy( QSizePolicy( QSizePolicy::Maximum, QSizePolicy::Preferred ) );

 // Restore sizes
  config = KConfigGroup(KGlobal::config(),"General");
  QList<int> sizes = config.readEntry(  "SplitterSizes",QList<int>() );
  if (!sizes.isEmpty())
     _splitter->setSizes(sizes);

  // set up the right hand side (the docking area)
  _dock = new DockContainer( _splitter );

  // That one does the trick ...
  _splitter->setResizeMode( _tab, QSplitter::KeepSize );

  connect(_dock, SIGNAL(newModule(const QString&, const QString&, const QString&)),
                  this, SLOT(newModule(const QString&, const QString&, const QString&)));
  connect(_dock, SIGNAL(changedModule(ConfigModule*)),
          SLOT(changedModule(ConfigModule*)));

  // set the main view
  setCentralWidget( _splitter );

  // initialize the GUI actions
  setupActions();

  // activate defaults
  if (KCGlobal::viewMode() == Tree)   {
    activateTreeView();
    tree_view->setChecked(true);
  }
  else {
    activateIconView();
    icon_view->setChecked(true);
  }

  // insert the about widget
  if (KCGlobal::isInfoCenter())
  {
      AboutWidget *aw = new AboutWidget( this, _indextab->firstTreeViewItem());
      connect( aw, SIGNAL( moduleSelected( ConfigModule * ) ),
               SLOT( activateModule( ConfigModule * ) ) );
      _dock->setBaseWidget( aw );
      KWindowSystem::setIcons(  winId(),
		       KIconLoader::global()->loadIcon("hwinfo", KIconLoader::NoGroup, 32 ),
		       KIconLoader::global()->loadIcon("hwinfo", KIconLoader::NoGroup, 16 ) );
  }
  else
  {
      AboutWidget *aw = new AboutWidget(this);
      connect( aw, SIGNAL( moduleSelected( ConfigModule * ) ),
                   SLOT( activateModule( ConfigModule * ) ) );
      _dock->setBaseWidget(aw);
  }
}

TopLevel::~TopLevel()
{
  KConfigGroup config(KGlobal::config(), "Index");
  if (KCGlobal::viewMode() == Tree)
    config.writeEntry("ViewMode", "Tree");
  else
    config.writeEntry("ViewMode", "Icon");

  switch (KCGlobal::iconSize())
    {
    case KIconLoader::SizeSmall:
      config.writeEntry("IconSize", "Small");
      break;
    case KIconLoader::SizeLarge:
      config.writeEntry("IconSize", "Large");
      break;
    case KIconLoader::SizeHuge:
      config.writeEntry("IconSize", "Huge");
      break;
    default:
      config.writeEntry("IconSize", "Medium");
      break;
    }

  config = KConfigGroup(KGlobal::config(),"General");
  config.writeEntry("SplitterSizes", _splitter->sizes());

  config.sync();

  delete _modules;
}

bool TopLevel::queryClose()
{
  return _dock->dockModule(0);
}

void TopLevel::setupActions()
{
  KStandardAction::quit(this, SLOT(close()), actionCollection());
  KStandardAction::keyBindings(guiFactory(), SLOT(configureShortcuts()),
                          actionCollection());

  QActionGroup* viewModeGroup = new QActionGroup(this);

  icon_view = new KToggleAction(i18n("&Icon View"), this);
  actionCollection()->addAction("activate_iconview", icon_view);
  connect(icon_view, SIGNAL(triggered(bool) ), SLOT(activateIconView()));
  icon_view->setActionGroup(viewModeGroup);

  tree_view = new KToggleAction(i18n("&Tree View"), this);
  actionCollection()->addAction("activate_treeview", tree_view);
  connect(tree_view, SIGNAL(triggered(bool) ), SLOT(activateTreeView()));
  tree_view->setActionGroup(viewModeGroup);

  QActionGroup* iconSizeGroup = new QActionGroup(this);

  icon_small = new KToggleAction(i18n("&Small"), this);
  actionCollection()->addAction("activate_smallicons", icon_small);
  connect(icon_small, SIGNAL(triggered(bool) ), SLOT(activateSmallIcons()));
  icon_small->setActionGroup(iconSizeGroup);

  icon_medium = new KToggleAction(i18n("&Medium"), this);
  actionCollection()->addAction("activate_mediumicons", icon_medium);
  connect(icon_medium, SIGNAL(triggered(bool) ), SLOT(activateMediumIcons()));
  icon_medium->setActionGroup(iconSizeGroup);

  icon_large = new KToggleAction(i18n("&Large"), this);
  actionCollection()->addAction("activate_largeicons", icon_large);
  connect(icon_large, SIGNAL(triggered(bool) ), SLOT(activateLargeIcons()));
  icon_large->setActionGroup(iconSizeGroup);

  icon_huge = new KToggleAction(i18n("&Huge"), this);
  actionCollection()->addAction("activate_hugeicons", icon_huge);
  connect(icon_huge, SIGNAL(triggered(bool) ), SLOT(activateHugeIcons()));
  icon_huge->setActionGroup(iconSizeGroup);

  about_module = actionCollection()->addAction("help_about_module");
  about_module->setText(i18n("About Current Module"));
  connect(about_module, SIGNAL(triggered(bool) ), SLOT(aboutModule()));
  about_module->setEnabled(false);

  createGUI("kinfocenterui.rc");

  report_bug = actionCollection()->action("help_report_bug");
  report_bug->setText(i18n("&Report Bug..."));
  report_bug->disconnect();
  connect(report_bug, SIGNAL(activated()), SLOT(reportBug()));
}

void TopLevel::activateIconView()
{
  KCGlobal::setViewMode(Icon);
  _indextab->activateView(Icon);

  icon_small->setEnabled(true);
  icon_medium->setEnabled(true);
  icon_large->setEnabled(true);
  icon_huge->setEnabled(true);

  switch(KCGlobal::iconSize())
    {
    case KIconLoader::SizeSmall:
      icon_small->setChecked(true);
      break;
    case KIconLoader::SizeLarge:
      icon_large->setChecked(true);
      break;
    case KIconLoader::SizeHuge:
      icon_huge->setChecked(true);
      break;
    default:
      icon_medium->setChecked(true);
      break;
    }
}

void TopLevel::activateTreeView()
{
  KCGlobal::setViewMode(Tree);
  _indextab->activateView(Tree);

  icon_small->setEnabled(false);
  icon_medium->setEnabled(false);
  icon_large->setEnabled(false);
  icon_huge->setEnabled(false);
}

void TopLevel::activateSmallIcons()
{
  KCGlobal::setIconSize(KIconLoader::SizeSmall);
  _indextab->reload();
}

void TopLevel::activateMediumIcons()
{
  KCGlobal::setIconSize(KIconLoader::SizeMedium);
  _indextab->reload();
}

void TopLevel::activateLargeIcons()
{
  KCGlobal::setIconSize(KIconLoader::SizeLarge);
  _indextab->reload();
}

void TopLevel::activateHugeIcons()
{
  KCGlobal::setIconSize(KIconLoader::SizeHuge);
  _indextab->reload();
}

void TopLevel::newModule(const QString &name, const QString& docPath, const QString &quickhelp)
{
    setCaption(name, false);

  _helptab->setText( docPath, quickhelp );

  if (!report_bug) return;

  if(name.isEmpty())
    report_bug->setText(i18n("&Report Bug..."));
  else
    report_bug->setText(i18n("Report Bug on Module %1...", handleAmpersand( name)));
}

void TopLevel::changedModule(ConfigModule *changed)
{
    if (!changed)
        return;
    setCaption(changed->moduleName(), changed->isChanged() );
}

void TopLevel::categorySelected(Q3ListViewItem *category)
{
  if (_active)
  {
    if (_active->isChanged())
      {
        int res = KMessageBox::warningYesNoCancel(this, _active ?
             i18n("There are unsaved changes in the active module.\n"
                  "Do you want to apply the changes before running "
                  "the new module or discard the changes?") :
             i18n("There are unsaved changes in the active module.\n"
                  "Do you want to apply the changes before exiting "
                  "the Control Center or discard the changes?"),
                            i18n("Unsaved Changes"),
                            KStandardGuiItem::apply(),
                            KStandardGuiItem::discard());
        if (res == KMessageBox::Yes)
          _active->module()->applyClicked();
        else if (res == KMessageBox::Cancel)
          return;
      }
  }
  _dock->removeModule();
  about_module->setText( i18n( "About Current Module" ) );
  about_module->setIcon( KIcon() );
  about_module->setEnabled( false );

  // insert the about widget
  Q3ListViewItem *firstItem = category->firstChild();
  QString caption = static_cast<ModuleTreeItem*>(category)->caption();
  if( _dock->baseWidget()->isA( "AboutWidget" ) )
  {
    static_cast<AboutWidget *>( _dock->baseWidget() )->setCategory( firstItem, caption);
  }
  else
  {
    AboutWidget *aw = new AboutWidget( this, firstItem, caption );
    connect( aw, SIGNAL( moduleSelected( ConfigModule * ) ),
             SLOT( activateModule( ConfigModule * ) ) );
    _dock->setBaseWidget( aw );
  }
}


void TopLevel::activateModule(ConfigModule *mod)
{
  if ( _dock->module() == mod )
     return;

  // tell the index to display the module
  _indextab->makeVisible(mod);

  // tell the index to mark this module as loaded
  _indextab->makeSelected(mod);

  // dock it
  if (!_dock->dockModule(mod))
  {
     if ( _dock->module() )
     {
       _indextab->makeVisible(_active);
       _indextab->makeSelected(_active);
     }
     return;
  }

  _active=mod;

  if (mod->aboutData())
  {
     about_module->setText(i18nc("Help menu->about <modulename>", "About %1",
                             handleAmpersand( mod->moduleName())));
     about_module->setIcon(KIcon(mod->icon()));
     about_module->setEnabled(true);
  }
  else
  {
     about_module->setText(i18n("About Current Module"));
     about_module->setIcon(KIcon());
     about_module->setEnabled(false);
  }
}

void TopLevel::deleteDummyAbout()
{
  delete dummyAbout;
  dummyAbout = 0;
}


void TopLevel::slotHelpRequest()
{
    _tab->showPage( _helptab );
}

void TopLevel::reportBug()
{
    // this assumes the user only opens one bug report at a time
    static char buffer[128];

    dummyAbout = 0;
    bool deleteit = false;

    if (!_active) // report against kinfocenter
        dummyAbout = const_cast<KAboutData*>(KGlobal::mainComponent().aboutData());
    else
    {
        if (_active->aboutData())
            dummyAbout = const_cast<KAboutData*>(_active->aboutData());
        else
        {
            snprintf(buffer, sizeof(buffer), "kcm%s", _active->library().toLatin1().constData());
            dummyAbout = new KAboutData(buffer, 0, ki18n(_active->moduleName().toUtf8()), "2.0");
            deleteit = true;
        }
    }
    KBugReport *br = new KBugReport(this, false, dummyAbout);
    if (deleteit)
        connect(br, SIGNAL(finished()), SLOT(deleteDummyAbout()));
    else
        dummyAbout = 0;
    br->show();
}

void TopLevel::aboutModule()
{
    KAboutApplicationDialog dlg(_active->aboutData());
    dlg.exec();
}

QString TopLevel::handleAmpersand( QString modulename ) const
{
   if( modulename.contains( '&' )) // double it
   {
      for( int i = modulename.length();
           i >= 0;
           --i )
         if( modulename[ i ] == '&' )
             modulename.insert( i, "&" );
   }
   return modulename;
}
