/***********************************************************************
 *
 *  Kfind.h
 *
 ***********************************************************************/

#ifndef KFIND_H
#define KFIND_H

#include "kftabdlg.h"
#include "kfwin.h"

#include <kprocess.h>

class QPushButton;

class Kfind: public QWidget
{
Q_OBJECT

public:
  Kfind( QWidget * parent = 0 ,const char * name = 0, const char*searchPath = 0);
  ~Kfind();
  void copySelection();

public slots:
  void startSearch();
  void stopSearch();
  void newSearch();
  void handleStdout(KProcess *proc, char *buffer, int buflen);
  void setFocus() { tabWidget->setFocus(); }

signals:
  void  haveResults(bool);
  void  resultSelected(bool);
  void  statusChanged(const char *);
  void  enableSearchButton(bool);
  void  enableStatusBar(bool);

  void open();
  void addToArchive();
  void deleteFile();
  void renameFile();
  void properties();
  void openFolder();
  void saveResults();

protected:

private:
  KShellProcess *findProcess;
  KfindTabWidget *tabWidget;
  KfindWindow * win;

  bool isResultReported;
  bool hasBeenKilled;
  char *iBuffer;
  void setExpanded(bool);
};

#endif

 
