/***************************************************************************
 *   Copyright (C) 2001 by Matthias Hoelzer-Kluepfel <mhk@caldera.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef _KCMUSB_H
#define _KCMUSB_H

#include <Qt3Support/Q3IntDict>

#define KDE3_SUPPORT
#include <kcmodule.h>
#undef KDE3_SUPPORT

class Q3ListView;
class Q3ListViewItem;
class QTextEdit;


class USBViewer : public KCModule
{
  Q_OBJECT

public:

  explicit USBViewer(QWidget *parent = 0L, const QVariantList &list=QVariantList() );

  void load();

protected Q_SLOTS:

  void selectionChanged(Q3ListViewItem *item);
  void refresh();

private:

  Q3IntDict<Q3ListViewItem> _items;
  Q3ListView *_devices;
  QTextEdit *_details;
};


#endif
