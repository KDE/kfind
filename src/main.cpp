/*
    main.cpp

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QUrl>

#include <KAboutData>
#include <KLocalizedString>

#include "kfinddlg.h"
#include "kfind_version.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("kfind");

    KAboutData aboutData(QStringLiteral("kfind"), i18n("KFind"),
                         QStringLiteral(KFIND_VERSION_STRING), i18n("KDE file find utility"), KAboutLicense::GPL,
                         i18n("(c) 1998-2021, The KDE Developers"));

    aboutData.addAuthor(i18n("Kai Uwe Broulik"), i18n("Current Maintainer"), QStringLiteral("kde@privat.broulik.de"));
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
