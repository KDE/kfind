/***********************************************************************
 *
 *  KfindDlg.h
 *
 ***********************************************************************/

#ifndef KFINDDLG_H
#define KFINDDLG_H

#include <kdialogbase.h>
#include <kdirlister.h>
#include <kdirwatch.h>

class QString;

class KQuery;
class KURL;
class KFileItem;
class KfindTabWidget;
class KfindWindow;
class KStatusBar;
class KAboutApplication;

class KfindDlg: public KDialogBase
{
Q_OBJECT

public:
  KfindDlg(const KURL & url, QWidget * parent = 0, const char * name = 0);
  ~KfindDlg();
  void copySelection();

  void setStatusMsg(const QString &);
  void setProgressMsg(const QString &);

private:
  void closeEvent(QCloseEvent *);
  /*Return a QStringList of all subdirs of d*/
  QStringList getAllSubdirs(QDir d);

public slots:
  void startSearch();
  void stopSearch();
  void newSearch();
  void addFile(const KFileItem* item, const QString& matchingLine);
  void setFocus();
  void slotResult(int);
//  void slotSearchDone();
  void  about ();
  void slotDeleteItem(const QString&);
  void slotNewItems( const QString&  );

signals:
  void haveResults(bool);
  void resultSelected(bool);

private:
  KfindTabWidget *tabWidget;
  KfindWindow * win;
  KAboutApplication * aboutWin;

  bool isResultReported;
  KQuery *query;
  KStatusBar *mStatusBar;
  KDirLister *dirlister;
  KDirWatch *dirwatch;
};

#endif


