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

#include "aboutwidget.h"
#include "global.h"
#include "modules.h"
#include "moduletreeview.h"

#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>
#include <kcursor.h>
#include <kglobalsettings.h>
#include <khtml_part.h>
#include <kapplication.h>
#include <kvbox.h>

#include <QFile>
#include <QTextStream>

#include "aboutwidget.moc"

static const char kcc_text[] = I18N_NOOP("KDE Control Center");

static const char title_text[] = I18N_NOOP("Configure your desktop environment");

static const char intro_text[] = I18N_NOOP("Welcome to the \"KDE Control Center\", "
                                "a central place to configure your "
                                "desktop environment. "
                                "Select an item from the index on the left "
                                "to load a configuration module.");

static const char kcc_infotext[] = I18N_NOOP("KDE Info Center");

static const char title_infotext[] = I18N_NOOP("Get system and desktop environment information");

static const char intro_infotext[] = I18N_NOOP("Welcome to the \"KDE Info Center\", "
                                "a central place to find information about your "
                                "computer system.");

static const char use_text[] = I18N_NOOP("Click on the \"Help\" tab on the left to view help "
                        "for the active "
                        "control module. Use the \"Search\" tab if you are unsure "
                        "where to look for "
                        "a particular configuration option.");

static const char version_text[] = I18N_NOOP("KDE version:");
static const char user_text[] = I18N_NOOP("User:");
static const char host_text[] = I18N_NOOP("Hostname:");
static const char system_text[] = I18N_NOOP("System:");
static const char release_text[] = I18N_NOOP("Release:");
static const char machine_text[] = I18N_NOOP("Machine:");

AboutWidget::AboutWidget(QWidget *parent, Q3ListViewItem* category, const QString &caption)
   : KHBox(parent),
      _moduleList(false),
      _category(category),
      _caption(caption)
{
    if (_category)
      _moduleList = true;

    setMinimumSize(400, 400);

    // set qwhatsthis help
    this->setWhatsThis( i18n(intro_text));
    _viewer = new KHTMLPart( this );
    _viewer->widget()->setSizePolicy( QSizePolicy::Ignored, QSizePolicy::Ignored );
    connect( _viewer->browserExtension(),
             SIGNAL(openUrlRequest(const KUrl&, const KParts::OpenUrlArguments&, const KParts::BrowserArguments&)),
             this, SLOT(slotModuleLinkClicked(const KUrl&)) );
    updatePixmap();
}

void AboutWidget::setCategory( Q3ListViewItem* category, const QString &caption )
{
  _caption = caption;
  _category = category;
  if ( _category )
    _moduleList = true;
  else
    _moduleList = true;

  // Update the pixmap to be shown:
  updatePixmap();
}

void AboutWidget::updatePixmap()
{
    QString file = KStandardDirs::locate(  "data", "kinfocenter/about/main.html" );
    QFile f( file );
    f.open( QIODevice::ReadOnly );
    QTextStream t(  &f );
    QString res = t.readAll();

    res = res.arg(  KStandardDirs::locate(  "data", "kdeui/about/kde_infopage.css" ) );
    if (  kapp->layoutDirection() == Qt::RightToLeft )
        res = res.arg(  "@import \"%1\";" ).arg(  KStandardDirs::locate(  "data", "kdeui/about/kde_infopage_rtl.css" ) );
    else
        res = res.arg(  "" );


    QString title, intro, caption;
    if (KCGlobal::isInfoCenter())
    {
       res = res.arg(i18n(kcc_infotext))
                .arg(i18n(title_infotext))
                .arg(i18n(intro_infotext));
    }
    else
    {
       res = res.arg(i18n(kcc_text))
                .arg(i18n(title_text))
                .arg(i18n(intro_text));
    }

    QString content;

    if (!_moduleList)
    {
        content += "<table class=\"kc_table\">\n";
#define KC_HTMLROW( a, b ) "<tr><td class=\"kc_leftcol\">" + i18n( a ) + "</td><td class=\"kc_rightcol\">" + b + "</tr>\n"
        content += KC_HTMLROW( version_text, KCGlobal::kdeVersion() );
        content += KC_HTMLROW( user_text, KCGlobal::userName() );
        content += KC_HTMLROW( host_text, KCGlobal::hostName() );
        content += KC_HTMLROW( system_text, KCGlobal::systemName() );
        content += KC_HTMLROW( release_text, KCGlobal::systemRelease() );
        content += KC_HTMLROW( machine_text, KCGlobal::systemMachine() );
#undef KC_HTMLROW
        content += "</table>\n";
        content += "<p class=\"kc_use_text\">" + i18n( use_text ) + "</p>\n";
    }
    else
    {
        content += "<div id=\"tableTitle\">" + _caption + "</div>";

        content += "<table class=\"kc_table\">\n";
        // traverse the list
        Q3ListViewItem* pEntry = _category;
        while (pEntry != NULL)
        {
            QString szName;
            QString szComment;
            ConfigModule *module = static_cast<ModuleTreeItem*>(pEntry)->module();
            /* TODO: work out link */
            content += "<tr><td class=\"kc_leftcol\">";
            if (module)
            {
                szName = module->moduleName();
                szComment = module->comment();
                content += "<a href=\"%1\" class=\"kcm_link\">" + szName + "</a></td><td class=\"kc_rightcol\">" + szComment;
                KUrl moduleURL(QString("kcm://%1").arg(QString().sprintf("%p",(void*)module)) );
                QString linkURL( moduleURL.url() );
                content = content.arg( linkURL );
                _moduleMap.insert( linkURL, module );
            }
            else
            {
                szName = static_cast<ModuleTreeItem*>(pEntry)->caption();
                content += szName + "</td><td class=\"kc_rightcol\">" + szName;
            }
            content += "</td></tr>\n";
            pEntry = pEntry->nextSibling();
        }
        content += "</table>";
    }
    _viewer->begin(KUrl(file ));
    _viewer->write( res.arg( content ) );
    _viewer->end();
}

void AboutWidget::slotModuleLinkClicked( const KUrl& url )
{
	ConfigModule* module;
	module = _moduleMap[url.url()];
	if ( module )
		emit moduleSelected( module );
}

