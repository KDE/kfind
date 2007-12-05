/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 
*/               

#include "quickhelp.h"
#include "quickhelp.moc"

#include <QFrame>

QuickHelp::QuickHelp(QWidget *parent)
    : KTextBrowser( parent )
{
  setFrameStyle( QFrame::Panel | QFrame::Sunken );
  setFocusPolicy( Qt::ClickFocus );
  setHorizontalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
  setNotifyClick(true);
}

void QuickHelp::setText(const QString &text)
{
  QString s(text);
  //lukas: no need to change the font -- it breaks for
  //those not having it in the right encoding!
  //s.replace(QRegExp("<[Hh]1>"),"<font face=utopia><h1>");
  //s.replace(QRegExp("</[Hh]1>"),"</h1></font>");
  KTextBrowser::setHtml(s);
}

