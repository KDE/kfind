/*******************************************************************
* kfindtreeview.h
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

#ifndef KFINDTREEVIEW__H
#define KFINDTREEVIEW__H

#include <KFileItem>

#include <QAbstractTableModel>
#include <QDir>
#include <QDragMoveEvent>
#include <QIcon>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QUrl>

class QMenu;
class KFindTreeView;
class KActionCollection;
class KfindDlg;

class KFindItem
{
    Q_OBJECT
public:
    explicit KFindItem(const KFileItem & = KFileItem(), const QString &subDir = QString(), const QString &matchingLine = QString());

    QVariant data(int column, int role) const;

    KFileItem getFileItem() const
    {
        return m_fileItem;
    }

    bool isValid() const
    {
        return !m_fileItem.isNull();
    }

private:
    KFileItem m_fileItem;
    QString m_matchingLine;
    QString m_subDir;
    QString m_permission;
    QIcon m_icon;
};

class KFindItemModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit KFindItemModel(KFindTreeView *parent);

    void insertFileItems(const QList< QPair<KFileItem, QString> > &);

    void removeItem(const QUrl &);
    bool isInserted(const QUrl &);

    void clear();

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::CopyAction | Qt::MoveAction;
    }

    Qt::ItemFlags flags(const QModelIndex &) const override;
    QMimeData *mimeData(const QModelIndexList &) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent);
        return 6;
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    KFindItem itemAtIndex(const QModelIndex &index) const;

    QList<KFindItem> getItemList() const
    {
        return m_itemList;
    }

private:
    QList<KFindItem> m_itemList;
    KFindTreeView *m_view;
};

class KFindSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit KFindSortFilterProxyModel(QObject *parent = nullptr)
        : QSortFilterProxyModel(parent)
    {
    }

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};

class KFindTreeView : public QTreeView
{
    Q_OBJECT
public:
    explicit KFindTreeView(QWidget *parent, KfindDlg *findDialog);
    ~KFindTreeView();

    void beginSearch(const QUrl &baseUrl);
    void endSearch();

    void insertItems(const QList< QPair<KFileItem, QString> > &);
    void removeItem(const QUrl &url);

    bool isInserted(const QUrl &url)
    {
        return m_model->isInserted(url);
    }

    QString reducedDir(const QString &fullDir);

    int itemCount()
    {
        return m_model->rowCount();
    }

    QList<QUrl> selectedUrls();

public Q_SLOTS:
    void copySelection();
    void contextMenuRequested(const QPoint &p);
    void saveResults();

private Q_SLOTS:
    void deleteSelectedFiles();
    void moveToTrashSelectedFiles();

    void slotExecute(const QModelIndex &index);
    void slotExecuteSelected();

    void openContainingFolder();

    void reconfigureMouseSettings();
    void updateMouseButtons();

protected:
    void dragMoveEvent(QDragMoveEvent *e) override
    {
        e->accept();
    }

Q_SIGNALS:
    void resultSelected(bool);

private:
    void resizeToContents();

    QList<QPersistentModelIndex> selectedVisibleIndexes();

    QDir m_baseDir;

    KFindItemModel *m_model;
    KFindSortFilterProxyModel *m_proxyModel;
    KActionCollection *m_actionCollection;
    QMenu *m_contextMenu;

    Qt::MouseButtons m_mouseButtons;

    KfindDlg *m_kfindDialog;
};

#endif
