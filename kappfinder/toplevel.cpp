/*
    KAppfinder, the KDE application finder

    Copyright (c) 2002-2003 Tobias Koenig <tokoe@kde.org>

    Based on code written by Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// Own
#include "toplevel.h"

// Qt
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtCore/QDir>
#include <QtCore/QRegExp>
#include <QtGui/QProgressBar>
#include <QtGui/QBoxLayout>

// KDE
#include <kapplication.h>
#include <kbuildsycocaprogressdialog.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kservice.h>
#include <kservicegroup.h>
#include <kstandarddirs.h>
#include <kstandardguiitem.h>

TopLevel::TopLevel( const QString &destDir, QWidget *parent )
  : KDialog( parent )
{
  setCaption( i18n( "KAppfinder" ) );
  setButtons( Apply | Close | User1 | User2 | User3 );

  QFrame *frame = new QFrame;
  setMainWidget( frame );

  QVBoxLayout *layout = new QVBoxLayout( frame );
  layout->setMargin( marginHint() );

  QLabel *label = new QLabel( i18n( "The application finder looks for non-KDE "
                                    "applications on your system and adds "
                                    "them to the KDE menu system. "
                                    "Click 'Scan' to begin, select the desired "
                                    "applications and then click 'Apply'."), frame);
  label->setAlignment( Qt::AlignLeft );
  label->setWordWrap( true );

  mListView = new Q3ListView( frame );
  mListView->addColumn( i18n( "Application" ) );
  mListView->addColumn( i18n( "Description" ) );
  mListView->addColumn( i18n( "Command" ) );
  mListView->setMinimumSize( 300, 300 );
  mListView->setRootIsDecorated( true );
  mListView->setAllColumnsShowFocus( true );
  mListView->setSelectionMode(Q3ListView::NoSelection);

  mProgress = new QProgressBar( frame );
  mProgress->setTextVisible( false );

  mSummary = new QLabel( i18n( "Summary:" ), frame );

  setButtonGuiItem(User1 , KGuiItem( i18n( "Scan" ), "edit-find"));
  setButtonGuiItem(User2,  KGuiItem(i18n( "Select All" )));
  setButtonGuiItem(User3,  KGuiItem(i18n( "Unselect All" )));
  connect(this,SIGNAL(user1Clicked()), this, SLOT( slotScan() ) );
  connect(this,SIGNAL(user2Clicked()), this, SLOT( slotSelectAll() ) );
  connect(this,SIGNAL(user3Clicked()), this, SLOT( slotUnselectAll() ) );
  connect(this,SIGNAL(applyClicked()), this, SLOT( slotCreate() ) );
  connect(this,SIGNAL(closeClicked()), kapp, SLOT( quit() ) );
  
  enableButton(User2, false );
  enableButton(User3, false );
  enableButton(Apply, false );
  

  layout->addWidget( label );
  layout->addSpacing( 5 );
  layout->addWidget( mListView );
  layout->addWidget( mProgress );
  layout->addWidget( mSummary );

  connect( kapp, SIGNAL( lastWindowClosed() ), kapp, SLOT( quit() ) );

  adjustSize();

  mDestDir = destDir;
  mDestDir = mDestDir.replace( QRegExp( "^~/" ), QDir::homePath() + '/' );
}

TopLevel::~TopLevel()
{
  qDeleteAll(mAppCache);
  mAppCache.clear();
}

Q3ListViewItem* TopLevel::addGroupItem( Q3ListViewItem *parent, const QString &relPath,
                                       const QString &name )
{
  KServiceGroup::Ptr root = KServiceGroup::group( relPath );
  if( !root )
    return 0L;
  KServiceGroup::List list = root->entries();

  KServiceGroup::List::ConstIterator it;
  for ( it = list.begin(); it != list.end(); ++it ) {
    const KSycocaEntry *p = (*it).data();
    if ( p->isType( KST_KServiceGroup ) ) {
      const KServiceGroup* serviceGroup = static_cast<const KServiceGroup*>( p );
      if ( QString( "%1%2/" ).arg( relPath ).arg( name ) == serviceGroup->relPath() ) {
        Q3ListViewItem* retval;
        if ( parent )
          retval = parent->firstChild();
        else
          retval = mListView->firstChild();

        while ( retval ) {
          if ( retval->text( 0 ) == serviceGroup->caption() )
            return retval;

          retval = retval->nextSibling();
        }

        Q3ListViewItem *item;
        if ( parent )
          item = new Q3ListViewItem( parent, serviceGroup->caption() );
        else
          item = new Q3ListViewItem( mListView, serviceGroup->caption() );

        item->setPixmap( 0, SmallIcon( serviceGroup->icon() ) );
        item->setOpen( true );
        return item;
      }
    }
  }

  return 0;
}

void TopLevel::slotScan()
{
  KIconLoader* loader = KIconLoader::global();

  mTemplates = KGlobal::dirs()->findAllResources( "data", "kappfinder/apps/*.desktop", KStandardDirs::Recursive );

  mAppCache.clear();

  mFound = 0;
  int count = mTemplates.count();

  enableButton(User1, false ); //disable scan button
  mProgress->setTextVisible( true );
  mProgress->setRange( 0, count );
  mProgress->setValue( 0 );

  mListView->clear();

  QStringList::Iterator it;
  for ( it = mTemplates.begin(); it != mTemplates.end(); ++it ) {
    // eye candy
    mProgress->setValue( mProgress->value() + 1 );

    QString desktopName = *it;
    int i = desktopName.lastIndexOf('/');
    desktopName = desktopName.mid(i+1);
    i = desktopName.lastIndexOf('.');
    if (i != -1)
       desktopName = desktopName.left(i);

    bool found;
    found = KService::serviceByDesktopName(desktopName);
    if (found)
       continue;

    found = KService::serviceByMenuId("kde4-"+desktopName+".desktop");
    if (found)
       continue; 

    found = KService::serviceByMenuId("gnome-"+desktopName+".desktop");
    if (found)
       continue; 

    KDesktopFile desktop(  *it );

    // copy over the desktop file, if exists
    if ( scanDesktopFile( mAppCache, *it, mDestDir ) ) {
      QString relPath = *it;
      int pos = relPath.indexOf( "kappfinder/apps/" );
      relPath = relPath.mid( pos + strlen( "kappfinder/apps/" ) );
      relPath = relPath.left( relPath.lastIndexOf( '/' ) + 1 );
      QStringList dirList = relPath.split( '/');

      Q3ListViewItem *dirItem = 0;
      QString tmpRelPath = QString();

      QStringList::Iterator tmpIt;
      for ( tmpIt = dirList.begin(); tmpIt != dirList.end(); ++tmpIt ) {
        dirItem = addGroupItem( dirItem, tmpRelPath, *tmpIt );
        tmpRelPath += *tmpIt + '/';
      }

      mFound++;

      Q3CheckListItem *item;
      if ( dirItem )
        item = new Q3CheckListItem( dirItem, desktop.readName(), Q3CheckListItem::CheckBox );
      else
        item = new Q3CheckListItem( mListView, desktop.readName(), Q3CheckListItem::CheckBox );

      item->setPixmap( 0, loader->loadIcon( desktop.readIcon(), K3Icon::Small ) );
      item->setText( 1, desktop.readGenericName() );
      item->setText( 2, desktop.desktopGroup().readPathEntry( "Exec" ) );
      if ( desktop.desktopGroup().readEntry( "X-StandardInstall" , false) )
        item->setOn( true );

      AppLnkCache* cache = mAppCache.last();
      if ( cache )
        cache->item = item;
    }

    // update summary
    QString sum( i18np( "Summary: %1 application found",
                       "Summary: %1 applications found", mFound ) );
    mSummary->setText( sum );
  }

  // stop scanning
  mProgress->setValue( 0 );
  mProgress->setTextVisible( false );

  enableButton(User1, true ); //enable scan button

  if ( mFound > 0 ) {
    enableButton(Apply, true );
    enableButton(User2, true ); 
    enableButton(User3, true ); 
  }
}

void TopLevel::slotSelectAll()
{
  AppLnkCache* cache;
  Q_FOREACH( cache, mAppCache )
    cache->item->setOn( true );
}

void TopLevel::slotUnselectAll()
{
  AppLnkCache* cache;
  Q_FOREACH( cache, mAppCache )
    cache->item->setOn( false );
}

void TopLevel::slotCreate()
{
  // copy template files
  mAdded = 0;
  createDesktopFiles( mAppCache, mAdded );

  // decorate directories
  decorateDirs( mDestDir );

  KBuildSycocaProgressDialog::rebuildKSycoca(this);

  QString message( i18np( "%1 application was added to the KDE menu system.",
                         "%1 applications were added to the KDE menu system.", mAdded ) );
  KMessageBox::information( this, message, QString(), "ShowInformation" );
}

#include "toplevel.moc"
