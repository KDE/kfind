/*
    kfindtreeview.cpp
    SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

*/

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

#include <KIO/CopyJob>
#include <KIO/DeleteJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/JobUiDelegate>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KIO/DeleteOrTrashJob>

// Permission strings
#include <KLazyLocalizedString>

const KLazyLocalizedString perm[4] = {
    kli18n("Read-write"),
    kli18n("Read-only"),
    kli18n("Write-only"),
    kli18n("Inaccessible")
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

void KFindItemModel::insertFileItems(const QList<QPair<KFileItem, QString>> &pairs)
{
    if (!pairs.isEmpty()) {
        beginInsertRows(QModelIndex(), m_itemList.size(), m_itemList.size()+pairs.size()-1);

        for (const auto &[item, dir] : pairs) {
            const QString subDir = m_view->reducedDir(item.url().adjusted(QUrl::RemoveFilename).path());
            m_itemList.append(KFindItem(item, subDir, dir));
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

QList<KFindItem> KFindItemModel::getItemList() const
{
    return m_itemList;
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
    const int itemCount = m_itemList.size();
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
    const int itemCount = m_itemList.size();
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
    if (!m_itemList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_itemList.size());
        m_itemList.clear();
        endRemoveRows();
    }
}

Qt::DropActions KFindItemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

Qt::ItemFlags KFindItemModel::flags(const QModelIndex &index) const
{
    const Qt::ItemFlags defaultFlags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    if (index.isValid()) {
        return Qt::ItemIsDragEnabled | defaultFlags;
    }
    return defaultFlags;
}

QMimeData *KFindItemModel::mimeData(const QModelIndexList &indexes) const
{
    QList<QUrl> uris;

    for (const QModelIndex &index : indexes) {
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

int KFindItemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 6;
}

//END KFindItemModel

//BEGIN KFindItem

KFindItem::KFindItem(const KFileItem &_fileItem, const QString &subDir, const QString &matchingLine) : m_fileItem(_fileItem), m_matchingLine(matchingLine), m_subDir(subDir)
{
    //TODO more caching ?
    if (!m_fileItem.isNull() && m_fileItem.isLocalFile()) {
        QFileInfo fileInfo(m_fileItem.url().toLocalFile());

        int perm_index;
        if (fileInfo.isReadable()) {
            perm_index = fileInfo.isWritable() ? RW : RO;
        } else {
            perm_index = fileInfo.isWritable() ? WO : NA;
        }
        m_permission = KLocalizedString(perm[perm_index]).toString();

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

KFileItem KFindItem::getFileItem() const
{
    return m_fileItem;
}

bool KFindItem::isValid() const
{
    return !m_fileItem.isNull();
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
    , m_model(new KFindItemModel(this))
    , m_proxyModel(new KFindSortFilterProxyModel(this))
    , m_kfindDialog(findDialog)
{
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

    QAction *copyPathAction = m_actionCollection->addAction(QStringLiteral("copy_location"));
    copyPathAction->setText(i18nc("@action:incontextmenu", "Copy Location"));
    copyPathAction->setWhatsThis(i18nc("@info:whatsthis copy_location", "This will copy the path of the first selected item into the clipboard."));
    copyPathAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy-path")));
    KActionCollection::setDefaultShortcuts(copyPathAction, {Qt::CTRL | Qt::ALT | Qt::Key_C});
    connect(copyPathAction, &QAction::triggered, this, &KFindTreeView::copySelectionPath);

    QAction *openFolder = new QAction(QIcon::fromTheme(QStringLiteral("window-new")), i18n("&Open containing folder(s)"), this);
    connect(openFolder, &QAction::triggered, this, &KFindTreeView::openContainingFolder);
    m_actionCollection->addAction(QStringLiteral("openfolder"), openFolder);

    QAction *del = new QAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("&Delete"), this);
    connect(del, &QAction::triggered, this, &KFindTreeView::deleteSelectedFiles);
    KActionCollection::setDefaultShortcut(del, Qt::SHIFT | Qt::Key_Delete);

    QAction *trash = new QAction(QIcon::fromTheme(QStringLiteral("user-trash")), i18n("&Move to Trash"), this);
    connect(trash, &QAction::triggered, this, &KFindTreeView::moveToTrashSelectedFiles);
    KActionCollection::setDefaultShortcut(trash, Qt::Key_Delete);
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

int KFindTreeView::itemCount() const
{
    return m_model->rowCount();
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
        delete m_contextMenu;
        m_contextMenu = nullptr;
    }
    m_model->removeItem(url);
}

bool KFindTreeView::isInserted(const QUrl &url) const
{
    return m_model->isInserted(url);
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

void KFindTreeView::copySelectionPath()
{
    const auto selectedIndexes = m_proxyModel->mapSelectionToSource(selectionModel()->selection()).indexes();
    if (selectedIndexes.isEmpty()) {
        return;
    }

    const auto item = m_model->itemAtIndex(selectedIndexes.at(0)).getFileItem();

    QString path = item.localPath();
    if (path.isEmpty()) {
        path = item.url().toDisplayString();
    }

    QClipboard *clipboard = QApplication::clipboard();
    if (!clipboard) {
        return;
    }
    clipboard->setText(path);
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
        stream.setEncoding(QStringConverter::System);

        const QList<KFindItem> itemList = m_model->getItemList();
        if (dialog.selectedMimeTypeFilter() == QLatin1String("text/html")) {
            stream << QString::fromLatin1("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\""
                                          "\"http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd\"><html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
                                          "<head>\n"
                                          "<title>%2</title></head>\n"
                                          "<meta charset=\"%1\">\n"
                                          "<body>\n<h1>%2</h1>\n"
                                          "<dl>\n")
                .arg(QString::fromLatin1(QTextCodec::codecForLocale()->name()), i18n("KFind Results File"));

            for (const KFindItem &item : itemList) {
                const KFileItem fileItem = item.getFileItem();
                stream << QStringLiteral("<dt><a href=\"%1\">%2</a></dt>\n").arg(
                    fileItem.url().url(), fileItem.url().toDisplayString());
            }
            stream << QStringLiteral("</dl>\n</body>\n</html>\n");
        } else {
            for (const KFindItem &item : itemList) {
                stream << item.getFileItem().url().url() << Qt::endl;
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
    for (const QModelIndex &index : selected) {
        if (index.column() == 0) {
            KFindItem item = m_model->itemAtIndex(index);
            if (item.isValid()) {
                auto *job = new KIO::OpenUrlJob(item.getFileItem().targetUrl());
                job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
                job->start();
            }
        }
    }
}

void KFindTreeView::slotExecute(const QModelIndex &index)
{
    if ((m_mouseButtons & Qt::LeftButton) && QApplication::keyboardModifiers() == Qt::NoModifier) {
        if (!index.isValid()) {
            return;
        }

        QModelIndex realIndex = m_proxyModel->mapToSource(index);
        if (!realIndex.isValid()) {
            return;
        }

        const KFindItem item = m_model->itemAtIndex(realIndex);
        if (item.isValid()) {
            auto *job = new KIO::OpenUrlJob(item.getFileItem().targetUrl());
            job->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
            job->start();
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

    for (const QModelIndex &index : selected) {
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

    QAction *copyLocationAction = m_actionCollection->action(QStringLiteral("copy_location"));
    // It's not worth it to track the selection and update the action live.
    copyLocationAction->setEnabled(fileList.count() == 1);
    m_contextMenu->addAction(copyLocationAction);

    //m_contextMenu->addAction(m_actionCollection->action(QLatin1String("del")));
    m_contextMenu->addAction(m_actionCollection->action(QStringLiteral("trash")));
    m_contextMenu->addSeparator();

    // Open With...
    KFileItemActions menuActions;
    KFileItemListProperties fileProperties(fileList);
    menuActions.setItemListProperties(fileProperties);
    menuActions.insertOpenWithActionsTo(nullptr, m_contextMenu, QStringList());
    // 'Actions' submenu
    menuActions.addActionsTo(m_contextMenu);

    m_contextMenu->addSeparator();

    // Properties
    if (KPropertiesDialog::canDisplay(fileList)) {
        QAction *act = new QAction(m_contextMenu);
        act->setText(i18n("&Properties"));
        QObject::connect(act, &QAction::triggered, this, [this, fileList]() {
            KPropertiesDialog::showDialog(fileList, this, false /*non modal*/);
        });
        m_contextMenu->addAction(act);
    }

    m_contextMenu->exec(this->mapToGlobal(p));
}

QList<QUrl> KFindTreeView::selectedUrls() const
{
    QList<QUrl> uris;

    const QModelIndexList indexes = m_proxyModel->mapSelectionToSource(selectionModel()->selection()).indexes();
    for (const QModelIndex &index : indexes) {
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

    using Iface = KIO::AskUserActionInterface;
    auto *trashJob = new KIO::DeleteOrTrashJob(uris, Iface::Delete, Iface::ForceConfirmation, this);
    trashJob->start();
}

void KFindTreeView::moveToTrashSelectedFiles()
{
    QList<QUrl> uris = selectedUrls();
    if (uris.isEmpty()) {
        return;
    }

    using Iface = KIO::AskUserActionInterface;
    auto *trashJob = new KIO::DeleteOrTrashJob(uris, Iface::Trash, Iface::ForceConfirmation, this);
    trashJob->start();
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

void KFindTreeView::dragMoveEvent(QDragMoveEvent *e)
{
    e->accept();
}

//END KFindTreeView

#include "moc_kfindtreeview.cpp"
