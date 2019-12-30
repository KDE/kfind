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

#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QMenu>
#include <QMimeData>
#include <QScrollBar>
#include <QTextCodec>
#include <QTextStream>

#include <KActionCollection>
#include <KFileItemActions>
#include <KFileItemListProperties>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPropertiesDialog>
#include <KRun>

#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/JobUiDelegate>
#include <KIO/OpenFileManagerWindowJob>
#include <kio_version.h>

// Permission strings
static const char *const perm[4] = {
    I18N_NOOP("Read-write"),
    I18N_NOOP("Read-only"),
    I18N_NOOP("Write-only"),
    I18N_NOOP("Inaccessible")
};
#define RW 0
#define RO 1
#define WO 2
#define NA 3

//BEGIN KFindItemModel

KFindItemModel::KFindItemModel(KFindTreeView *parentView)
    : QAbstractTableModel(parentView)
{
    m_view = parentView;
}

QVariant KFindItemModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
        switch (section) {
        case 0:
            return i18nc("file name column", "Name");
        case 1:
            return i18nc("name of the containing folder", "In Subfolder");
        case 2:
            return i18nc("file size column", "Size");
        case 3:
            return i18nc("modified date column", "Modified");
        case 4:
            return i18nc("file permissions column", "Permissions");
        case 5:
            return i18nc("first matching line of the query string in this file", "First Matching Line");
        default:
            return QVariant();
        }
    }
    return QVariant();
}

void KFindItemModel::insertFileItems(const QList< QPair<KFileItem, QString> > &pairs)
{
    if (pairs.size() > 0) {
        beginInsertRows(QModelIndex(), m_itemList.size(), m_itemList.size()+pairs.size()-1);

        QList< QPair<KFileItem, QString> >::const_iterator it = pairs.constBegin();
        QList< QPair<KFileItem, QString> >::const_iterator end = pairs.constEnd();

        for (; it != end; ++it) {
            QPair<KFileItem, QString> pair = *it;

            QString subDir = m_view->reducedDir(pair.first.url().adjusted(QUrl::RemoveFilename).path());
            m_itemList.append(KFindItem(pair.first, subDir, pair.second));
        }

        endInsertRows();
    }
}

int KFindItemModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return m_itemList.count(); //Return itemcount for toplevel
    } else {
        return 0;
    }
}

KFindItem KFindItemModel::itemAtIndex(const QModelIndex &index) const
{
    if (index.isValid() && m_itemList.size() >= index.row()) {
        return m_itemList.at(index.row());
    }

    return KFindItem();
}

QVariant KFindItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.column() > 6 || index.row() >= m_itemList.count()) {
        return QVariant();
    }

    switch (role) {
    case Qt::DisplayRole:
    case Qt::DecorationRole:
    case Qt::UserRole:
        return m_itemList.at(index.row()).data(index.column(), role);
    default:
        return QVariant();
    }
    return QVariant();
}

void KFindItemModel::removeItem(const QUrl &url)
{
    int itemCount = m_itemList.size();
    for (int i = 0; i < itemCount; i++) {
        KFindItem item = m_itemList.at(i);
        if (item.getFileItem().url() == url) {
            beginRemoveRows(QModelIndex(), i, i);
            m_itemList.removeAt(i);
            endRemoveRows();
            return;
        }
    }
}

bool KFindItemModel::isInserted(const QUrl &url)
{
    int itemCount = m_itemList.size();
    for (int i = 0; i < itemCount; i++) {
        KFindItem item = m_itemList.at(i);
        if (item.getFileItem().url() == url) {
            return true;
        }
    }
    return false;
}

void KFindItemModel::clear()
{
    beginRemoveRows(QModelIndex(), 0, m_itemList.size());
    m_itemList.clear();
    endRemoveRows();
}

Qt::ItemFlags KFindItemModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.isValid()) {
        return Qt::ItemIsDragEnabled | defaultFlags;
    }
    return defaultFlags;
}

QMimeData *KFindItemModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> uris;

    foreach (const QModelIndex &index, indexes) {
        if (index.isValid()) {
            if (index.column() == 0) { //Only use the first column item
                uris.append(m_itemList.at(index.row()).getFileItem().url());
            }
        }
    }

    if (uris.isEmpty()) {
        return nullptr;
    }

    QMimeData *mimeData = new QMimeData();
    mimeData->setUrls(uris);

    return mimeData;
}

//END KFindItemModel

//BEGIN KFindItem

KFindItem::KFindItem(const KFileItem &_fileItem, const QString &subDir, const QString &matchingLine)
{
    m_fileItem = _fileItem;
    m_subDir = subDir;
    m_matchingLine = matchingLine;

    //TODO more caching ?
    if (!m_fileItem.isNull() && m_fileItem.isLocalFile()) {
        QFileInfo fileInfo(m_fileItem.url().toLocalFile());

        int perm_index;
        if (fileInfo.isReadable()) {
            perm_index = fileInfo.isWritable() ? RW : RO;
        } else {
            perm_index = fileInfo.isWritable() ? WO : NA;
        }

        m_permission = i18n(perm[perm_index]);

        m_icon = QIcon::fromTheme(m_fileItem.iconName());
    }
}

QVariant KFindItem::data(int column, int role) const
{
    if (m_fileItem.isNull()) {
        return QVariant();
    }

    if (role == Qt::DecorationRole) {
        if (column == 0) {
            return m_icon;
        } else {
            return QVariant();
        }
    }

    if (role == Qt::DisplayRole) {
        switch (column) {
        case 0:
            return m_fileItem.url().fileName();
        case 1:
            return m_subDir;
        case 2:
            return KIO::convertSize(m_fileItem.size());
        case 3:
            return m_fileItem.timeString(KFileItem::ModificationTime);
        case 4:
            return m_permission;
        case 5:
            return m_matchingLine;
        default:
            return QVariant();
        }
    }

    if (role == Qt::UserRole) {
        switch (column) {
        case 2:
            return m_fileItem.size();
        case 3:
            return m_fileItem.time(KFileItem::ModificationTime).toSecsSinceEpoch();
        default:
            return QVariant();
        }
    }

    return QVariant();
}

//END KFindItem

//BEGIN KFindSortFilterProxyModel

bool KFindSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    //Order by UserData size in bytes or unix date
    if (left.column() == 2 || left.column() == 3) {
        qulonglong leftData = sourceModel()->data(left, Qt::UserRole).toULongLong();
        qulonglong rightData = sourceModel()->data(right, Qt::UserRole).toULongLong();
        return leftData < rightData;
    }
    // Default sorting rules for string values
    else {
        return QSortFilterProxyModel::lessThan(left, right);
    }
}

//END KFindSortFilterProxyModel

//BEGIN KFindTreeView

KFindTreeView::KFindTreeView(QWidget *parent, KfindDlg *findDialog)
    : QTreeView(parent)
    , m_contextMenu(Q_NULLPTR)
    , m_kfindDialog(findDialog)
{
    //Configure model and proxy model
    m_model = new KFindItemModel(this);
    m_proxyModel = new KFindSortFilterProxyModel();
    m_proxyModel->setSourceModel(m_model);
    setModel(m_proxyModel);

    //Configure QTreeView
    setRootIsDecorated(false);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSortingEnabled(true);
    setDragEnabled(true);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &KFindTreeView::customContextMenuRequested, this, &KFindTreeView::contextMenuRequested);

    //Mouse single/double click settings
    reconfigureMouseSettings();

    // TODO: this is a workaround until  Qt-issue 176832 has been fixed (from Dolphin)
    connect(this, &KFindTreeView::pressed, this, &KFindTreeView::updateMouseButtons);

    //Generate popup menu actions
    m_actionCollection = new KActionCollection(this);
    m_actionCollection->addAssociatedWidget(this);

    QAction *open = KStandardAction::open(this, SLOT(slotExecuteSelected()), this);
    m_actionCollection->addAction(QStringLiteral("file_open"), open);

    QAction *copy = KStandardAction::copy(this, SLOT(copySelection()), this);
    m_actionCollection->addAction(QStringLiteral("edit_copy"), copy);

    QAction *openFolder = new QAction(QIcon::fromTheme(QStringLiteral("window-new")), i18n("&Open containing folder(s)"), this);
    connect(openFolder, &QAction::triggered, this, &KFindTreeView::openContainingFolder);
    m_actionCollection->addAction(QStringLiteral("openfolder"), openFolder);

    QAction *del = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("&Delete"), this);
    connect(del, &QAction::triggered, this, &KFindTreeView::deleteSelectedFiles);
    m_actionCollection->setDefaultShortcut(del, Qt::SHIFT + Qt::Key_Delete);

    QAction *trash = new QAction(QIcon::fromTheme(QStringLiteral("user-trash")), i18n("&Move to Trash"), this);
    connect(trash, &QAction::triggered, this, &KFindTreeView::moveToTrashSelectedFiles);
    m_actionCollection->setDefaultShortcut(trash, Qt::Key_Delete);
    m_actionCollection->addAction(QStringLiteral("trash"), trash);

    header()->setStretchLastSection(true);

    sortByColumn(0, Qt::AscendingOrder);
}

KFindTreeView::~KFindTreeView()
{
    delete m_model;
    delete m_proxyModel;
    delete m_actionCollection;
}

void KFindTreeView::resizeToContents()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
    resizeColumnToContents(2);
    resizeColumnToContents(3);
}

QString KFindTreeView::reducedDir(const QString &fullDir)
{
    QString relDir = m_baseDir.relativeFilePath(fullDir);
    if (!relDir.startsWith(QLatin1String(".."))) {
        return relDir;
    }
    return fullDir;
}

void KFindTreeView::beginSearch(const QUrl &baseUrl)
{
    m_baseDir = QDir(baseUrl.toLocalFile());
    m_model->clear();
}

void KFindTreeView::endSearch()
{
    resizeToContents();
}

QList<QPersistentModelIndex> KFindTreeView::selectedVisibleIndexes() {
    QModelIndexList selected = selectedIndexes();
    if (selected.empty()) {
        return QList<QPersistentModelIndex>();
    }
    QModelIndex index = indexAt(QPoint(0, 0));
    QModelIndex bottomIndex = indexAt(viewport()->rect().bottomLeft());
    QList<QPersistentModelIndex> result;
    while (index.isValid()) {
        if (selected.contains(index)) {
            result.append(QPersistentModelIndex(index));
        }
        if (index == bottomIndex) {
            break;
        }
        index = indexBelow(index);
    }
    return result;
}

void KFindTreeView::insertItems(const QList< QPair<KFileItem, QString> > &pairs)
{
    auto hScroll = horizontalScrollBar()->value();
    QPersistentModelIndex topIndex(indexAt(rect().topLeft()));
    QList<QPersistentModelIndex> selectedVisible = selectedVisibleIndexes();

    m_model->insertFileItems(pairs);

    if (topIndex.isValid()) {
        if (verticalScrollBar()->value() > 0) {
            scrollTo(topIndex, QAbstractItemView::PositionAtTop);
        }
    }
    if (!selectedVisible.empty()) {
        scrollTo(selectedVisible.last());
    }
    horizontalScrollBar()->setValue(hScroll);
}

void KFindTreeView::removeItem(const QUrl &url)
{
    QList<QUrl> list = selectedUrls();
    if (list.contains(url)) {
        //Close menu
        if (m_contextMenu) {
            delete m_contextMenu;
            m_contextMenu = nullptr;
        }
    }
    m_model->removeItem(url);
}

// copy to clipboard
void KFindTreeView::copySelection()
{
    QMimeData *mime = m_model->mimeData(m_proxyModel->mapSelectionToSource(selectionModel()->selection()).indexes());
    if (mime) {
        QClipboard *cb = qApp->clipboard();
        cb->setMimeData(mime);
    }
}

void KFindTreeView::saveResults()
{
    QFileDialog dialog;
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    dialog.setWindowTitle(i18nc("@title:window", "Save Results As"));
    dialog.setMimeTypeFilters({
        QStringLiteral("text/html"),
        QStringLiteral("text/plain")
    });

    if (!dialog.exec()) {
        return;
    }

    if (dialog.selectedUrls().isEmpty()) {
        return;
    }

    const QUrl u = dialog.selectedUrls().constFirst();
    if (!u.isValid() || !u.isLocalFile()) {
        return;
    }

    QString filename = u.toLocalFile();

    QFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        KMessageBox::error(parentWidget(),
                           i18n("Unable to save results."));
    } else {
        QTextStream stream(&file);
        stream.setCodec(QTextCodec::codecForLocale());

        const QList<KFindItem> itemList = m_model->getItemList();
        if (dialog.selectedMimeTypeFilter() == QLatin1String("text/html")) {
            stream << QString::fromLatin1("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\""
                                          "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"><html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
                                          "<head>\n"
                                          "<title>%2</title></head>\n"
                                          "<meta charset=\"%1\">\n"
                                          "<body>\n<h1>%2</h1>\n"
                                          "<dl>\n")
                .arg(QString::fromLatin1(QTextCodec::codecForLocale()->name()))
                .arg(i18n("KFind Results File"));

            for (const KFindItem &item : itemList) {
                const KFileItem fileItem = item.getFileItem();
                stream << QStringLiteral("<dt><a href=\"%1\">%2</a></dt>\n").arg(
                    fileItem.url().url(), fileItem.url().toDisplayString());
            }
            stream << QStringLiteral("</dl>\n</body>\n</html>\n");
        } else {
            for (const KFindItem &item : itemList) {
                stream << item.getFileItem().url().url()
          #if (QT_VERSION < QT_VERSION_CHECK(5, 15, 0))
                       << endl
          #else
                       << Qt::endl
          #endif
                          ;
            }
        }

        file.close();
        m_kfindDialog->setStatusMsg(i18nc("%1=filename", "Results were saved to: %1", filename));
    }
}

void KFindTreeView::openContainingFolder()
{
    KIO::highlightInFileManager(selectedUrls());
}

void KFindTreeView::slotExecuteSelected()
{
    const QModelIndexList selected = m_proxyModel->mapSelectionToSource(selectionModel()->selection()).indexes();
    if (selected.isEmpty()) {
        return;
    }

    //TODO if >X add a warn ?
    Q_FOREACH (const QModelIndex &index, selected) {
        if (index.column() == 0) {
            KFindItem item = m_model->itemAtIndex(index);
            if (item.isValid()) {
                new KRun(item.getFileItem().targetUrl(), this);
            }
        }
    }
}

void KFindTreeView::slotExecute(const QModelIndex &index)
{
    if ((m_mouseButtons &Qt::LeftButton) && QApplication::keyboardModifiers() == Qt::NoModifier) {
        if (!index.isValid()) {
            return;
        }

        QModelIndex realIndex = m_proxyModel->mapToSource(index);
        if (!realIndex.isValid()) {
            return;
        }

        KFindItem item = m_model->itemAtIndex(realIndex);
        if (item.isValid()) {
            new KRun(item.getFileItem().targetUrl(), this);
        }
    }
}

void KFindTreeView::contextMenuRequested(const QPoint &p)
{
    KFileItemList fileList;

    const QModelIndexList selected = m_proxyModel->mapSelectionToSource(selectionModel()->selection()).indexes();
    if (selected.isEmpty()) {
        return;
    }

    Q_FOREACH (const QModelIndex &index, selected) {
        if (index.column() == 0) {
            const KFindItem item = m_model->itemAtIndex(index);
            if (item.isValid()) {
                fileList.append(item.getFileItem());
            }
        }
    }

    delete m_contextMenu;
    m_contextMenu = new QMenu(this);
    m_contextMenu->addAction(m_actionCollection->action(QStringLiteral("file_open")));
    m_contextMenu->addAction(m_actionCollection->action(QStringLiteral("openfolder")));
    m_contextMenu->addAction(m_actionCollection->action(QStringLiteral("edit_copy")));
    //m_contextMenu->addAction(m_actionCollection->action(QLatin1String("del")));
    m_contextMenu->addAction(m_actionCollection->action(QStringLiteral("trash")));
    m_contextMenu->addSeparator();

    // Open With...
    KFileItemActions menuActions;
    KFileItemListProperties fileProperties(fileList);
    menuActions.setItemListProperties(fileProperties);
    menuActions.addOpenWithActionsTo(m_contextMenu);
    // 'Actions' submenu
    menuActions.addServiceActionsTo(m_contextMenu);
    // Plugins
    menuActions.addPluginActionsTo(m_contextMenu);

    m_contextMenu->addSeparator();

    // Properties
    if (KPropertiesDialog::canDisplay(fileList)) {
        QAction *act = new QAction(m_contextMenu);
        act->setText(i18n("&Properties"));
        QObject::connect(act, &QAction::triggered, [this, fileList]() {
            KPropertiesDialog::showDialog(fileList, this, false /*non modal*/);
        });
        m_contextMenu->addAction(act);
    }

    m_contextMenu->exec(this->mapToGlobal(p));
}

QList<QUrl> KFindTreeView::selectedUrls()
{
    QList<QUrl> uris;

    const QModelIndexList indexes = m_proxyModel->mapSelectionToSource(selectionModel()->selection()).indexes();
    Q_FOREACH (const QModelIndex &index, indexes) {
        if (index.column() == 0 && index.isValid()) {
            KFindItem item = m_model->itemAtIndex(index);
            if (item.isValid()) {
                uris.append(item.getFileItem().url());
            }
        }
    }

    return uris;
}

void KFindTreeView::deleteSelectedFiles()
{
    QList<QUrl> uris = selectedUrls();
    if (uris.isEmpty()) {
        return;
    }

    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(this);
    if (uiDelegate.askDeleteConfirmation(uris, KIO::JobUiDelegate::Delete, KIO::JobUiDelegate::ForceConfirmation)) {
        KJob *deleteJob = KIO::del(uris);
        KJobWidgets::setWindow(deleteJob, this);
        deleteJob->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

void KFindTreeView::moveToTrashSelectedFiles()
{
    QList<QUrl> uris = selectedUrls();
    if (uris.isEmpty()) {
        return;
    }

    KIO::JobUiDelegate uiDelegate;
    uiDelegate.setWindow(this);
    if (uiDelegate.askDeleteConfirmation(uris, KIO::JobUiDelegate::Trash, KIO::JobUiDelegate::ForceConfirmation)) {
        KJob *trashJob = KIO::trash(uris);
        KJobWidgets::setWindow(trashJob, this);
        trashJob->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

void KFindTreeView::reconfigureMouseSettings()
{
    disconnect(this, &KFindTreeView::clicked, this, &KFindTreeView::slotExecute);
    disconnect(this, &KFindTreeView::doubleClicked, this, &KFindTreeView::slotExecute);

    if (m_kfindDialog->style()->styleHint(QStyle::SH_ItemView_ActivateItemOnSingleClick)) {
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
