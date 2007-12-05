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

#include <QLabel>

#include <QPixmap>
#include <QFont>

#include <QApplication>

#include <kapplication.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <kvbox.h>

#include "dockcontainer.h"
#include "dockcontainer.moc"

#include "global.h"
#include "modules.h"
#include "proxywidget.h"

class ModuleTitle : public KHBox
{
  public:
    ModuleTitle( QWidget *parent, const char *name=0 );
    ~ModuleTitle() {}

    void showTitleFor( ConfigModule *module );
    void clear();

  protected:
    QLabel *m_icon;
    QLabel *m_name;
};

ModuleTitle::ModuleTitle( QWidget *parent, const char *name )
    : KHBox( parent )
{
  QWidget *spacer = new QWidget( this );
  spacer->setFixedWidth( KDialog::marginHint()-KDialog::spacingHint() );
  m_icon = new QLabel( this );
  m_name = new QLabel( this );

  QFont font = m_name->font();
  font.setPointSize( font.pointSize()+1 );
  font.setBold( true );
  m_name->setFont( font );

  setSpacing( KDialog::spacingHint() );
  if ( QApplication::isRightToLeft() )
  {
      spacer = new QWidget( this );
      setStretchFactor( spacer, 10 );
  }
  else
      setStretchFactor( m_name, 10 );
}

void ModuleTitle::showTitleFor( ConfigModule *config )
{
  if ( !config )
    return;

  this->setWhatsThis( config->comment() );
  KIconLoader *loader = KIconLoader::global();
  QPixmap icon = loader->loadIcon( config->icon(), KIconLoader::NoGroup, 22 );
  m_icon->setPixmap( icon );
  m_name->setText( config->moduleName() );

  show();
}

void ModuleTitle::clear()
{
  m_icon->setPixmap( QPixmap() );
  m_name->setText( QString() );
  kapp->processEvents();
}

class ModuleWidget : public QWidget
{
  public:
    ModuleWidget( QWidget *parent, const char *name );
    ~ModuleWidget() {}

    ProxyWidget* load( ConfigModule *module );

  protected:
    QVBoxLayout *m_layout;
    ModuleTitle *m_title;
};

ModuleWidget::ModuleWidget( QWidget *parent, const char * )
    : QWidget( parent )
    , m_layout (new QVBoxLayout (this))
    , m_title (new ModuleTitle (this))
{
  m_layout->addWidget (m_title);
}

ProxyWidget *ModuleWidget::load( ConfigModule *module )
{
  m_title->clear();
  ProxyWidget *proxy = module->module();

  if ( proxy )
  {
    proxy->setParent(this);
    m_layout->addWidget (proxy);
    proxy->show();
    proxy->setSizePolicy (QSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding));
    m_title->showTitleFor( module );
  }

  return proxy;
}

DockContainer::DockContainer(QWidget *parent)
  : QStackedWidget(parent)
  , _basew(0L)
  , _module(0L)
{
  _busyw = new QLabel(i18n("<big><b>Loading...</b></big>"), this);
  _busyw->setAlignment(Qt::AlignCenter);
  _busyw->setTextFormat(Qt::RichText);
  _busyw->setGeometry(0,0, width(), height());
  addWidget( _busyw );

  _modulew = new ModuleWidget( this, "_modulew" );
  addWidget( _modulew );
}

DockContainer::~DockContainer()
{
  deleteModule();
}

void DockContainer::setBaseWidget(QWidget *widget)
{
  removeWidget( _basew );
  delete _basew;
  _basew = 0;
  if (!widget) return;

  _basew = widget;

  addWidget( _basew );
  setCurrentWidget( _basew );

  emit newModule(widget->windowTitle(), "", "");
}

ProxyWidget* DockContainer::loadModule( ConfigModule *module )
{
  QApplication::setOverrideCursor( Qt::WaitCursor );

  ProxyWidget *widget = _modulew->load( module );

  if (widget)
  {
    _module = module;
    connect(_module, SIGNAL(childClosed()), SLOT(removeModule()));
    connect(_module, SIGNAL(changed(ConfigModule *)),
            SIGNAL(changedModule(ConfigModule *)));
    connect(widget, SIGNAL(quickHelpChanged()), SLOT(quickHelpChanged()));

    setCurrentWidget( _modulew );
    emit newModule(widget->windowTitle(), module->docPath(), widget->quickHelp());
  }
  else
  {
    setCurrentWidget( _basew );
    emit newModule(_basew->windowTitle(), "", "");
  }

  QApplication::restoreOverrideCursor();

  return widget;
}

bool DockContainer::dockModule(ConfigModule *module)
{
  if (module == _module) return true;

  if (_module && _module->isChanged())
    {

      int res = KMessageBox::warningYesNoCancel(this,
module ?
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
        _module->module()->applyClicked();
      if (res == KMessageBox::Cancel)
        return false;
    }

  setCurrentWidget( _busyw );
  kapp->processEvents();

  deleteModule();
  if (!module) return true;

  ProxyWidget *widget = loadModule( module );

  KCGlobal::repairAccels( topLevelWidget() );
  return ( widget!=0 );
}

void DockContainer::removeModule()
{
  setCurrentWidget( _basew );
  deleteModule();

  if (_basew)
      emit newModule(_basew->windowTitle(), "", "");
  else
      emit newModule("", "", "");
}

void DockContainer::deleteModule()
{
  if(_module) {
	_module->deleteClient();
	_module = 0;
  }
}

void DockContainer::quickHelpChanged()
{
  if (_module && _module->module())
	emit newModule(_module->module()->windowTitle(), _module->docPath(), _module->module()->quickHelp());
}
