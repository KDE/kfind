/*
    kquery.h

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#ifndef KQUERY_H
#define KQUERY_H

#include <time.h>

#include <QList>
#include <QObject>
#include <QPair>
#include <QUrl>
#include <QQueue>
#include <QRegExp>
#include <QStringList>

#include <KIO/ListJob>
#include <KProcess>

class KFileItem;

class KQuery : public QObject
{
    Q_OBJECT

public:
    explicit KQuery(QObject *parent = nullptr);
    ~KQuery() override;

    /* Functions to set Query requirements */
    void setSizeRange(int mode, KIO::filesize_t value1, KIO::filesize_t value2);
    void setTimeRange(time_t from, time_t to);
    void setRegExp(const QString &regexp, bool caseSensitive);
    void setRecursive(bool recursive);
    void setPath(const QUrl &url);
    void setFileType(int filetype);
    void setMimeType(const QStringList &mimetype);
    void setContext(const QString &context, bool casesensitive, bool search_binary);
    void setUsername(const QString &username);
    void setGroupname(const QString &groupname);
    void setMetaInfo(const QString &metainfo, const QString &metainfokey);
    void setUseFileIndex(bool);
    void setShowHiddenFiles(bool);

    void start();
    void kill();
    const QUrl &url()
    {
        return m_url;
    }

private:
    /* Check if file meets the find's requirements*/
    inline void processQuery(const KFileItem &);

public Q_SLOTS:
    /* List of files found using slocate */
    void slotListEntries(const QStringList&);

protected Q_SLOTS:
    /* List of files found using KIO */
    void slotListEntries(KIO::Job *, const KIO::UDSEntryList &);
    void slotResult(KJob *);

    void slotreadyReadStandardOutput();
    void slotreadyReadStandardError();
    void slotendProcessLocate(int, QProcess::ExitStatus);

Q_SIGNALS:
    void foundFileList(const QList< QPair<KFileItem, QString> > &);
    void result(int);

private:
    void checkEntries();

    int m_filetype;
    int m_sizemode;
    KIO::filesize_t m_sizeboundary1;
    KIO::filesize_t m_sizeboundary2;
    QUrl m_url;
    time_t m_timeFrom;
    time_t m_timeTo;
    QRegExp m_regexp;// regexp for file content
    bool m_recursive;
    QStringList m_mimetype;
    QString m_context;
    QString m_username;
    QString m_groupname;
    QString m_metainfo;
    QString m_metainfokey;
    bool m_casesensitive;
    bool m_search_binary;
    bool m_useLocate;
    bool m_showHiddenFiles;
    QByteArray bufferLocate;
    QStringList locateList;
    KProcess *processLocate = nullptr;
    QList<QRegExp *> m_regexps;// regexps for file name
//  QValueList<bool> m_regexpsContainsGlobs;  // what should this be good for ? Alex
    KIO::ListJob *job;
    bool m_insideCheckEntries;
    QQueue<KFileItem> m_fileItems;
    QRegExp metaKeyRx;
    int m_result;
    QStringList ignore_mimetypes;
    QStringList ooo_mimetypes;   // OpenOffice.org mimetypes
    QStringList koffice_mimetypes;

    QList< QPair<KFileItem, QString> > m_foundFilesList;
};

#endif
