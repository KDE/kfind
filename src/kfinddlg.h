/*******************************************************************
* kfinddlg.h
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

#ifndef KFINDDLG_H
#define KFINDDLG_H

#include <QDialog>
#include <QLabel>

#include <KDirLister>
#include <KDirWatch>

QT_FORWARD_DECLARE_CLASS(QDir)
QT_FORWARD_DECLARE_CLASS(QString)
QT_FORWARD_DECLARE_CLASS(QUrl)

class KQuery;
class KFileItem;
class KfindTabWidget;
class KFindTreeView;
class QLabel;
class QPushButton;
class QStatusBar;

class KfindDlg : public QDialog
{
    Q_OBJECT

public:
    explicit KfindDlg(const QUrl &url, QWidget *parent = nullptr);
    ~KfindDlg();
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
    KfindTabWidget *tabWidget;
    KFindTreeView *win;
    QPushButton *m_saveAsButton;
    QPushButton *m_stopButton;
    QPushButton *m_findButton;
    QStatusBar *mStatusBar;
    QLabel *m_labelStatus;
    QLabel *m_labelProgress;

    bool isResultReported;
    KQuery *query;
    KDirLister *dirlister;
    KDirWatch *dirwatch;
};

#endif
