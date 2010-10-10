/**
 * kcookiesmanagement.cpp - Cookies manager
 *
 * Copyright 2000-2001 Marco Pinelli <pinmc@orion.it>
 * Copyright (c) 2000-2001 Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

// Own
#include "kcookiesmanagement.h"

// std
#include <assert.h>

// Qt
#include <QtGui/QApplication>
#include <QtGui/QLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtCore/QTimer>
#include <QtGui/QToolButton>
#include <QtGui/QBoxLayout>
#include <QtCore/QList>

#include <QtDBus/QtDBus>

// KDE
#include <kdebug.h>
#include <klocale.h>
#include <kdialog.h>
#include <kiconloader.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <kdatetime.h>

// Local
#include "kcookiesmain.h"
#include "kcookiespolicies.h"

QString tolerantFromAce(const QByteArray& _domain);

struct CookieProp
{
    QString host;
    QString name;
    QString value;
    QString domain;
    QString path;
    QString expireDate;
    QString secure;
    bool allLoaded;
};

CookieListViewItem::CookieListViewItem(QTreeWidget *parent, const QString &dom)
                   :QTreeWidgetItem(parent)
{
    init( 0, dom );
}

CookieListViewItem::CookieListViewItem(QTreeWidgetItem *parent, CookieProp *cookie)
                   :QTreeWidgetItem(parent)
{
    init( cookie );
}

CookieListViewItem::~CookieListViewItem()
{
    delete mCookie;
}

void CookieListViewItem::init( CookieProp* cookie, const QString &domain,
                               bool cookieLoaded )
{
    mCookie = cookie;
    mDomain = domain;
    mCookiesLoaded = cookieLoaded;

    if (mCookie)
    {
        kDebug() << "Host:" << mCookie->host << "Domain:" << domain;
        if (mDomain.isEmpty())
            setText(0, tolerantFromAce(mCookie->host.toLatin1()));
        else
            setText(0, tolerantFromAce(mDomain.toLatin1()));
        setText(1, mCookie->name);
    }
    else
    {
        QString siteName;
        if (mDomain.startsWith(QLatin1Char('.')))
            siteName = mDomain.mid(1);
        else
            siteName = mDomain;
        setText(0, tolerantFromAce(siteName.toLatin1()));
    }
}

CookieProp* CookieListViewItem::leaveCookie()
{
    CookieProp *ret = mCookie;
    mCookie = 0;
    return ret;
}

KCookiesManagement::KCookiesManagement(const KComponentData &componentData, QWidget *parent)
                   : KCModule(componentData, parent)
{
  // Toplevel layout
  QVBoxLayout* mainLayout = new QVBoxLayout(this);
  mainLayout->setMargin(0);

  dlg = new KCookiesManagementDlgUI (this);

  dlg->kListViewSearchLine->setTreeWidget(dlg->lvCookies);

  dlg->lvCookies->setColumnWidth(0, 150);

  mainLayout->addWidget(dlg);

  connect(dlg->lvCookies, SIGNAL(itemExpanded(QTreeWidgetItem*)), SLOT(getCookies(QTreeWidgetItem*)) );
  connect(dlg->lvCookies, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), SLOT(showCookieDetails(QTreeWidgetItem*)) );

  connect(dlg->pbDelete, SIGNAL(clicked()), SLOT(deleteCookie()));
  connect(dlg->pbDeleteAll, SIGNAL(clicked()), SLOT(deleteAllCookies()));
  connect(dlg->pbReload, SIGNAL(clicked()), SLOT(getDomains()));
  connect(dlg->pbPolicy, SIGNAL(clicked()), SLOT(doPolicy()));

  connect(dlg->lvCookies, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), SLOT(doPolicy()));
  m_bDeleteAll = false;
  mainWidget = parent;
}

KCookiesManagement::~KCookiesManagement()
{
}

void KCookiesManagement::load()
{
  reset();
  getDomains();
}

void KCookiesManagement::save()
{
  // If delete all cookies was requested!
  if(m_bDeleteAll)
  {
      QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
      QDBusReply<void> reply = kded.call( "deleteAllCookies" );
    if (!reply.isValid())
    {
      QString caption = i18n ("D-Bus Communication Error");
      QString message = i18n ("Unable to delete all the cookies as requested.");
      KMessageBox::sorry (this, message, caption);
      return;
    }

    m_bDeleteAll = false; // deleted[Cookies|Domains] have been cleared yet
  }

  // Certain groups of cookies were deleted...
  QStringList::Iterator dIt = deletedDomains.begin();
  while( dIt != deletedDomains.end() )
  {
    QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
    QDBusReply<void> reply = kded.call( "deleteCookiesFromDomain",( *dIt ) );
    if( !reply.isValid() )
    {
      QString caption = i18n ("D-Bus Communication Error");
      QString message = i18n ("Unable to delete cookies as requested.");
      KMessageBox::sorry (this, message, caption);
      return;
    }

    dIt = deletedDomains.erase(dIt);
  }

  // Individual cookies were deleted...
  bool success = true; // Maybe we can go on...
  QHashIterator<QString, CookiePropList> cookiesDom(deletedCookies);
  while(cookiesDom.hasNext())
  {
    cookiesDom.next();
    CookiePropList list = cookiesDom.value();
    foreach(CookieProp *cookie, list)
    {
        QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
        QDBusReply<void> reply = kded.call( "deleteCookie", cookie->domain,
                                            cookie->host, cookie->path,
                                            cookie->name );
        if( !reply.isValid() )
      {
        success = false;
        break;
      }

      list.removeOne(cookie);
    }

    if(!success)
      break;

    deletedCookies.remove(cookiesDom.key());
  }

  emit changed( false );
}

void KCookiesManagement::defaults()
{
  reset();
  getDomains();
  emit changed (false);
}

void KCookiesManagement::reset(bool deleteAll)
{
  if ( !deleteAll )
    m_bDeleteAll = false;

  clearCookieDetails();
  dlg->lvCookies->clear();
  deletedDomains.clear();
  deletedCookies.clear();
  dlg->pbDelete->setEnabled(false);
  dlg->pbDeleteAll->setEnabled(false);
  dlg->pbPolicy->setEnabled(false);
}

void KCookiesManagement::clearCookieDetails()
{
  dlg->leName->clear();
  dlg->leValue->clear();
  dlg->leDomain->clear();
  dlg->lePath->clear();
  dlg->leExpires->clear();
  dlg->leSecure->clear();
}

QString KCookiesManagement::quickHelp() const
{
  return i18n("<h1>Cookie Management Quick Help</h1>" );
}

void KCookiesManagement::getDomains()
{
  QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
  QDBusReply<QStringList> reply = kded.call( "findDomains" );

  if( !reply.isValid() )
  {
    QString caption = i18n ("Information Lookup Failure");
    QString message = i18n ("Unable to retrieve information about the "
                            "cookies stored on your computer.");
    KMessageBox::sorry (this, message, caption);
    return;
  }

  if ( dlg->lvCookies->topLevelItemCount() > 0 )
  {
    reset();
    dlg->lvCookies->setCurrentItem( 0L );
  }

  CookieListViewItem *dom;
  QListIterator<QString> it (reply.value());
  while (it.hasNext())
  {
    const QString domain = it.next();
    if (dlg->lvCookies->findItems(domain, Qt::MatchFixedString).isEmpty()) {
        dom = new CookieListViewItem(dlg->lvCookies, domain);
        dom->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
  }

  // are there any cookies?
  dlg->pbDeleteAll->setEnabled(dlg->lvCookies->topLevelItemCount() > 0);
  dlg->lvCookies->sortItems(0, Qt::AscendingOrder);
}

Q_DECLARE_METATYPE( QList<int> )

void KCookiesManagement::getCookies(QTreeWidgetItem *item)
{
  CookieListViewItem* cookieDom = static_cast<CookieListViewItem*>(item);
  if ( !cookieDom || cookieDom->cookiesLoaded() )
    return;

  QStringList cookies;
  QList<int> fields;
  fields << 0 << 1 << 2 << 3;
  const QString domain = cookieDom->domain();
  kDebug() << "Retreiving cookie for" << domain;

  QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
  QDBusReply<QStringList> reply = kded.call( "findCookies",
                                             QVariant::fromValue( fields ),
                                             domain,
                                             QString(),
                                             QString(),
                                             QString() );

  if (reply.isValid())
      cookies.append(reply.value());

  if (domain.startsWith(QLatin1Char('.')))
  {
    reply =  kded.call( "findCookies",
                        QVariant::fromValue( fields ),
                        domain.mid(1),
                        QString(),
                        QString(),
                        QString() );
    if (reply.isValid())
        cookies.append(reply.value());
  }

  QStringListIterator it(cookies);
  while(it.hasNext())
  {
    CookieProp *details = new CookieProp;
    details->domain = it.next();
    details->path = it.next();
    details->name = it.next();
    details->host = it.next();
    details->allLoaded = false;
    new CookieListViewItem(item, details);
  }

  if (!cookies.isEmpty())
  {
    static_cast<CookieListViewItem*>(item)->setCookiesLoaded();
    dlg->kListViewSearchLine->updateSearch();
  }
}

bool KCookiesManagement::cookieDetails(CookieProp *cookie)
{
  QList<int> fields;
  fields << 4 << 5 << 7;

  QDBusInterface kded("org.kde.kded", "/modules/kcookiejar", "org.kde.KCookieServer", QDBusConnection::sessionBus());
  QDBusReply<QStringList> reply = kded.call( "findCookies",
                                             QVariant::fromValue( fields ),
                                             cookie->domain,
                                             cookie->host,
                                             cookie->path,
                                             cookie->name);
  if( !reply.isValid() )
    return false;

  const QStringList fieldVal = reply.value();

  QStringList::const_iterator c = fieldVal.begin();
  if (c == fieldVal.end()) // empty list, do not crash
    return false;

  cookie->value = *c++;
  qint64 tmp = (*c++).toLongLong();

  if( tmp == 0 )
    cookie->expireDate = i18n("End of session");
  else
  {
    KDateTime expDate;
    expDate.setTime_t(tmp);
    cookie->expireDate = KGlobal::locale()->formatDateTime(expDate);
  }

  tmp = (*c).toUInt();
  cookie->secure = i18n(tmp ? "Yes" : "No");
  cookie->allLoaded = true;
  return true;
}

void KCookiesManagement::showCookieDetails(QTreeWidgetItem* item)
{
  kDebug ();

  if (!item)
    return;

  CookieProp *cookie = static_cast<CookieListViewItem*>(item)->cookie();
  if( cookie )
  {
    if( cookie->allLoaded || cookieDetails(cookie) )
    {
      dlg->leName->setText(cookie->name);
      dlg->leValue->setText(cookie->value);
      dlg->leDomain->setText(cookie->domain);
      dlg->lePath->setText(cookie->path);
      dlg->leExpires->setText(cookie->expireDate);
      dlg->leSecure->setText(cookie->secure);
    }

    dlg->pbPolicy->setEnabled (true);
  }
  else
  {
    clearCookieDetails();
    dlg->pbPolicy->setEnabled(false);
  }

  dlg->pbDelete->setEnabled(true);
}

void KCookiesManagement::doPolicy()
{
  // Get current item
  CookieListViewItem *item = static_cast<CookieListViewItem*>( dlg->lvCookies->currentItem() );

  if( item && item->cookie())
  {
    CookieProp *cookie = item->cookie();

    QString domain = cookie->domain;

    if( domain.isEmpty() )
    {
      CookieListViewItem *parent = static_cast<CookieListViewItem*>( item->parent() );

      if ( parent )
        domain = parent->domain ();
    }

    KCookiesMain* mainDlg =static_cast<KCookiesMain*>( mainWidget );
    // must be present or something is really wrong.
    assert (mainDlg);

    KCookiesPolicies* policyDlg = mainDlg->policyDlg();
    // must be present unless someone rewrote the widget in which case
    // this needs to be re-written as well.
    assert (policyDlg);
    policyDlg->addNewPolicy(domain);
  }
}

void KCookiesManagement::deleteCookie()
{
  QTreeWidgetItem* currentItem = dlg->lvCookies->currentItem();
  CookieListViewItem *item = static_cast<CookieListViewItem*>( currentItem );
  if( item->cookie() )
  {
    CookieListViewItem *parent = static_cast<CookieListViewItem*>(item->parent());
    CookiePropList list = deletedCookies.value(parent->domain());
    list.append(item->leaveCookie());
    deletedCookies.insert(parent->domain(), list);
    delete item;
    if(parent->childCount() == 0)
      delete parent;
  }
  else
  {
    deletedDomains.append(item->domain());
    delete item;
  }

  currentItem = dlg->lvCookies->currentItem();
  if ( currentItem )
  {
    dlg->lvCookies->setCurrentItem( currentItem );
    showCookieDetails( currentItem );
  }
  else
    clearCookieDetails();

  dlg->pbDelete->setEnabled(dlg->lvCookies->currentItem());
  dlg->pbDeleteAll->setEnabled(dlg->lvCookies->topLevelItemCount() > 0);
  dlg->pbPolicy->setEnabled(dlg->lvCookies->currentItem());

  emit changed( true );
}

void KCookiesManagement::deleteAllCookies()
{
  m_bDeleteAll = true;
  reset(true);

  emit changed( true );
}

#include "kcookiesmanagement.moc"