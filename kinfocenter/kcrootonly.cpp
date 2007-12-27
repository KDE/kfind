/*
  Copyright (c) 2001 Waldo Bastian <bastian@kde.org>

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
#include "kcrootonly.h"

#include <klocale.h>

#include <QLabel>
#include <QVBoxLayout>

KCRootOnly::KCRootOnly(const KComponentData &componentData, QWidget *parent)
  : KCModule( componentData, parent )
{
   QVBoxLayout *layout=new QVBoxLayout(this);
   QLabel *label = new QLabel(i18n("<big>You need super user privileges to run this control module.</big><br />"
                                    "Click on the \"Administrator Mode\" button below."), this);
   layout->addWidget(label);
   label->setAlignment(Qt::AlignCenter);
   label->setTextFormat(Qt::RichText);
   label->setMinimumSize(label->sizeHint());
}


