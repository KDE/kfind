#include <stdlib.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <kfileitem.h>
#include <kfilemetainfo.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include "kquery.h"

KQuery::KQuery(QObject *parent, const char * name)
  : QObject(parent, name),
    m_minsize(-1), m_maxsize(-1),
    m_timeFrom(0), m_timeTo(0),
    job(0)
{
  m_regexps.setAutoDelete(true);
  processLocate = new KProcess;
  connect(processLocate,SIGNAL(receivedStdout(KProcess*, char*, int)),this,SLOT(slotreceivedSdtout(KProcess*,char*,int)));
  connect(processLocate,SIGNAL(receivedStderr(KProcess*, char*, int)),this,SLOT(slotreceivedSdterr(KProcess*,char*,int)));
  connect(processLocate,SIGNAL(processExited(KProcess*)),this,SLOT(slotendProcessLocate(KProcess*)));
}

KQuery::~KQuery()
{
}

void KQuery::kill()
{
  if (job)
    job->kill(false);
}

void KQuery::start()
{
  QString directory;
  KProcess proc;
  
  directory.sprintf("kfindtmp.%d/", getpid());
  tmpdir = locateLocal("tmp", directory );

  proc.clearArguments();
  proc  << "mkdir" << tmpdir;
  proc.start(KProcess::Block);

  if(m_useLocate) //use "locate" instead of the internal search method
  {
    processLocate->clearArguments();
    *processLocate << "locate";
    *processLocate << m_url.path().latin1();
    bufferLocate=NULL;
    bufferLocateLength=0;
    processLocate->start(KProcess::NotifyOnExit,KProcess::AllOutput);
    return;
  }

  if (m_recursive)
    job = KIO::listRecursive( m_url, false );
  else
    job = KIO::listDir( m_url, false );

  connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
	  SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)));
  connect(job, SIGNAL(result(KIO::Job *)), SLOT(slotResult(KIO::Job *)));
  connect(job, SIGNAL(canceled(KIO::Job *)), SLOT(slotCanceled(KIO::Job *)));
}

void KQuery::slotResult( KIO::Job * _job )
{
  if (job != _job) return;
  job = 0;
  removeTempDir();
  emit result(_job->error());
}

void KQuery::slotCanceled( KIO::Job * _job )
{
  if (job != _job) return;
  job = 0;
  removeTempDir();
  emit result(KIO::ERR_USER_CANCELED);
}

/* List of files found using KIO */
void KQuery::slotListEntries( KIO::Job *, const KIO::UDSEntryList & list)
{
  KFileItem * file = 0;
  metaKeyRx=new QRegExp(m_metainfokey,true,true);
  KIO::UDSEntryListConstIterator  it = list.begin();
  KIO::UDSEntryListConstIterator end = list.end();

  for (; it != end; ++it)
  {
    file = new KFileItem(*it, m_url, true, true);
    processQuery(file);
    delete file;
  }

  delete metaKeyRx;
}

/* List of files found using slocate */
void KQuery::slotListEntries( QStringList  list )
{
  KFileItem * file = 0;
  metaKeyRx=new QRegExp(m_metainfokey,true,true);

  QStringList::Iterator it = list.begin();
  QStringList::Iterator end = list.end();

  for (; it != end; ++it)
  {
    file = new KFileItem( KFileItem::Unknown,  KFileItem::Unknown, KURL(*it));
    processQuery(file);
    delete file;
  }

  delete metaKeyRx;
}

/* Check if file meets the find's requirements*/
void KQuery::processQuery( KFileItem* file)
{
  QRegExp *filename_match;

    if ( file->name() == "." || file->name() == ".." )
      return;

    bool matched=false;

    for ( filename_match = m_regexps.first(); !matched && filename_match; filename_match = m_regexps.next() )
    {
      matched |=  filename_match->isEmpty()  ||
                  (filename_match->exactMatch( file->url().fileName( true ) ) );
    }
    if (!matched)
      return;

    // make sure the files are in the correct range
    if (  ( m_minsize >= 0 && (int)file->size() < m_minsize ) ||
          ( m_maxsize >= 0 && (int)file->size() > m_maxsize ) )
      return;

    // make sure it's in the correct date range
    // what about 0 times?
    if ( m_timeFrom && m_timeFrom > file->time(KIO::UDS_MODIFICATION_TIME) )
      return;
    if ( m_timeTo && m_timeTo < file->time(KIO::UDS_MODIFICATION_TIME) )
      return;

    // username / group match
    if ( (m_username != "") && (m_username != file->user()) )
       return;
    if ( (m_groupname != "") && (m_groupname != file->group()) )
       return;

    // file type
    switch (m_filetype)
    {
      case 0:
        break;
      case 1: // plain file
        if ( !S_ISREG( file->mode() ) )
          return;
        break;
      case 2:
        if ( !file->isDir() )
          return;
        break;
      case 3:
        if ( !file->isLink() )
          return;
        break;
      case 4:
        if ( !S_ISCHR ( file->mode() ) && !S_ISBLK ( file->mode() ) &&
              !S_ISFIFO( file->mode() ) && !S_ISSOCK( file->mode() ) )
              return;
        break;
      case 5: // binary
        if ( (file->permissions() & 0111) != 0111 || file->isDir() )
          return;
        break;
      case 6: // suid
        if ( (file->permissions() & 04000) != 04000 ) // fixme
          return;
        break;
      default:
        if (!m_mimetype.isEmpty() && m_mimetype != file->mimetype())
          return;
    }

    // match datas in metainfo...
    if ((!m_metainfo.isEmpty())  && (!m_metainfokey.isEmpty()))
    {
       bool foundmeta=false;
       QString filename = file->url().path();

       if(filename.startsWith("/dev/"))
          return;

       KFileMetaInfo metadatas(filename);
       KFileMetaInfoItem metaitem;
       QStringList metakeys;
       QString strmetakeycontent;

       if(metadatas.isEmpty())
          return;

       metakeys=metadatas.supportedKeys();
       for ( QStringList::Iterator it = metakeys.begin(); it != metakeys.end(); ++it )
       {
          if (!metaKeyRx->exactMatch(*it))
             continue;
          metaitem=metadatas.item(*it);
          strmetakeycontent=metaitem.string();
          if(strmetakeycontent.find(m_metainfo)!=-1)
          {
             foundmeta=true;
             break;
          }
       }
       if (!foundmeta)
          return;
    }

    // match contents...
    QString matchingLine;
    if (!m_context.isEmpty())
    {
       bool found = false;
       int matchingLineNumber=0;

       // FIXME: doesn't work with non local files
       
       QString filename;
       KProcess proc;

       //KWord's files are compressed...
       if((file->mimetype()=="application/x-kword")||(file->mimetype()=="application/vnd.sun.xml.writer"))
       {
         proc.clearArguments();
         proc << "unzip" << "-o" << file->url().path() << "-d" << tmpdir;
         proc.start(KProcess::Block);

         if(file->mimetype()=="application/x-kword")
           filename=tmpdir+"/maindoc.xml";
         else
           filename=tmpdir+"/content.xml";
           
         if(!QFile::exists(filename))
           filename=file->url().path();
       }
       else
         filename = file->url().path();
         
       if(filename.startsWith("/dev/"))
          return;
       QFile qf(filename);
       qf.open(IO_ReadOnly);
       QTextStream stream(&qf);
       stream.setEncoding(QTextStream::Locale);
       while ( ! stream.atEnd() )
       {
          QString str = stream.readLine();
          matchingLineNumber++;

          if (str.isNull()) break;
          if (m_regexpForContent)
          {
             if (m_regexp.search(str)>=0)
             {
                matchingLine=QString::number(matchingLineNumber)+": "+str;
                found = true;
                break;
             }
          }
          else
          {
             if (str.find(m_context, 0, m_casesensitive) != -1)
             {
                matchingLine=QString::number(matchingLineNumber)+": "+str;
                found = true;
                break;
             }
          };
       }

       if (!found)
          return;
    }
    emit addFile(file,matchingLine);
}

void KQuery::setContext(const QString & context, bool casesensitive, bool useRegexp)
{
  m_context = context;
  m_casesensitive = casesensitive;
  m_regexpForContent=useRegexp;
  m_regexp.setWildcard(!m_regexpForContent);
  m_regexp.setCaseSensitive(casesensitive);
  if (m_regexpForContent)
     m_regexp.setPattern(m_context);
}

void KQuery::setMetaInfo(const QString &metainfo, const QString &metainfokey)
{
  m_metainfo=metainfo;
  m_metainfokey=metainfokey;
}

void KQuery::setMimeType(const QString &mimetype)
{
  m_mimetype = mimetype;
}

void KQuery::setFileType(int filetype)
{
  m_filetype = filetype;
}

void KQuery::setSizeRange(int min, int max)
{
  m_minsize = min;
  m_maxsize = max;
}

void KQuery::setTimeRange(time_t from, time_t to)
{
  m_timeFrom = from;
  m_timeTo = to;
}

void KQuery::setUsername(QString username)
{
   m_username = username;
}

void KQuery::setGroupname(QString groupname)
{
   m_groupname = groupname;
}


void KQuery::setRegExp(const QString &regexp, bool caseSensitive)
{
  QRegExp *regExp;
  QRegExp sep(";");
  QStringList strList=QStringList::split( sep, regexp, false);
  QRegExp globChars ("[\\*\\?\\[\\]]", TRUE,  FALSE);

  m_regexps.clear();
  m_regexpsContainsGlobs.clear();
  for ( QStringList::Iterator it = strList.begin(); it != strList.end(); ++it ) {
    regExp = new QRegExp((*it),caseSensitive,true);
    m_regexpsContainsGlobs.append(regExp->pattern().contains(globChars));
    m_regexps.append(regExp);
  }
}

void KQuery::setRecursive(bool recursive)
{
  m_recursive = recursive;
}

void KQuery::setPath(const KURL &url)
{
  m_url = url;
}

void KQuery::setUseFileIndex(bool useLocate)
{
	m_useLocate=useLocate;
}

void KQuery::slotreceivedSdterr(KProcess* ,char* str,int)
{
  KMessageBox::error(NULL, QString(str), i18n("Error while using locate"));
}

void KQuery::slotreceivedSdtout(KProcess*,char* str,int l)
{
  int i;

  bufferLocateLength+=l; 
  str[l]='\0';
  bufferLocate=(char*)realloc(bufferLocate,sizeof(char)*(bufferLocateLength));
  for (i=0;i<l;i++)
    bufferLocate[bufferLocateLength-l+i]=str[i];
}

void KQuery::slotendProcessLocate(KProcess*)
{
  QString qstr;
  QStringList strlist;
  int i,j,k;

  if((bufferLocateLength==0)||(bufferLocate==NULL))
  {
    emit result(0);
    return;
  }
    
  i=0;
  do
  {
  	j=1;
  	while(bufferLocate[i]!='\n')
   {
   	i++;
    j++;
   }
   qstr="";
   for(k=0;k<j-1;k++)
   	qstr.append(bufferLocate[k+i-j+1]);
   strlist.append(qstr);
   i++;
  	
  }while(i<bufferLocateLength);
  bufferLocateLength=0;
  free(bufferLocate);
  bufferLocate=NULL;
  slotListEntries(strlist );
  emit result(0);
}

void KQuery::removeTempDir()
{
  KProcess proc;

  proc.clearArguments();
  proc  << "rm" << "-Rf" << tmpdir;
  proc.start(KProcess::Block);
}
  
#include "kquery.moc"
