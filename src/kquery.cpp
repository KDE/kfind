/*
    kquery.cpp

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#include "kquery.h"
#include "kfind_debug.h"
#include <stdlib.h>

#include <QCoreApplication>
#include <QMimeDatabase>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextCodec>
#include <QTextStream>
 
#include <KFileItem>
#include <KLocalizedString>
#include <KMessageBox>
#include <KZip>

#include <KFileMetaData/Extractor>
#include <KFileMetaData/ExtractorCollection>
#include <KFileMetaData/PropertyInfo>
#include <KFileMetaData/SimpleExtractionResult>

KQuery::KQuery(QObject *parent)
    : QObject(parent)
    , m_filetype(0)
    , m_sizemode(0)
    , m_sizeboundary1(0)
    , m_sizeboundary2(0)
    , m_timeFrom(0)
    , m_timeTo(0)
    , m_recursive(false)
    , m_casesensitive(false)
    , m_search_binary(false)
    , m_useLocate(false)
    , m_showHiddenFiles(false)
    , job(nullptr)
    , m_insideCheckEntries(false)
    , m_result(0)
{
    processLocate = new KProcess(this);
    connect(processLocate, &KProcess::readyReadStandardOutput, this, &KQuery::slotreadyReadStandardOutput);
    connect(processLocate, &KProcess::readyReadStandardError, this, &KQuery::slotreadyReadStandardError);
    connect(processLocate, QOverload<int, QProcess::ExitStatus>::of(&KProcess::finished), this, &KQuery::slotendProcessLocate);

    // Files with these mime types can be ignored, even if
    // findFormatByFileContent() in some cases may claim that
    // these are text files:
    ignore_mimetypes.append(QStringLiteral("application/pdf"));
    ignore_mimetypes.append(QStringLiteral("application/postscript"));

    // PLEASE update the documentation when you add another
    // file type here:
    ooo_mimetypes.append(QStringLiteral("application/vnd.sun.xml.writer"));
    ooo_mimetypes.append(QStringLiteral("application/vnd.sun.xml.calc"));
    ooo_mimetypes.append(QStringLiteral("application/vnd.sun.xml.impress"));
    // OASIS mimetypes, used by OOo-2.x and KOffice >= 1.4
    //ooo_mimetypes.append("application/vnd.oasis.opendocument.chart");
    //ooo_mimetypes.append("application/vnd.oasis.opendocument.graphics");
    //ooo_mimetypes.append("application/vnd.oasis.opendocument.graphics-template");
    //ooo_mimetypes.append("application/vnd.oasis.opendocument.formula");
    //ooo_mimetypes.append("application/vnd.oasis.opendocument.image");
    ooo_mimetypes.append(QStringLiteral("application/vnd.oasis.opendocument.presentation-template"));
    ooo_mimetypes.append(QStringLiteral("application/vnd.oasis.opendocument.presentation"));
    ooo_mimetypes.append(QStringLiteral("application/vnd.oasis.opendocument.spreadsheet-template"));
    ooo_mimetypes.append(QStringLiteral("application/vnd.oasis.opendocument.spreadsheet"));
    ooo_mimetypes.append(QStringLiteral("application/vnd.oasis.opendocument.text-template"));
    ooo_mimetypes.append(QStringLiteral("application/vnd.oasis.opendocument.text"));
    // KOffice-1.3 mimetypes
    koffice_mimetypes.append(QStringLiteral("application/x-kword"));
    koffice_mimetypes.append(QStringLiteral("application/x-kspread"));
    koffice_mimetypes.append(QStringLiteral("application/x-kpresenter"));
}

KQuery::~KQuery()
{
    qDeleteAll(m_regexps);
    m_fileItems.clear();
    if (processLocate->state() == QProcess::Running) {
        disconnect(processLocate);
        processLocate->kill();
        processLocate->waitForFinished(5000);
        delete processLocate;
    }
}

void KQuery::kill()
{
    if (job) {
        job->kill(KJob::EmitResult);
    }
    if (processLocate->state() == QProcess::Running) {
        processLocate->kill();
    }
    m_fileItems.clear();
}

void KQuery::start()
{
    m_fileItems.clear();
    if (m_useLocate) { //Use "locate" instead of the internal search method
        bufferLocate.clear();
        m_url = m_url.adjusted(QUrl::NormalizePathSegments);

        processLocate->clearProgram();
        processLocate->setProgram(QStandardPaths::findExecutable(QStringLiteral("locate")), QStringList{m_url.toLocalFile()});

        processLocate->setOutputChannelMode(KProcess::SeparateChannels);
        processLocate->start();
    } else { //Use KIO
        if (m_recursive) {
            job = KIO::listRecursive(m_url, KIO::HideProgressInfo);
        } else {
            job = KIO::listDir(m_url, KIO::HideProgressInfo);
        }

        connect(job, &KIO::ListJob::entries, this, QOverload<KIO::Job*, const KIO::UDSEntryList&>::of(&KQuery::slotListEntries));
        connect(job, &KIO::ListJob::result, this, &KQuery::slotResult);
    }
}

void KQuery::slotResult(KJob *_job)
{
    if (job != _job) {
        return;
    }
    job = nullptr;

    m_result = _job->error();
    if (m_result == KIO::ERR_USER_CANCELED) {
        m_fileItems.clear();
    }
    checkEntries();
}

void KQuery::slotListEntries(KIO::Job *, const KIO::UDSEntryList &list)
{
    for (const KIO::UDSEntry &entry : list) {
        m_fileItems.enqueue(KFileItem(entry, m_url, true, true));
    }

    checkEntries();
}

void KQuery::checkEntries()
{
    if (m_insideCheckEntries) {
        return;
    }

    m_insideCheckEntries = true;

    metaKeyRx = QRegExp(m_metainfokey);
    metaKeyRx.setPatternSyntax(QRegExp::Wildcard);

    m_foundFilesList.clear();

    int processingCount = 0;
    while (!m_fileItems.isEmpty())
    {
        processQuery(m_fileItems.dequeue());
        processingCount++;

        /* This is a workaround. As the qApp->processEvents() call inside processQuery
         * will bring more KIO entries, m_fileItems will increase even inside this loop
         * and that will lead to a big loop, it will take time to report found items to the GUI
         * so we are going to force Q_EMIT results every 100 files processed */
        if (processingCount == 100) {
            processingCount = 0;
            if (m_foundFilesList.size() > 0) {
                Q_EMIT foundFileList(m_foundFilesList);
                m_foundFilesList.clear();
            }
        }
    }

    if (m_foundFilesList.size() > 0) {
        Q_EMIT foundFileList(m_foundFilesList);
    }

    if (job == nullptr) {
        Q_EMIT result(m_result);
    }

    m_insideCheckEntries = false;
}

/* List of files found using slocate */
void KQuery::slotListEntries(const QStringList &list)
{
    metaKeyRx = QRegExp(m_metainfokey);
    metaKeyRx.setPatternSyntax(QRegExp::Wildcard);

    m_foundFilesList.clear();
    for (const auto &file : list) {
        processQuery(KFileItem(QUrl::fromLocalFile(file)));
    }

    if (!m_foundFilesList.isEmpty()) {
        Q_EMIT foundFileList(m_foundFilesList);
    }
}

/* Check if file meets the find's requirements*/
void KQuery::processQuery(const KFileItem &file)
{
    if (file.name() == QLatin1String(".") || file.name() == QLatin1String("..")) {
        return;
    }

    if (!m_showHiddenFiles && file.isHidden()) {
        return;
    }

    bool matched = false;

    for (const QRegExp *reg : std::as_const(m_regexps)) {
        matched = matched || (reg == nullptr) || (reg->exactMatch(file.url().adjusted(QUrl::StripTrailingSlash).fileName()));
    }
    if (!matched) {
        return;
    }

    // make sure the files are in the correct range
    switch (m_sizemode) {
    case 1: // "at least"
        if (file.size() < m_sizeboundary1) {
            return;
        }
        break;
    case 2: // "at most"
        if (file.size() > m_sizeboundary1) {
            return;
        }
        break;
    case 3: // "equal"
        if (file.size() != m_sizeboundary1) {
            return;
        }
        break;
    case 4: // "between"
        if ((file.size() < m_sizeboundary1)
            || (file.size() > m_sizeboundary2)) {
            return;
        }
        break;
    case 0: // "none" -> Fall to default
    default:
        break;
    }

    // make sure it's in the correct date range
    // what about 0 times?
    if (m_timeFrom && ((uint)m_timeFrom) > file.time(KFileItem::ModificationTime).toSecsSinceEpoch()) {
        return;
    }
    if (m_timeTo && ((uint)m_timeTo) < file.time(KFileItem::ModificationTime).toSecsSinceEpoch()) {
        return;
    }

    // username / group match
    if ((!m_username.isEmpty()) && (m_username != file.user())) {
        return;
    }
    if ((!m_groupname.isEmpty()) && (m_groupname != file.group())) {
        return;
    }

    // file type
    switch (m_filetype) {
    case 0:
        break;
    case 1: // plain file
        if (!S_ISREG(file.mode())) {
            return;
        }
        break;
    case 2:
        if (!file.isDir()) {
            return;
        }
        break;
    case 3:
        if (!file.isLink()) {
            return;
        }
        break;
    case 4:
        if (!S_ISCHR(file.mode()) && !S_ISBLK(file.mode())
            && !S_ISFIFO(file.mode()) && !S_ISSOCK(file.mode())) {
            return;
        }
        break;
    case 5: // binary
        if ((file.permissions() & 0111) != 0111 || file.isDir()) {
            return;
        }
        break;
    case 6: // suid
        if ((file.permissions() & 04000) != 04000) { // fixme
            return;
        }
        break;
    default:
        if (!m_mimetype.isEmpty() && !m_mimetype.contains(file.mimetype())) {
            return;
        }
    }

    // match data in metainfo...
    if ((!m_metainfo.isEmpty()) && (!m_metainfokey.isEmpty())) {
        //Avoid sequential files (fifo,char devices)
        if (!file.isRegularFile()) {
            return;
        }

        bool foundmeta = false;
        QString filename = file.url().path();

        if (filename.startsWith(QLatin1String("/dev/"))) {
            return;
        }

        QMimeDatabase mimeDb;
        QString mimetype = mimeDb.mimeTypeForFile(filename).name();
        QString strmetakeycontent;

        KFileMetaData::ExtractorCollection extractors;
        const QList<KFileMetaData::Extractor*> exList = extractors.fetchExtractors(mimetype);

        for (KFileMetaData::Extractor* ex : exList) {
            KFileMetaData::SimpleExtractionResult result(filename, mimetype,
                                                         KFileMetaData::ExtractionResult::ExtractMetaData);
            ex->extract(&result);

            const KFileMetaData::PropertyMultiMap properties = result.properties();
            for (auto it = properties.cbegin(); it != properties.cend(); ++it) {
                if (!metaKeyRx.exactMatch(KFileMetaData::PropertyInfo(it.key()).displayName())) {
                    continue;
                }
                strmetakeycontent = it.value().toString();
                if (strmetakeycontent.indexOf(m_metainfo) != -1) {
                    foundmeta = true;
                    break;
                }
            }
        }
        if (!foundmeta) {
            return;
        }
    }

    // match contents...
    QString matchingLine;
    if (!m_context.isEmpty()) {
        //Avoid sequential files (fifo,char devices)
        if (!file.isRegularFile()) {
            return;
        }

        if (!m_search_binary && ignore_mimetypes.indexOf(file.mimetype()) != -1) {
            return;
        }

        bool found = false;
        bool isZippedOfficeDocument = false;
        int matchingLineNumber = 0;

        // FIXME: doesn't work with non local files

        const QRegularExpression xmlTags(QStringLiteral("<.*?>"));
        QString filename;
        QTextStream *stream = nullptr;
        QFile qf;
        QByteArray zippedXmlFileContent;

        // KWord's and OpenOffice.org's files are zipped...
        if (ooo_mimetypes.indexOf(file.mimetype()) != -1
            || koffice_mimetypes.indexOf(file.mimetype()) != -1) {
            KZip zipfile(file.url().path());
            KZipFileEntry *zipfileEntry;

            if (zipfile.open(QIODevice::ReadOnly)) {
                const KArchiveDirectory *zipfileContent = zipfile.directory();

                if (koffice_mimetypes.indexOf(file.mimetype()) != -1) {
                    zipfileEntry = (KZipFileEntry *)zipfileContent->entry(QStringLiteral("maindoc.xml"));
                } else {
                    zipfileEntry = (KZipFileEntry *)zipfileContent->entry(QStringLiteral("content.xml")); //for OpenOffice.org
                }
                if (!zipfileEntry) {
                    qCWarning(KFING_LOG) << "Expected XML file not found in ZIP archive " << file.url();
                    return;
                }

                zippedXmlFileContent = zipfileEntry->data();
                stream = new QTextStream(zippedXmlFileContent, QIODevice::ReadOnly);

                // QTextStream default encoding is UTF-8 in Qt6
                #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
                stream->setCodec("UTF-8");
                #endif

                isZippedOfficeDocument = true;
            } else {
                qCWarning(KFING_LOG) << "Cannot open supposed ZIP file " << file.url();
            }
        } else if (!m_search_binary && !file.mimetype().startsWith(QLatin1String("text/"))
                   && file.url().isLocalFile() && !file.url().path().startsWith(QLatin1String("/dev"))) {
            QFile binfile(file.url().toLocalFile());
            if (!binfile.open(QIODevice::ReadOnly)) {
                return; // err, whatever
            }
            // Check the first 128 bytes (see shared-mime spec)
            const QByteArray bindata = binfile.read(128);
            const char* pbin = bindata.data();
            const int end = qMin(128, bindata.size());
            for (int i = 0; i < end; ++i) {
                if ((unsigned char)(pbin[i]) < 32 && pbin[i] != 9 && pbin[i] != 10 && pbin[i] != 13) // ASCII control character
                    return;
            }
        }

        if (!isZippedOfficeDocument) { //any other file or non-compressed KWord
            filename = file.url().path();
            if (filename.startsWith(QLatin1String("/dev/"))) {
                return;
            }
            qf.setFileName(filename);
            qf.open(QIODevice::ReadOnly);
            stream = new QTextStream(&qf);
            #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            stream->setEncoding(QStringConverter::System);
            #else
            stream->setCodec(QTextCodec::codecForLocale());
            #endif
        }

        while (!stream->atEnd())
        {
            QString str = stream->readLine();
            matchingLineNumber++;

            //If the stream ended (readLine().isNull() is true) the file was read completely
            //Do *not* use isEmpty() because that will exit if there is an empty line in the file
            if (str.isNull()) {
                break;
            }
            if (isZippedOfficeDocument) {
                str.remove(xmlTags);
            }

            if (str.indexOf(m_context, 0, m_casesensitive ? Qt::CaseSensitive : Qt::CaseInsensitive) != -1) {
                matchingLine = QString::number(matchingLineNumber) + QStringLiteral(": ") + str.trimmed();
                found = true;
                break;
            }
            qApp->processEvents();
        }

        delete stream;

        if (!found) {
            return;
        }
    }

    m_foundFilesList.append(QPair<KFileItem, QString>(file, matchingLine));
}

void KQuery::setContext(const QString &context, bool casesensitive, bool search_binary)
{
    m_context = context;
    m_casesensitive = casesensitive;
    m_search_binary = search_binary;
    m_regexp.setPatternSyntax(QRegExp::Wildcard);
    if (casesensitive) {
        m_regexp.setCaseSensitivity(Qt::CaseSensitive);
    } else {
        m_regexp.setCaseSensitivity(Qt::CaseInsensitive);
    }
}

void KQuery::setMetaInfo(const QString &metainfo, const QString &metainfokey)
{
    m_metainfo = metainfo;
    m_metainfokey = metainfokey;
}

void KQuery::setMimeType(const QStringList &mimetype)
{
    m_mimetype = mimetype;
}

void KQuery::setFileType(int filetype)
{
    m_filetype = filetype;
}

void KQuery::setSizeRange(int mode, KIO::filesize_t value1, KIO::filesize_t value2)
{
    m_sizemode = mode;
    m_sizeboundary1 = value1;
    m_sizeboundary2 = value2;
}

void KQuery::setTimeRange(time_t from, time_t to)
{
    m_timeFrom = from;
    m_timeTo = to;
}

void KQuery::setUsername(const QString &username)
{
    m_username = username;
}

void KQuery::setGroupname(const QString &groupname)
{
    m_groupname = groupname;
}

void KQuery::setRegExp(const QString &regexp, bool caseSensitive)
{
    const QStringList strList = regexp.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    //  QRegExp globChars ("[\\*\\?\\[\\]]", TRUE, FALSE);
    qDeleteAll(m_regexps);
    m_regexps.clear();
    m_regexps.reserve(strList.size());

    //  m_regexpsContainsGlobs.clear();
    for (const auto &str : strList) {
        //m_regexpsContainsGlobs.append(regExp->pattern().contains(globChars));
        m_regexps.append(new QRegExp(str, (caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive), QRegExp::Wildcard));
    }
}

void KQuery::setRecursive(bool recursive)
{
    m_recursive = recursive;
}

void KQuery::setPath(const QUrl &url)
{
    m_url = url;
}

void KQuery::setUseFileIndex(bool useLocate)
{
    m_useLocate = useLocate;
}

void KQuery::setShowHiddenFiles(bool showHidden)
{
    m_showHiddenFiles = showHidden;
}

void KQuery::slotreadyReadStandardError()
{
    KMessageBox::error(nullptr, QString::fromLocal8Bit(processLocate->readAllStandardOutput()), i18nc("@title:window", "Error while using locate"));
}

void KQuery::slotreadyReadStandardOutput()
{
    bufferLocate += processLocate->readAllStandardOutput();
}

void KQuery::slotendProcessLocate(int code, QProcess::ExitStatus)
{
    if (code == 0) {
        if (!bufferLocate.isEmpty()) {
            QString str = QString::fromLocal8Bit(bufferLocate);
            bufferLocate.clear();
            slotListEntries(str.split(QLatin1Char('\n'), Qt::SkipEmptyParts));
        }
    }
    Q_EMIT result(0);
}
