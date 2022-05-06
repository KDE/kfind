/*
    kfinddlg.h

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#ifndef KFINDDLG_H
#define KFINDDLG_H

#include <QDialog>
#include <QLabel>

#include <KDirWatch>
#include <KFileItem>

QT_FORWARD_DECLARE_CLASS(QDir)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QUrl)

class KQuery;
class KfindTabWidget;
class KFindTreeView;
class QPushButton;
class QStatusBar;

class KfindDlg : public QDialog
{
    Q_OBJECT

public:
    explicit KfindDlg(const QUrl &url, QWidget *parent = nullptr);
    ~KfindDlg() override;
    void copySelection();

    void setStatusMsg(const QString &);
    void setProgressMsg(const QString &);

private:
    /*Return a QStringList of all subdirs of d*/
    QStringList getAllSubdirs(QDir d);

public Q_SLOTS:
    void startSearch();
    void stopSearch();
    void newSearch();
    void addFiles(const QList< QPair<KFileItem, QString> > &);
    void setFocus();
    void slotResult(int);
//  void slotSearchDone();
    void  about();
    void slotDeleteItem(const QString &);
    void slotNewItems(const QString &);

    void finishAndClose();

Q_SIGNALS:
    void haveResults(bool);
    void resultSelected(bool);

private:
    KfindTabWidget *tabWidget = nullptr;
    KFindTreeView *win = nullptr;
    QPushButton *m_saveAsButton = nullptr;
    QPushButton *m_stopButton = nullptr;
    QPushButton *m_findButton = nullptr;
    QStatusBar *mStatusBar = nullptr;
    QLabel *m_labelStatus = nullptr;
    QLabel *m_labelProgress = nullptr;

    bool isResultReported = false;
    KQuery *query = nullptr;
    KDirWatch *dirwatch = nullptr;
};

#endif
