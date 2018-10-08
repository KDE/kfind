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

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QUrl>

#include <KAboutData>
#include <KLocalizedString>
#include <Kdelibs4ConfigMigrator>

#include "kfinddlg.h"
#include "kfind_version.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    Kdelibs4ConfigMigrator migrate(QStringLiteral("kfind"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("kfindrc"));
    migrate.migrate();
    KLocalizedString::setApplicationDomain("kfind");

    KAboutData aboutData(QStringLiteral("kfind"), i18n("KFind"),
                         QStringLiteral(KFIND_VERSION_STRING), i18n("KDE file find utility"), KAboutLicense::GPL,
                         i18n("(c) 1998-2017, The KDE Developers"));

    aboutData.addAuthor(i18n("Kai-Uwe Broulik"), i18n("Current Maintainer"), QStringLiteral("kde@privat.broulik.de"));
    aboutData.addAuthor(i18n("Eric Coquelle"), i18n("Former Maintainer"), QStringLiteral("coquelle@caramail.com"));
    aboutData.addAuthor(i18n("Mark W. Webb"), i18n("Developer"), QStringLiteral("markwebb@adelphia.net"));
    aboutData.addAuthor(i18n("Beppe Grimaldi"), i18n("UI Design & more search options"), QStringLiteral("grimalkin@ciaoweb.it"));
    aboutData.addAuthor(i18n("Martin Hartig"));
    aboutData.addAuthor(i18n("Stephan Kulow"), QString(), QStringLiteral("coolo@kde.org"));
    aboutData.addAuthor(i18n("Mario Weilguni"), QString(), QStringLiteral("mweilguni@sime.com"));
    aboutData.addAuthor(i18n("Alex Zepeda"), QString(), QStringLiteral("zipzippy@sonic.net"));
    aboutData.addAuthor(i18n("Miroslav Flídr"), QString(), QStringLiteral("flidr@kky.zcu.cz"));
    aboutData.addAuthor(i18n("Harri Porten"), QString(), QStringLiteral("porten@kde.org"));
    aboutData.addAuthor(i18n("Dima Rogozin"), QString(), QStringLiteral("dima@mercury.co.il"));
    aboutData.addAuthor(i18n("Carsten Pfeiffer"), QString(), QStringLiteral("pfeiffer@kde.org"));
    aboutData.addAuthor(i18n("Hans Petter Bieker"), QString(), QStringLiteral("bieker@kde.org"));
    aboutData.addAuthor(i18n("Waldo Bastian"), i18n("UI Design"), QStringLiteral("bastian@kde.org"));
    aboutData.addAuthor(i18n("Alexander Neundorf"), QString(), QStringLiteral("neundorf@kde.org"));
    aboutData.addAuthor(i18n("Clarence Dang"), QString(), QStringLiteral("dang@kde.org"));
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"), i18nc("EMAIL OF TRANSLATORS", "Your emails"));
    // enable high dpi support
    app.setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    parser.addOption(QCommandLineOption(QStringList() <<  QStringLiteral("+[searchpath]"), i18n("Path(s) to search")));

    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    QUrl url;
    if (!parser.positionalArguments().isEmpty())
    {
        url = QUrl::fromUserInput(parser.positionalArguments().at(0), QDir::currentPath(), QUrl::AssumeLocalFile);
    }
    if (url.isEmpty()) {
        url = QUrl::fromLocalFile(QDir::currentPath());
    }
    if (url.isEmpty()) {
        url = QUrl::fromLocalFile(QDir::homePath());
    }

    KfindDlg kfinddlg(url);
    return kfinddlg.exec();
}
