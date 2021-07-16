/*
    kfindtreeview.h
    SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

*/

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
public:
    explicit KFindItem(const KFileItem & = KFileItem(), const QString &subDir = QString(), const QString &matchingLine = QString());

    QVariant data(int column, int role) const;

    KFileItem getFileItem() const;

    bool isValid() const;

private:
    KFileItem m_fileItem;
    QString m_matchingLine;
    QString m_subDir;
    QString m_permission;
    QIcon m_icon;
};

class KFindItemModel : public QAbstractTableModel
{
public:
    explicit KFindItemModel(KFindTreeView *parent);

    void insertFileItems(const QList< QPair<KFileItem, QString> > &);

    void removeItem(const QUrl &);
    bool isInserted(const QUrl &);

    void clear();

    Qt::DropActions supportedDropActions() const override;

    Qt::ItemFlags flags(const QModelIndex &) const override;
    QMimeData *mimeData(const QModelIndexList &) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    KFindItem itemAtIndex(const QModelIndex &index) const;

    QList<KFindItem> getItemList() const;

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
    ~KFindTreeView() override;

    void beginSearch(const QUrl &baseUrl);
    void endSearch();

    void insertItems(const QList< QPair<KFileItem, QString> > &);
    void removeItem(const QUrl &url);

    bool isInserted(const QUrl &url) const;

    QString reducedDir(const QString &fullDir);

    int itemCount() const;

    QList<QUrl> selectedUrls() const;

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
    void dragMoveEvent(QDragMoveEvent *e) override;

Q_SIGNALS:
    void resultSelected(bool);

private:
    void resizeToContents();

    QList<QPersistentModelIndex> selectedVisibleIndexes();

    QDir m_baseDir;

    KFindItemModel *const m_model;
    KFindSortFilterProxyModel *const m_proxyModel;
    KActionCollection *m_actionCollection = nullptr;
    QMenu *m_contextMenu = nullptr;

    Qt::MouseButtons m_mouseButtons;

    KfindDlg *const m_kfindDialog;
};

#endif
