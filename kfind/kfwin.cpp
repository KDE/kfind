/***********************************************************************
 *
 *  Kfwin.cpp
 *
 **********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <time.h>

#include <qtextstream.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qclipboard.h>
#include <qpixmap.h>
#include <qdragobject.h>

#include <kfiledialog.h>
#include <klocale.h>
#include <kapp.h>
#include <krun.h>
#include <kprocess.h>
#include <kpropsdlg.h>
#include <kstddirs.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kglobal.h>
#include <kopenwith.h>
#include <kpopupmenu.h>
#include <kio/netaccess.h>
#include <kurl.h>
#include <kurldrag.h>
#include <qptrlist.h>

#include "kfwin.h"

#include "kfwin.moc"

template class QPtrList<KfFileLVI>;

// Permission strings
static const char* perm[4] = {
  I18N_NOOP( "Read-write" ),
  I18N_NOOP( "Read-only" ),
  I18N_NOOP( "Write-only" ),
  I18N_NOOP( "Inaccessible" ) };
#define RW 0
#define RO 1
#define WO 2
#define NA 3

KfFileLVI::KfFileLVI(KfindWindow* lv, const KFileItem &item, const QString& matchingLine)
  : QListViewItem(lv),
    fileitem(item)
{
  fileInfo = new QFileInfo(item.url().path());

  QString size = KGlobal::locale()->formatNumber(item.size(), 0);

  QDateTime dt;
  dt.setTime_t(item.time(KIO::UDS_MODIFICATION_TIME));
  QString date = KGlobal::locale()->formatDateTime(dt);

  int perm_index;
  if(fileInfo->isReadable())
    perm_index = fileInfo->isWritable() ? RW : RO;
  else
    perm_index = fileInfo->isWritable() ? WO : NA;

  // Fill the item with data
  setText(0, item.url().fileName(false));
  setText(1, lv->reducedDir(item.url().directory(false)));
  setText(2, size);
  setText(3, date);
  setText(4, i18n(perm[perm_index]));
  setText(5, matchingLine);

  // put the icon into the leftmost column
  setPixmap(0, item.pixmap(16));
}

KfFileLVI::~KfFileLVI()
{
  delete fileInfo;
}

QString KfFileLVI::key(int column, bool) const
{
  switch (column) {
  case 2:
    // Returns date in bytes. Used for sorting
    return QString().sprintf("%10d", fileInfo->size());
  case 3:
    // Returns time in secs from 1/1/1970. Used for sorting
    return QString().sprintf("%10ld", fileitem.time(KIO::UDS_MODIFICATION_TIME));
  }

  return text(column);
}

KfindWindow::KfindWindow( QWidget *parent, const char *name )
  : KListView( parent, name )
,m_baseDir("")
,m_menu(0)
{
  setSelectionMode( QListView::Extended );
  setShowSortIndicator( TRUE );

  addColumn(i18n("Name"));
  addColumn(i18n("In subdirectory"));
  addColumn(i18n("Size"));
  setColumnAlignment(2, AlignRight);
  addColumn(i18n("Modified"));
  setColumnAlignment(3, AlignRight);
  addColumn(i18n("Permissions"));
  setColumnAlignment(4, AlignRight);

  addColumn(i18n("First matching line"));
  setColumnAlignment(5, AlignLeft);

  // Disable autoresize for all columns
  // Resizing is done by resetColumns() function
  for (int i = 0; i < 6; i++)
    setColumnWidthMode(i, Manual);

  resetColumns(TRUE);

  connect( this, SIGNAL(selectionChanged()),
	   this, SLOT( selectionHasChanged() ));

  connect(this, SIGNAL(contextMenu(KListView *, QListViewItem*,const QPoint&)),
	  this, SLOT(slotContextMenu(KListView *,QListViewItem*,const QPoint&)));

  connect(this, SIGNAL(executed(QListViewItem*)),
	  this, SLOT(slotExecute(QListViewItem*)));
  setDragEnabled(true);
}

QString KfindWindow::reducedDir(const QString& fullDir)
{
   if (fullDir.find(m_baseDir)==0)
   {
      QString tmp=fullDir.mid(m_baseDir.length());
      return tmp;
   };
   return fullDir;
};

void KfindWindow::beginSearch(const KURL& baseUrl)
{
  m_baseDir=baseUrl.path(+1);
  haveSelection = false;
  clear();
}

void KfindWindow::endSearch()
{
}

void KfindWindow::insertItem(const KFileItem &item, const QString& matchingLine)
{
  new KfFileLVI(this, item, matchingLine);
}

// copy to clipboard aka X11 selection
void KfindWindow::copySelection()
{
  QDragObject *drag_obj = dragObject();

  if (drag_obj)
  {
    QClipboard *cb = kapp->clipboard();
    cb->setData(drag_obj);
  }
}

void KfindWindow::saveResults()
{
  QListViewItem *item;

  KFileDialog *dlg = new KFileDialog(QString::null, QString::null, this,
	"filedialog", true);

  dlg->setCaption(i18n("Save Results As"));

  KMimeType::List list;

  list.append(KMimeType::mimeType("text/plain"));
  list.append(KMimeType::mimeType("text/html"));

  dlg->setFilterMimeType(i18n("Save as:"), list, KMimeType::mimeType("text/plain"));

  dlg->exec();

  KURL u = dlg->selectedURL();
  KMimeType::Ptr mimeType = dlg->currentFilterMimeType();
  delete dlg;

  if (u.isMalformed() || !u.isLocalFile())
     return;

  QString filename = u.path();

  QFile file(filename);

  if ( !file.open(IO_WriteOnly) )
    KMessageBox::error(parentWidget(),
		       i18n("It wasn't possible to save results!"));
  else {
    QTextStream stream( &file );
    stream.setEncoding( QTextStream::Locale );

    if ( mimeType->name() == "text/html") {
      stream << QString::fromLatin1("<HTML><HEAD>\n"
				    "<!DOCTYPE %1>\n"
				    "<TITLE>%2</TITLE></HEAD>\n"
				    "<BODY><H1>%3</H1>"
				    "<DL><p>\n")
	.arg(i18n("KFind Results File"))
	.arg(i18n("KFind Results File"))
	.arg(i18n("KFind Results File"));

      item = firstChild();
      while(item != NULL)
	{
	  QString path=((KfFileLVI*)item)->fileitem.url().url();
	  QString pretty=((KfFileLVI*)item)->fileitem.url().prettyURL();
	  stream << QString::fromLatin1("<DT><A HREF=\"%1\">%2</A>\n")
	    .arg(path).arg(pretty);

	  item = item->nextSibling();
	}
      stream << QString::fromLatin1("</DL><P></BODY></HTML>\n");
    }
    else {
      item = firstChild();
      while(item != NULL)
      {
	QString path=((KfFileLVI*)item)->fileitem.url().url();
	stream << path << endl;
	item = item->nextSibling();
      }
    }

    file.close();
    KMessageBox::information(parentWidget(),
			     i18n("Results were saved to file\n")+
			     filename);
  }
}

// This function is called when selection is changed (both selected/deselected)
// It notifies the parent about selection status and enables/disables menubar
void KfindWindow::selectionHasChanged()
{
  emit resultSelected(true);

  QListViewItem *item = firstChild();
  while(item != 0L)
  {
    if(isSelected(item)) {
      emit resultSelected( true );
      haveSelection = true;
      return;
    }

    item = item->nextSibling();
  }

  haveSelection = false;
  emit resultSelected(false);
}

void KfindWindow::deleteFiles()
{
  QString tmp = i18n("Do you really want to delete selected file(s)?");
  if (KMessageBox::questionYesNo(parentWidget(), tmp) == KMessageBox::No)
    return;

  // Iterate on all selected elements
  QPtrList<QListViewItem> selected = selectedItems();
  for ( uint i = 0; i < selected.count(); i++ ) {
    KfFileLVI *item = (KfFileLVI *) selected.at(i);
    KFileItem file = item->fileitem;

    KIO::NetAccess::del(file.url());
  }
  selected.setAutoDelete(true);
}

void KfindWindow::fileProperties()
{
  // This dialog must be modal because it parent dialog is modal as well.
  // Non-modal property dialog will hide behind the main window
  (void) new KPropertiesDialog( &((KfFileLVI *)currentItem())->fileitem, this,
				"propDialog", true);
}

void KfindWindow::openFolder()
{
  KFileItem fileitem = ((KfFileLVI *)currentItem())->fileitem;
  KURL url = fileitem.url();
  url.setFileName(QString::null);

  (void) new KRun(url);
}

void KfindWindow::openBinding()
{
  ((KfFileLVI*)currentItem())->fileitem.run();
}

void KfindWindow::slotExecute(QListViewItem* item)
{
   if (item==0)
      return;
  ((KfFileLVI*)item)->fileitem.run();
}

// Resizes KListView to occupy all visible space
void KfindWindow::resizeEvent(QResizeEvent *e)
{
  KListView::resizeEvent(e);
  resetColumns(FALSE);
  clipper()->repaint();
}

QDragObject * KfindWindow::dragObject() const
{
  KURL::List uris;
  QPtrList<QListViewItem> selected = selectedItems();

  // create a list of URIs from selection
  for ( uint i = 0; i < selected.count(); i++ )
  {
    KfFileLVI *item = (KfFileLVI *) selected.at( i );
    if (item)
    {
      uris.append( item->fileitem.url() );
    }
  }

  if ( uris.count() <= 0 )
     return 0;

  QUriDrag *ud = KURLDrag::newDrag( uris, (QWidget *) this, "kfind uridrag" );

  const QPixmap *pix = currentItem()->pixmap(0);
  if ( pix && !pix->isNull() )
    ud->setPixmap( *pix );

  return ud;
}

void KfindWindow::resetColumns(bool init)
{
   QFontMetrics fm = fontMetrics();
  if (init)
  {
    setColumnWidth(2, QMAX(fm.width(columnText(2)), fm.width("0000000")) + 15);
    QString sampleDate =
      KGlobal::locale()->formatDateTime(QDateTime::currentDateTime());
    setColumnWidth(3, QMAX(fm.width(columnText(3)), fm.width(sampleDate)) + 15);
    setColumnWidth(4, QMAX(fm.width(columnText(4)), fm.width(i18n(perm[RO]))) + 15);
    setColumnWidth(5, QMAX(fm.width(columnText(5)), fm.width("some text")) + 15);
  }

  int free_space = visibleWidth() -
    columnWidth(2) - columnWidth(3) - columnWidth(4) - columnWidth(5);

//  int name_w = QMIN((int)(free_space*0.5), 150);
//  int dir_w = free_space - name_w;
  int name_w = QMAX((int)(free_space*0.5), fm.width("some long filename"));
  int dir_w = name_w;

  setColumnWidth(0, name_w);
  setColumnWidth(1, dir_w);
}

void KfindWindow::slotContextMenu(KListView *,QListViewItem *item,const QPoint&p)
{
  if (!item) return;
  int count = selectedItems().count();

  if (count == 0)
  {
     return;
  };

  if (m_menu==0)
     m_menu = new KPopupMenu(this);
  else
     m_menu->clear();

  if (count == 1)
  {
     //menu = new KPopupMenu(item->text(0), this);
     m_menu->insertTitle(item->text(0));
     m_menu->insertItem(i18n("Copy"), this, SLOT(copySelection()));
     m_menu->insertItem(i18n("Delete"), this, SLOT(deleteFiles()));
     m_menu->insertItem(i18n("Open Directory"), this, SLOT(openFolder()));
     m_menu->insertItem(i18n("Open With..."), this, SLOT(slotOpenWith()));
     m_menu->insertItem(i18n("Open"), this, SLOT(openBinding()));
     m_menu->insertSeparator();
     m_menu->insertItem(i18n("Properties"), this, SLOT(fileProperties()));
  }
  else
  {
     m_menu->insertTitle(i18n("Selected Files"));
     m_menu->insertItem(i18n("Copy"), this, SLOT(copySelection()));
     m_menu->insertItem(i18n("Delete"), this, SLOT(deleteFiles()));
  }
  m_menu->popup(p, 1);
}

void KfindWindow::slotOpenWith()
{
   KOpenWithHandler::getOpenWithHandler()->displayOpenWithDialog( KURL::split(((KfFileLVI*)currentItem())->fileitem.url()) );
}


