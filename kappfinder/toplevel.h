/*
    KAppfinder, the KDE application finder

    Copyright (c) 2002-2003 Tobias Koenig <tokoe@kde.org>

    Based on code written by Matthias Hoelzer-Kluepfel <hoelzer@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of version 2 of the GNU General Public
    License as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef TOPLEVEL_H
#define TOPLEVEL_H

#include <kdialog.h>

#include <QtCore/QStringList>
#include <QtGui/QTreeWidget>

#include "common.h"

class QProgressBar;
class QLabel;

class TopLevel : public KDialog
{
  Q_OBJECT

  public:
    explicit TopLevel( const QString &destDir, QWidget *parent = 0 );
    ~TopLevel();

  public Q_SLOTS:
    void slotScan();
    void slotCreate();
    void slotSelectAll();
    void slotUnselectAll();

  private:
    QTreeWidgetItem *addGroupItem( QTreeWidgetItem *parent, const QString &relPath,
                                 const QString &name );

    QProgressBar *mProgress;
    QLabel *mSummary;
    QTreeWidget *mListView;
    QString mDestDir;
    QStringList mTemplates;

    QList<AppLnkCache*> mAppCache;

    int mFound, mAdded;
};

#endif
