/* This file is part of the KDE project
   Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __KWRITE_MAIN_H__
#define __KWRITE_MAIN_H__

#include <kparts/mainwindow.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

class KAction;
class KToggleAction;
class KSelectAction;
class KRecentFilesAction;

class KWrite : public KParts::MainWindow
{
  Q_OBJECT

  public:
    KWrite(KTextEditor::Document * = 0L);
    ~KWrite();

    void init(); //initialize caption, status and show

    void loadURL(const KURL &url);

    KTextEditor::View *kateView() const { return m_kateView; }

  private:
    virtual bool queryClose();
    virtual bool queryExit();

    void setupEditWidget(KTextEditor::Document *);
    void setupActions();
    void setupStatusBar();

    virtual void dragEnterEvent( QDragEnterEvent * );
    virtual void dropEvent( QDropEvent * );

    KTextEditor::View * m_kateView;
    KRecentFilesAction * m_recentFiles;
    KToggleAction * m_paShowPath;
    KToggleAction * m_paShowStatusBar;

  public slots:
    void slotNew();
    void slotFlush ();
    void slotOpen();
    void slotOpen( const KURL& url);
    void newView();
    void toggleStatusBar();
    void editKeys();
    void editToolbars();
    void changeEditor();

  public slots:
    void printNow();
    void printDlg();

    void newStatus(const QString &msg);
    void newCaption();

    void slotDropEvent(QDropEvent *);

    void slotEnableActions( bool enable );

  //config file functions
  public:
    //common config
    void readConfig(KConfig *);
    void writeConfig(KConfig *);
    //config file
    void readConfig();

  public slots:
    void writeConfig();

  //session management
  public:
    void restore(KConfig *,int);
    static void restore();

  private:
    virtual void readProperties(KConfig *);
    virtual void saveProperties(KConfig *);
    virtual void saveGlobalProperties(KConfig *);

  private:
    QString encoding;
    static QPtrList<KTextEditor::Document> docList;
};

#endif

