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

int main( int argc, char ** argv )
{
    QApplication app(argc, argv);
    Kdelibs4ConfigMigrator migrate(QStringLiteral("kfind"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("kfindrc"));
    migrate.migrate();
    KLocalizedString::setApplicationDomain("kfind");

    KAboutData aboutData( QLatin1String("kfind"), i18n("KFind"),
                          QLatin1String(KFIND_VERSION_STRING), i18n("KDE file find utility"), KAboutLicense::GPL,
                          i18n("(c) 1998-2017, The KDE Developers"));

    aboutData.addAuthor(i18n("Kai-Uwe Broulik"), i18n("Current Maintainer"), QLatin1String("kde@privat.broulik.de"));
    aboutData.addAuthor(i18n("Eric Coquelle"), i18n("Former Maintainer"), QLatin1String("coquelle@caramail.com"));
    aboutData.addAuthor(i18n("Mark W. Webb"), i18n("Developer"), QLatin1String("markwebb@adelphia.net"));
    aboutData.addAuthor(i18n("Beppe Grimaldi"), i18n("UI Design & more search options"), QLatin1String("grimalkin@ciaoweb.it"));
    aboutData.addAuthor(i18n("Martin Hartig"));
    aboutData.addAuthor(i18n("Stephan Kulow"), QString(), QLatin1String("coolo@kde.org"));
    aboutData.addAuthor(i18n("Mario Weilguni"),QString(), QLatin1String("mweilguni@sime.com"));
    aboutData.addAuthor(i18n("Alex Zepeda"),QString(), QLatin1String("zipzippy@sonic.net"));
    aboutData.addAuthor(i18n("Miroslav Flídr"),QString(), QLatin1String("flidr@kky.zcu.cz"));
    aboutData.addAuthor(i18n("Harri Porten"),QString(), QLatin1String("porten@kde.org"));
    aboutData.addAuthor(i18n("Dima Rogozin"),QString(), QLatin1String("dima@mercury.co.il"));
    aboutData.addAuthor(i18n("Carsten Pfeiffer"),QString(), QLatin1String("pfeiffer@kde.org"));
    aboutData.addAuthor(i18n("Hans Petter Bieker"), QString(), QLatin1String("bieker@kde.org"));
    aboutData.addAuthor(i18n("Waldo Bastian"), i18n("UI Design"), QLatin1String("bastian@kde.org"));
    aboutData.addAuthor(i18n("Alexander Neundorf"), QString(), QLatin1String("neundorf@kde.org"));
    aboutData.addAuthor(i18n("Clarence Dang"), QString(), QLatin1String("dang@kde.org"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    // enable high dpi support
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addVersionOption();
    parser.addHelpOption();
    parser.addOption(QCommandLineOption(QStringList() <<  QLatin1String("+[searchpath]"), i18n("Path(s) to search")));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);


    QUrl url;
    if (parser.positionalArguments().count() > 0)
#if QT_VERSION >= QT_VERSION_CHECK(5,4,0)
        url = QUrl::fromUserInput(parser.positionalArguments().at(0), QDir::currentPath(), QUrl::AssumeLocalFile);
#else
        url = QUrl::fromUserInput(parser.positionalArguments().at(0));
#endif
    if (url.isEmpty())
        url = QUrl::fromLocalFile(QDir::currentPath());
    if (url.isEmpty())
        url = QUrl::fromLocalFile(QDir::homePath());

    KfindDlg kfinddlg(url);
    return kfinddlg.exec();
}

