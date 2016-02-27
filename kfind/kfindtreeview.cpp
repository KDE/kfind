/*******************************************************************
* kfindtreeview.cpp
* Copyright 2009    Dario Andres Rodriguez <andresbajotierra@gmail.com>
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
******************************************************************/

#include "kfindtreeview.h"

#include "kfinddlg.h"

#include <QTextStream>
#include <QTextCodec>
#include <QFileInfo>
#include <QClipboard>
#include <QHeaderView>
#include <QApplication>
#include <QDate>

#include <KActionCollection>
#include <kfiledialog.h>
#include <klocale.h>
#include <kapplication.h>
#include <krun.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <QDebug>
#include <kiconloader.h>
#include <kglobalsettings.h>
#include <kjobwidgets.h>

#include <kio/netaccess.h>
#include <kio/copyjob.h>
#include <kio/deletejob.h>
#include <kio/jobuidelegate.h>

#include <knewfilemenu.h>

// Permission strings
static const char* const perm[4] = {
  I18N_NOOP( "Read-write" ),
  I18N_NOOP( "Read-only" ),
  I18N_NOOP( "Write-only" ),
  I18N_NOOP( "Inaccessible" ) };
#define RW 0
#define RO 1
#define WO 2
#define NA 3

//BEGIN KFindItemModel

KFindItemModel::KFindItemModel( KFindTreeView * parentView ) : 
    QAbstractTableModel( parentView )
{
    m_view = parentView;
}

QVariant KFindItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if ( role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
            case 0:
                return i18nc("file name column","Name");
            case 1:
                return i18nc("name of the containing folder","In Subfolder");
            case 2:
                return i18nc("file size column","Size");
            case 3:
                return i18nc("modified date column","Modified");
            case 4:
                return i18nc("file permissions column","Permissions");
            case 5:
                return i18nc("first matching line of the query string in this file", "First Matching Line");
            default:
                return QVariant();
        }
    }
    return QVariant();
}

void KFindItemModel::insertFileItems( const QList< QPair<KFileItem,QString> > & pairs)
{
    if ( pairs.size() > 0 )
    {
        beginInsertRows( QModelIndex(), m_itemList.size(), m_itemList.size()+pairs.size()-1 );
        
        QList< QPair<KFileItem,QString> >::const_iterator it = pairs.constBegin();
        QList< QPair<KFileItem,QString> >::const_iterator end = pairs.constEnd();
        
        for (; it != end; ++it)
        {
            QPair<KFileItem,QString> pair = *it;

            QString subDir = m_view->reducedDir(pair.first.url().adjusted(QUrl::RemoveFilename).path());
            m_itemList.append( KFindItem( pair.first, subDir, pair.second ) );
        }

        endInsertRows();
    }
}

int KFindItemModel::rowCount ( const QModelIndex & parent ) const
{ 
    if( !parent.isValid() )
        return m_itemList.count(); //Return itemcount for toplevel
    else
        return 0;
}

KFindItem KFindItemModel::itemAtIndex( const QModelIndex & index ) const
{
    if ( index.isValid() && m_itemList.size() >= index.row() )
        return m_itemList.at( index.row() );

    return KFindItem();
}

QVariant KFindItemModel::data ( const QModelIndex & index, int role ) const
{
    if (!index.isValid())
        return QVariant();

    if (index.column() > 6 || index.row() >= m_itemList.count() )
        return QVariant();

    switch( role )
    {
        case Qt::DisplayRole:
        case Qt::DecorationRole:
        case Qt::UserRole:
            return m_itemList.at( index.row() ).data( index.column(), role );
        default:
            return QVariant();
    }
    return QVariant();
}

void KFindItemModel::removeItem( const QUrl & url )
{
    int itemCount = m_itemList.size();
    for ( int i = 0; i < itemCount; i++)
    {
        KFindItem item = m_itemList.at(i);
        if ( item.getFileItem().url() == url )
        {
            beginRemoveRows( QModelIndex(), i, i ); 
            m_itemList.removeAt( i );
            endRemoveRows();
            return;
        }
    }
}

bool KFindItemModel::isInserted( const QUrl & url )
{
    int itemCount = m_itemList.size();
    for ( int i = 0; i < itemCount; i++)
    {
        KFindItem item = m_itemList.at(i);
        if ( item.getFileItem().url() == url )
        {
            return true;
        }
    }
    return false;
}

void KFindItemModel::clear()
{
    beginRemoveRows( QModelIndex(), 0, m_itemList.size() );
    m_itemList.clear();
    endRemoveRows();
}

Qt::ItemFlags KFindItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.isValid())
        return Qt::ItemIsDragEnabled | defaultFlags;
    return defaultFlags;
}
 
QMimeData * KFindItemModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> uris;
    
    foreach ( const QModelIndex & index, indexes )
    {
        if( index.isValid())
        {
            if( index.column() == 0 ) //Only use the first column item
            {
                uris.append( m_itemList.at( index.row() ).getFileItem().url() );
            }
        }
    }

    if ( uris.count() <= 0 )
        return 0;

    QMimeData * mimeData = new QMimeData();
    mimeData->setUrls(uris);
    
    return mimeData;
}

//END KFindItemModel

//BEGIN KFindItem

KFindItem::KFindItem( const KFileItem & _fileItem, const QString & subDir, const QString & matchingLine )
{
    m_fileItem = _fileItem;
    m_subDir = subDir;
    m_matchingLine = matchingLine;

    //TODO more caching ?
    if ( !m_fileItem.isNull() && m_fileItem.isLocalFile() )
    {
        QFileInfo fileInfo(m_fileItem.url().toLocalFile());

        int perm_index;
        if(fileInfo.isReadable())
            perm_index = fileInfo.isWritable() ? RW : RO;
        else
            perm_index = fileInfo.isWritable() ? WO : NA;
            
        m_permission = i18n(perm[perm_index]);
        
        m_icon = QIcon::fromTheme( m_fileItem.iconName() );
    }
}

QVariant KFindItem::data( int column, int role ) const
{
    if ( m_fileItem.isNull() )
        return QVariant();
        
    if( role == Qt::DecorationRole )
    {
        if (column == 0)
            return m_icon;
        else
            return QVariant();
    }
        
    if( role == Qt::DisplayRole )
        switch( column )
        {
            case 0:
                return m_fileItem.url().fileName();
            case 1:
                return m_subDir;
            case 2:
                return KIO::convertSize( m_fileItem.size() );
            case 3:
                return m_fileItem.timeString(KFileItem::ModificationTime);
            case 4:
                return m_permission;
            case 5:
                return m_matchingLine;
            default:
                return QVariant();
        }
        
    if( role == Qt::UserRole )
        switch( column )
        {
            case 2:
                return m_fileItem.size();
            case 3:
                return m_fileItem.time(KFileItem::ModificationTime).toTime_t();
            default:
                return QVariant();
        }
    
    return QVariant();
}

//END KFindItem

//BEGIN KFindSortFilterProxyModel

bool KFindSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    //Order by UserData size in bytes or unix date
    if( left.column() == 2 || left.column() == 3)
    {
        qulonglong leftData = sourceModel()->data( left, Qt::UserRole ).toULongLong();
        qulonglong rightData = sourceModel()->data( right, Qt::UserRole ).toULongLong();
        return leftData < rightData;
    }
    // Default sorting rules for string values
    else
    {
        return QSortFilterProxyModel::lessThan( left, right );
    }
}

//END KFindSortFilterProxyModel

//BEGIN KFindTreeView

KFindTreeView::KFindTreeView( QWidget *parent,  KfindDlg * findDialog )
    : QTreeView( parent ) ,
    m_contextMenu(0),
    m_kfindDialog(findDialog)
{
    //Configure model and proxy model
    m_model = new KFindItemModel( this );
    m_proxyModel = new KFindSortFilterProxyModel();
    m_proxyModel->setSourceModel( m_model );
    setModel( m_proxyModel );
    
    //Configure QTreeView
    setRootIsDecorated( false );
    setSelectionMode( QAbstractItemView::ExtendedSelection );
    setSortingEnabled( true );
    setDragEnabled( true );
    setContextMenuPolicy( Qt::CustomContextMenu );

    connect(this, &KFindTreeView::customContextMenuRequested, this, &KFindTreeView::contextMenuRequested);
           
    //Mouse single/double click settings
    connect(KGlobalSettings::self(), &KGlobalSettings::settingsChanged, this, &KFindTreeView::reconfigureMouseSettings);
    reconfigureMouseSettings();
    
    // TODO: this is a workaround until  Qt-issue 176832 has been fixed (from Dolphin)
    connect(this, &KFindTreeView::pressed, this, &KFindTreeView::updateMouseButtons);
                
    //Generate popup menu actions
    m_actionCollection = new KActionCollection( this );
    m_actionCollection->addAssociatedWidget(this);

    QAction * open = KStandardAction::open(this, SLOT(slotExecuteSelected()), this);
    m_actionCollection->addAction( QLatin1String("file_open"), open );
    
    QAction * copy = KStandardAction::copy(this, SLOT(copySelection()), this);
    m_actionCollection->addAction( QLatin1String("edit_copy"), copy );
    
    QAction * openFolder = new QAction( QIcon::fromTheme(QLatin1String("window-new")), i18n("&Open containing folder(s)"), this );
    connect(openFolder, &QAction::triggered, this, &KFindTreeView::openContainingFolder);
    m_actionCollection->addAction( QLatin1String("openfolder"), openFolder );
    
    QAction * del = new QAction( QIcon::fromTheme(QLatin1String("edit-delete")), i18n("&Delete"), this );
    connect(del, &QAction::triggered, this, &KFindTreeView::deleteSelectedFiles);
    m_actionCollection->setDefaultShortcut(del, Qt::SHIFT + Qt::Key_Delete);
   
    QAction * trash = new QAction( QIcon::fromTheme(QLatin1String("user-trash")), i18n("&Move to Trash"), this );
    connect(trash, &QAction::triggered, this, &KFindTreeView::moveToTrashSelectedFiles);
    m_actionCollection->setDefaultShortcut(trash, Qt::Key_Delete);
    m_actionCollection->addAction( QLatin1String("trash"), trash );
    
    header()->setStretchLastSection( true );
    
    sortByColumn( 0, Qt::AscendingOrder );
}

KFindTreeView::~KFindTreeView()
{
    delete m_model;
    delete m_proxyModel;
    delete m_actionCollection;
}

void KFindTreeView::resizeToContents()
{
    resizeColumnToContents( 0 );
    resizeColumnToContents( 1 );
    resizeColumnToContents( 2 );
    resizeColumnToContents( 3 );
}

QString KFindTreeView::reducedDir(const QString& fullDir)
{
    QString relDir = m_baseDir.relativeFilePath(fullDir);
    if (!relDir.startsWith(QLatin1String("..")))
        return relDir;
    return fullDir;
}

void KFindTreeView::beginSearch(const QUrl& baseUrl)
{
    //qDebug() << QString("beginSearch in: %1").arg(baseUrl.path());
    m_baseDir = QDir(baseUrl.toLocalFile());
    m_model->clear();
}

void KFindTreeView::endSearch()
{
    resizeToContents();
}

void KFindTreeView::insertItems (const QList< QPair<KFileItem,QString> > & pairs)
{
    m_model->insertFileItems( pairs );
}

void KFindTreeView::removeItem(const QUrl & url)
{
    QList<QUrl> list = selectedUrls();
    if ( list.contains(url) )
    {
        //Close menu
        if( m_contextMenu )
        {
            m_contextMenu->hide();
            delete m_contextMenu;
            m_contextMenu = 0;
        }
    }
    m_model->removeItem( url );
}

// copy to clipboard
void KFindTreeView::copySelection()
{
    QMimeData * mime = m_model->mimeData( m_proxyModel->mapSelectionToSource( selectionModel()->selection() ).indexes() );
    if (mime)
    {
        QClipboard * cb = kapp->clipboard();
        cb->setMimeData( mime );
    }
}

void KFindTreeView::saveResults()
{
    KFileDialog *dlg = new KFileDialog(QString(), QString(), this);
    dlg->setOperationMode (KFileDialog::Saving);
    dlg->setWindowTitle( i18nc("@title:window", "Save Results As") );
    dlg->setFilter( QString::fromLatin1("*.html|%1\n*.txt|%2").arg( i18n("HTML page"), i18n("Text file") ) );
    dlg->setConfirmOverwrite(true);    
    
    dlg->exec();

    QUrl u = dlg->selectedUrl();
    
    QString filter = dlg->currentFilter();
    delete dlg;

    if (!u.isValid() || !u.isLocalFile())
        return;

    QString filename = u.toLocalFile();

    QFile file(filename);

    if ( !file.open(QIODevice::WriteOnly) )
    {
        KMessageBox::error(parentWidget(),
                i18n("Unable to save results."));
    }
    else
    {
        QTextStream stream( &file );
        stream.setCodec( QTextCodec::codecForLocale() );
        
        QList<KFindItem> itemList = m_model->getItemList();
        if ( filter == QLatin1String("*.html") ) 
        {
            stream << QString::fromLatin1("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\""
            "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"><html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
                            "<head>\n"
                            "<title>%2</title></head>\n"
                            "<meta charset=\"%1\">\n"
                            "<body>\n<h1>%2</h1>\n"
                            "<dl>\n")
            .arg(QString::fromLatin1(QTextCodec::codecForLocale()->name()))
            .arg(i18n("KFind Results File"));

            Q_FOREACH( const KFindItem & item, itemList )
            {
                const KFileItem fileItem = item.getFileItem();
                stream << QString::fromLatin1("<dt><a href=\"%1\">%2</a></dt>\n").arg( 
                    fileItem.url().url(), fileItem.url().toDisplayString() );

            }
            stream << QString::fromLatin1("</dl>\n</body>\n</html>\n");
        }
        else 
        {
            Q_FOREACH( const KFindItem & item, itemList )
            {
                stream << item.getFileItem().url().url() << endl;
            }
        }

        file.close();
        m_kfindDialog->setStatusMsg(i18nc("%1=filename", "Results were saved to: %1", filename));
    }
}

void KFindTreeView::openContainingFolder()
{
    QList<QUrl> uris = selectedUrls();
    QMap<QUrl, int> folderMaps;
    
    //Generate *unique* folders
    Q_FOREACH( const QUrl & url, uris )
    {
        QUrl dir = url.adjusted(QUrl::RemoveFilename);
        folderMaps.insert( dir, 0 );
    }

    //TODO if >1 add a warn ?
    Q_FOREACH( const QUrl & url, folderMaps.keys() )
    {
        (void) new KRun(url, this);
    }
}

void KFindTreeView::slotExecuteSelected()
{
    QModelIndexList selected = m_proxyModel->mapSelectionToSource( selectionModel()->selection() ).indexes();
    if ( selected.size() == 0 )
        return;
    
    //TODO if >X add a warn ?
    Q_FOREACH( const QModelIndex & index, selected )
    {
        if( index.column() == 0 )
        {
            KFindItem item = m_model->itemAtIndex( index );
            if ( item.isValid() )
                new KRun(item.getFileItem().targetUrl(), this);
        }
    }
}

void KFindTreeView::slotExecute( const QModelIndex & index )
{
    if( (m_mouseButtons & Qt::LeftButton) && QApplication::keyboardModifiers() == Qt::NoModifier )
    {
        if ( !index.isValid() )
            return;
            
        QModelIndex realIndex = m_proxyModel->mapToSource( index );
        if ( !realIndex.isValid() )
            return;
            
        KFindItem item = m_model->itemAtIndex( realIndex );
        if ( item.isValid() )
            new KRun(item.getFileItem().targetUrl(), this);
    }        
}

void KFindTreeView::contextMenuRequested( const QPoint & p)
{
    KFileItemList fileList;
    
    QModelIndexList selected = m_proxyModel->mapSelectionToSource( selectionModel()->selection() ).indexes();
    if ( selected.size() == 0 )
        return;
    
    Q_FOREACH( const QModelIndex & index, selected )
    {
        if( index.column() == 0 )
        {
            const KFindItem item = m_model->itemAtIndex( index );
            if( item.isValid() )
                fileList.append( item.getFileItem() );
        }
    }
    
    KParts::BrowserExtension::PopupFlags flags = KParts::BrowserExtension::ShowProperties; // | KParts::BrowserExtension::ShowUrlOperations;
    
    QList<QAction*> editActions;
    editActions.append(m_actionCollection->action(QLatin1String("file_open")));
    editActions.append(m_actionCollection->action(QLatin1String("openfolder")));
    editActions.append(m_actionCollection->action(QLatin1String("edit_copy")));
    editActions.append(m_actionCollection->action(QLatin1String("del")));
    editActions.append(m_actionCollection->action(QLatin1String("trash")));
    
    KParts::BrowserExtension::ActionGroupMap actionGroups;
    actionGroups.insert(QLatin1String("editactions"), editActions);
    
    if( m_contextMenu )
    {
        m_contextMenu->hide();
        delete m_contextMenu;
        m_contextMenu = 0;
    }
    m_contextMenu = new KonqPopupMenu( fileList, QUrl(), *m_actionCollection, new KNewFileMenu( m_actionCollection, QLatin1String("new_menu"), this), 0, flags, this, 0, actionGroups);

    m_contextMenu->exec( this->mapToGlobal( p ) );
}

QList<QUrl> KFindTreeView::selectedUrls()
{
    QList<QUrl> uris;
    
    QModelIndexList indexes = m_proxyModel->mapSelectionToSource( selectionModel()->selection() ).indexes();
    Q_FOREACH( const QModelIndex & index, indexes )
    {
        if( index.column() == 0 && index.isValid() )
        {
            KFindItem item = m_model->itemAtIndex( index );
            if( item.isValid() )
                uris.append( item.getFileItem().url() );
        }
    }
    
    return uris;
}

void KFindTreeView::deleteSelectedFiles()
{
    QList<QUrl> uris = selectedUrls();
    if ( uris.isEmpty() ) {
        return;
    }

    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(this);
    if (uiDelegate.askDeleteConfirmation(uris, KIO::JobUiDelegate::Delete, KIO::JobUiDelegate::ForceConfirmation)) {
        KJob * deleteJob = KIO::del(uris);
        KJobWidgets::setWindow(deleteJob, this);
        deleteJob->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

void KFindTreeView::moveToTrashSelectedFiles()
{
    QList<QUrl> uris = selectedUrls();
    if ( uris.isEmpty() ) {
        return;
    }

    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(this);
    if (uiDelegate.askDeleteConfirmation(uris, KIO::JobUiDelegate::Trash, KIO::JobUiDelegate::ForceConfirmation)) {
        KJob * trashJob = KIO::trash(uris);
        KJobWidgets::setWindow(trashJob, this);
        trashJob->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

void KFindTreeView::reconfigureMouseSettings()
{
    disconnect( SIGNAL(clicked(QModelIndex)) );
    disconnect( SIGNAL(doubleClicked(QModelIndex)) );
    
    if ( KGlobalSettings::singleClick() ) 
    {
        connect(this, &KFindTreeView::clicked, this, &KFindTreeView::slotExecute);
    } else {
        connect(this, &KFindTreeView::doubleClicked, this, &KFindTreeView::slotExecute);
    }
}

void KFindTreeView::updateMouseButtons()
{
    m_mouseButtons = QApplication::mouseButtons();
}

//END KFindTreeView
