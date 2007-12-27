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

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "helpwidget.h"
#include "global.h"
#include "quickhelp.h"

#include <klocale.h>
#include <kdebug.h>
#include <kprocess.h>
#include <kapplication.h>
#include <krun.h>
#include <ktoolinvocation.h>

#include <QVBoxLayout>

#include "helpwidget.moc"

HelpWidget::HelpWidget(QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *l = new QVBoxLayout(this);

  _browser = new QuickHelp(this);
  connect(_browser, SIGNAL(urlClick(const QString &)),
	  SLOT(urlClicked(const QString &)));
  connect(_browser, SIGNAL(mailClick(const QString &,const QString &)),
	  SLOT(mailClicked(const QString &,const QString &)));

  l->addWidget(_browser);

  setBaseText();
}

void HelpWidget::setText( const QString& docPath, const QString& text)
{
  docpath = docPath;
  if (text.isEmpty() && docPath.isEmpty())
    setBaseText();
  else if (docPath.isEmpty())
    _browser->setText(text);
  else
  {
    QByteArray a = docPath.toLocal8Bit();
    QString path = QString::fromLocal8Bit (a.data(), a.size());

    _browser->setText(text + i18n("<p>Use the \"Whats This\" (Shift+F1) to get help on specific options.</p><p>To read the full manual click <a href=\"%1\">here</a>.</p>",
		       path));
  }
}

void HelpWidget::setBaseText()
{
   _browser->setText(i18n("<h1>KDE Info Center</h1>"
			 "There is no quick help available for the active info module."
			 "<br /><br />"
			 "Click <a href = \"kinfocenter/index.html\">here</a> to read the general Info Center manual.") );
}

void HelpWidget::urlClicked(const QString & _url)
{
    KUrl url(KUrl("help:/"), _url);

    if (url.protocol() == "help" || url.protocol() == "man" || url.protocol() == "info") {
        KProcess::startDetached("khelpcenter", QStringList() << url.url());
    } else {
        new KRun(url, this);
    }
}

void HelpWidget::mailClicked(const QString &,const QString & addr)
{
  KToolInvocation::invokeMailer(addr, QString());
}
