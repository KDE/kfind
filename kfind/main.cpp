/*******************************************************************
* main.cpp
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
******************************************************************/

#include <QDir>
#include <QFile>


#include <kurl.h>
#include <KLocalizedString>
#include <QApplication>
#include <KAboutData>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <Kdelibs4ConfigMigrator>

#include "kfinddlg.h"
#include "kfind_version.h"

static const char description[] = I18N_NOOP("KDE file find utility");

int main( int argc, char ** argv )
{
    Kdelibs4ConfigMigrator migrate(QStringLiteral("kfind"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("kfindrc"));
    migrate.migrate();

    KLocalizedString::setApplicationDomain("kfind");

  KAboutData aboutData( QLatin1String("kfind"), i18n("KFind"),
      QLatin1String(KFIND_VERSION_STRING), i18n(description), KAboutLicense::GPL,
      i18n("(c) 1998-2003, The KDE Developers"));

  aboutData.addAuthor(i18n("Eric Coquelle"), i18n("Current Maintainer"), "coquelle@caramail.com");
  aboutData.addAuthor(i18n("Mark W. Webb"), i18n("Developer"), "markwebb@adelphia.net");
  aboutData.addAuthor(i18n("Beppe Grimaldi"), i18n("UI Design & more search options"), "grimalkin@ciaoweb.it");
  aboutData.addAuthor(i18n("Martin Hartig"));
  aboutData.addAuthor(i18n("Stephan Kulow"), QString(), "coolo@kde.org");
  aboutData.addAuthor(i18n("Mario Weilguni"),QString(), "mweilguni@sime.com");
  aboutData.addAuthor(i18n("Alex Zepeda"),QString(), "zipzippy@sonic.net");
  aboutData.addAuthor(i18n("Miroslav Flídr"),QString(), "flidr@kky.zcu.cz");
  aboutData.addAuthor(i18n("Harri Porten"),QString(), "porten@kde.org");
  aboutData.addAuthor(i18n("Dima Rogozin"),QString(), "dima@mercury.co.il");
  aboutData.addAuthor(i18n("Carsten Pfeiffer"),QString(), "pfeiffer@kde.org");
  aboutData.addAuthor(i18n("Hans Petter Bieker"), QString(), "bieker@kde.org");
  aboutData.addAuthor(i18n("Waldo Bastian"), i18n("UI Design"), "bastian@kde.org");
  aboutData.addAuthor(i18n("Alexander Neundorf"), QString(), "neundorf@kde.org");
  aboutData.addAuthor(i18n("Clarence Dang"), QString(), "dang@kde.org");

    QApplication app(argc, argv);
    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("+[searchpath]"), i18n("Path(s) to search")));

    //PORTING SCRIPT: adapt aboutdata variable if necessary
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);


  KUrl url;
  if (parser.positionalArguments().count() > 0)
    url = parser.positionalArguments().at(0);
  if (url.isEmpty())
    url = QDir::currentPath();
  if (url.isEmpty())
    url = QDir::homePath();
  

  KfindDlg kfinddlg(url);
  return kfinddlg.exec();
}

