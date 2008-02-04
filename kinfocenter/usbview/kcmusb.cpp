/***************************************************************************
 *   Copyright (C) 2001 by Matthias Hoelzer-Kluepfel <mhk@caldera.de>      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <Qt3Support/Q3GroupBox>
#include <Qt3Support/Q3Header>
#include <QLayout>
#include <Qt3Support/Q3CheckListItem>
#include <QSplitter>
#include <QtGui/QTextEdit>
#include <QTimer>
//Added by qt3to4:
#include <QVBoxLayout>
#include <QList>

#include <kaboutdata.h>
#include <kdialog.h>

#include "usbdevices.h"
#include <KPluginFactory>
#include <KPluginLoader>
#include "kcmusb.moc"

K_PLUGIN_FACTORY(USBFactory,
        registerPlugin<USBViewer>();
        )
K_EXPORT_PLUGIN(USBFactory("kcmusb"))

USBViewer::USBViewer(QWidget *parent, const QVariantList &)
  : KCModule(USBFactory::componentData(), parent)
{
  setButtons(Help);

  setQuickHelp( i18n("<h1>USB Devices</h1> This module allows you to see"
     " the devices attached to your USB bus(es)."));

  QVBoxLayout *vbox = new QVBoxLayout(this);
  vbox->setMargin(0);
  vbox->setSpacing(KDialog::spacingHint());

  Q3GroupBox *gbox = new Q3GroupBox(i18n("USB Devices"), this);
  gbox->setColumnLayout( 0, Qt::Horizontal );
  vbox->addWidget(gbox);

  QVBoxLayout *vvbox = new QVBoxLayout();
  gbox->layout()->addItem( vvbox );
  vvbox->setSpacing(KDialog::spacingHint());

  QSplitter *splitter = new QSplitter(gbox);
  vvbox->addWidget(splitter);

  _devices = new Q3ListView(splitter);
  _devices->addColumn(i18n("Device"));
  _devices->setRootIsDecorated(true);
  _devices->header()->hide();
  _devices->setMinimumWidth(200);
  _devices->setColumnWidthMode(0, Q3ListView::Maximum);

  QList<int> sizes;
  sizes.append(200);
  splitter->setSizes(sizes);

  _details = new QTextEdit(splitter);
  _details->setReadOnly(true);
  splitter->setResizeMode(_devices, QSplitter::KeepSize);

  QTimer *refreshTimer = new QTimer(this);
  // 1 sec seems to be a good compromise between latency and polling load.
  refreshTimer->start(1000);

  connect(refreshTimer, SIGNAL(timeout()), SLOT(refresh()));
  connect(_devices, SIGNAL(selectionChanged(Q3ListViewItem*)),
	  this, SLOT(selectionChanged(Q3ListViewItem*)));

  KAboutData *about =
  new KAboutData(I18N_NOOP("kcmusb"), 0, ki18n("KDE USB Viewer"),
                0, KLocalizedString(), KAboutData::License_GPL,
                ki18n("(c) 2001 Matthias Hoelzer-Kluepfel"));

  about->addAuthor(ki18n("Matthias Hoelzer-Kluepfel"), KLocalizedString(), "mhk@kde.org");
  about->addCredit(ki18n("Leo Savernik"), ki18n("Live Monitoring of USB Bus"), "l.savernik@aon.at");
  setAboutData( about );

  load();
}

void USBViewer::load()
{
  _items.clear();
  _devices->clear();

  refresh();
}

static quint32 key( USBDevice &dev )
{
  return dev.bus()*256 + dev.device();
}

static quint32 key_parent( USBDevice &dev )
{
  return dev.bus()*256 + dev.parent();
}

static void delete_recursive( Q3ListViewItem *item, const Q3IntDict<Q3ListViewItem> &new_items )
{
  if (!item)
	return;

  Q3ListViewItemIterator it( item );
  while ( it.current() ) {
        if (!new_items.find(it.current()->text(1).toUInt())) {
		delete_recursive( it.current()->firstChild(), new_items);
		delete it.current();
	}
	++it;
  }
}

void USBViewer::refresh()
{
  Q3IntDict<Q3ListViewItem> new_items;

  if (!USBDevice::parse("/proc/bus/usb/devices"))
    USBDevice::parseSys("/sys/bus/usb/devices");

  int level = 0;
  bool found = true;

  while (found)
    {
      found = false;

      Q3PtrListIterator<USBDevice> it(USBDevice::devices());
      for ( ; it.current(); ++it)
	if (it.current()->level() == level)
	  {
	    quint32 k = key(*it.current());
	    if (level == 0)
	      {
		Q3ListViewItem *item = _items.find(k);
		if (!item) {
		    item = new Q3ListViewItem(_devices,
				it.current()->product(),
				QString::number(k));
		}
		new_items.insert(k, item);
		found = true;
	      }
	    else
	      {
		Q3ListViewItem *parent = new_items.find(key_parent(*it.current()));
		if (parent)
		  {
		    Q3ListViewItem *item = _items.find(k);

		    if (!item) {
		        item = new Q3ListViewItem(parent,
				    it.current()->product(),
				    QString::number(k) );
		    }
		    new_items.insert(k, item);
		    parent->setOpen(true);
		    found = true;
		  }
	      }
	  }

      ++level;
    }

    // recursive delete all items not in new_items
    delete_recursive( _devices->firstChild(), new_items );

    _items = new_items;

    if (!_devices->selectedItem())
        selectionChanged(_devices->firstChild());
}


void USBViewer::selectionChanged(Q3ListViewItem *item)
{
  if (item)
    {
      quint32 busdev = item->text(1).toUInt();
      USBDevice *dev = USBDevice::find(busdev>>8, busdev&255);
      if (dev)
	{
	  _details->setHtml(dev->dump());
	  return;
	}
    }
  _details->clear();
}


