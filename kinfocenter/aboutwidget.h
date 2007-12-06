/*
  Copyright (c) 2000,2001 Matthias Elter <elter@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef __aboutwidget_h__
#define __aboutwidget_h__

#include <QWidget>
#include <Qt3Support/Q3CheckListItem>
#include <QMap>
#include <QPixmap>
#include <kvbox.h>

class QPixmap;
class ConfigModule;
class KHTMLPart;
class KUrl;

class AboutWidget : public KHBox
{
  Q_OBJECT

public:
  explicit AboutWidget(QWidget *parent, Q3ListViewItem* category=0, const QString &caption=QString());

    /**
     * Set a new category without creating a new AboutWidget if there is
     * one visible already (reduces flicker)
     */
    void setCategory( Q3ListViewItem* category, const QString& caption);

Q_SIGNALS:
    void moduleSelected(ConfigModule *);

private Q_SLOTS:
    void slotModuleLinkClicked( const KUrl& );

private:
    /**
     * Update the pixmap to be shown. Called from resizeEvent and from
     * setCategory.
     */
    void updatePixmap();

    bool    _moduleList;
    Q3ListViewItem* _category;
    QString _caption;
    KHTMLPart *_viewer;
    QMap<QString,ConfigModule*> _moduleMap;
};

#endif
