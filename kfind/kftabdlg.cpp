/***********************************************************************
 *
 *  kftabdlg.cpp
 *
 **********************************************************************/

#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qtooltip.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qpushbutton.h>
#include <qapplication.h>

#include <kcalendarsystem.h>
#include <kglobal.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kregexpeditorinterface.h>
#include <kparts/componentfactory.h>
#include <kstandarddirs.h>

#include "kquery.h"
#include "kftabdlg.h"

// Static utility functions
static void save_pattern(QComboBox *, const QString &, const QString &);

#define SPECIAL_TYPES 7

class KSortedMimeTypeList : public QPtrList<KMimeType>
{
public:
  KSortedMimeTypeList() { };
  int compareItems(QPtrCollection::Item s1, QPtrCollection::Item s2)
  {
     KMimeType *item1 = (KMimeType *) s1;
     KMimeType *item2 = (KMimeType *) s2;
     if (item1->comment() > item2->comment()) return 1;
     if (item1->comment() == item2->comment()) return 0;
     return -1;
  }
};

KfindTabWidget::KfindTabWidget(QWidget *parent, const char *name)
  : QTabWidget( parent, name ), regExpDialog(0)
{
    // This validator will be used for all numeric edit fields
    //KDigitValidator *digitV = new KDigitValidator(this);

    // ************ Page One ************

    pages[0] = new QWidget( this, "page1" );

    nameBox = new KComboBox(TRUE, pages[0], "combo1");
    QLabel * namedL = new QLabel(nameBox, i18n("&Named:"), pages[0], "named");
    QToolTip::add( namedL, i18n("You can use wildcard matching and \";\" for separating multiple names") );
    dirBox  = new KComboBox(TRUE, pages[0], "combo2");
    QLabel * lookinL = new QLabel(dirBox, i18n("Look &in:"), pages[0], "named");
    subdirsCb  = new QCheckBox(i18n("Include &subfolders"), pages[0]);
    caseSensCb  = new QCheckBox(i18n("Case s&ensitive search"), pages[0]);
    browseB    = new QPushButton(i18n("&Browse..."), pages[0]);
    useLocateCb = new QCheckBox(i18n("&Use files index"), pages[0]);

    // Setup

    subdirsCb->setChecked(true);
    caseSensCb->setChecked(false);
    useLocateCb->setChecked(false);
    if(KStandardDirs::findExe("locate")==NULL)
    	useLocateCb->setEnabled(false);

    nameBox->setDuplicatesEnabled(FALSE);
    nameBox->setFocus();
    dirBox->setDuplicatesEnabled(FALSE);

    nameBox->setInsertionPolicy(QComboBox::AtTop);
    dirBox->setInsertionPolicy(QComboBox::AtTop);

    const QString nameWhatsThis
      = i18n("<qt>Enter the filename you are looking for. <br>"
	     "Alternatives may be separated by a semicolon \";\".<br>"
	     "<br>"
	     "The filename may contain the following special characters:"
	     "<ul>"
	     "<li><b>?</b> matches any single character</li>"
	     "<li><b>*</b> matches zero or more of any characters</li>"
	     "<li><b>[...]</b> matches any of the characters in braces</li>"
	     "</ul>"
	     "<br>"
	     "Example searches:"
	     "<ul>"
	     "<li><b>*.kwd;*.txt</b> finds all files ending with .kwd or .txt</li>"
	     "<li><b>go[dt]</b> finds god and got</li>"
	     "<li><b>Hel?o</b> finds all files that start with \"Hel\" and end with \"o\", "
	     "having one character in between</li>"
	     "<li><b>My Document.kwd</b> finds a file of exactly that name</li>"
	     "</ul></qt>");
    QWhatsThis::add(nameBox,nameWhatsThis);
    QWhatsThis::add(namedL,nameWhatsThis);
    const QString whatsfileindex
        = i18n("<qt>This lets you use the files' index created by the <i>slocate</i> "
               "package to speed-up the search; remember to update the index from time to time "
               "(using <i>updatedb</i>)."
               "</qt>");
    QWhatsThis::add(useLocateCb,whatsfileindex);

    // Layout

    QGridLayout *grid = new QGridLayout( pages[0], 3, 2,
					 KDialog::marginHint(),
					 KDialog::spacingHint() );
    QBoxLayout *subgrid = new QHBoxLayout( -1 , "subgrid" );
    grid->addWidget( namedL, 0, 0 );
    grid->addMultiCellWidget( nameBox, 0, 0, 1, 2 );
    grid->addWidget( lookinL, 1, 0 );
    grid->addWidget( dirBox, 1, 1 );
    grid->addWidget( browseB, 1, 2);
    grid->setColStretch(1,1);
    grid->addMultiCellLayout( subgrid, 2, 2, 1, 2 );
    subgrid->addWidget( subdirsCb );
    subgrid->addSpacing( KDialog::spacingHint() );
    subgrid->addWidget( caseSensCb);
    subgrid->addWidget( useLocateCb );
   subgrid->addStretch(1);

    // Signals

    connect( browseB, SIGNAL(clicked()),
             this, SLOT(getDirectory()) );

    connect( nameBox, SIGNAL(activated(int)),
             this, SIGNAL(startSearch()));

    // ************ Page Two

    pages[1] = new QWidget( this, "page2" );

    findCreated =  new QCheckBox(i18n("Find all files created or &modified:"), pages[1]);
    bg  = new QButtonGroup();
    rb[0] = new QRadioButton(i18n("&between"), pages[1] );
    rb[1] = new QRadioButton(i18n("&during the previous"), pages[1] );
    QLabel * andL   = new QLabel(i18n("and"), pages[1], "and");
    betweenType = new KComboBox(FALSE, pages[1], "comboBetweenType");
    betweenType->insertItem(i18n("minute(s)"));
    betweenType->insertItem(i18n("hour(s)"));
    betweenType->insertItem(i18n("day(s)"));
    betweenType->insertItem(i18n("month(s)"));
    betweenType->insertItem(i18n("year(s)"));
    betweenType->setCurrentItem(1);


    QDate dt = KGlobal::locale()->calendar()->addYears(QDate::currentDate(), -1);

    fromDate = new KDateCombo(dt, pages[1], "fromDate");
    toDate = new KDateCombo(pages[1], "toDate");
    timeBox = new QSpinBox(1, 60, 1, pages[1], "timeBox");

    sizeBox =new KComboBox(FALSE, pages[1], "sizeBox");
    QLabel * sizeL   =new QLabel(sizeBox,i18n("File &size is:"), pages[1],"size");
    sizeEdit=new QSpinBox(0, INT_MAX, 1, pages[1], "sizeEdit" );
    sizeEdit->setValue(1);
    sizeUnitBox =new KComboBox(FALSE, pages[1], "sizeUnitBox");

    m_usernameBox = new KComboBox( true, pages[1], "m_combo1");
    QLabel *usernameLabel= new QLabel(m_usernameBox,i18n("Files owned by &user:"),pages[1]);
    m_groupBox = new KComboBox( true, pages[1], "m_combo2");
    QLabel *groupLabel= new QLabel(m_groupBox,i18n("Owned by &group:"),pages[1]);

    sizeBox ->insertItem( i18n("(none)") );
    sizeBox ->insertItem( i18n("At Least") );
    sizeBox ->insertItem( i18n("At Most") );
    sizeBox ->insertItem( i18n("Equal To") );

    sizeUnitBox ->insertItem( i18n("Bytes") );
    sizeUnitBox ->insertItem( i18n("KB") );
    sizeUnitBox ->insertItem( i18n("MB") );
    sizeUnitBox ->setCurrentItem(1);

    int tmp = sizeEdit->fontMetrics().width(" 000000000 ");
    sizeEdit->setMinimumSize(tmp, sizeEdit->sizeHint().height());

    m_usernameBox->setDuplicatesEnabled(FALSE);
    m_groupBox->setDuplicatesEnabled(FALSE);
    m_usernameBox->setInsertionPolicy(QComboBox::AtTop);
    m_groupBox->setInsertionPolicy(QComboBox::AtTop);


    // Setup
    timeBox->setButtonSymbols(QSpinBox::PlusMinus);
    rb[0]->setChecked(true);
    bg->insert( rb[0] );
    bg->insert( rb[1] );

    // Layout

    QGridLayout *grid1 = new QGridLayout( pages[1], 5,  6,
					  KDialog::marginHint(),
					  KDialog::spacingHint() );

    grid1->addMultiCellWidget(findCreated, 0, 0, 0, 3 );
    grid1->addColSpacing(0, KDialog::spacingHint());

    grid1->addWidget(rb[0], 1, 1 );
    grid1->addWidget(fromDate, 1, 2 );
    grid1->addWidget(andL, 1, 3, AlignHCenter );
    grid1->addWidget(toDate, 1, 4 );

    grid1->addWidget(rb[1], 2, 1 );
    grid1->addMultiCellWidget(timeBox, 2, 2, 2, 3);
    grid1->addWidget(betweenType, 2, 4 );

    grid1->addMultiCellWidget(sizeL,3,3,0,1);
    grid1->addWidget(sizeBox,3,2);
    grid1->addWidget(sizeEdit,3,3);
    grid1->addWidget(sizeUnitBox,3,4);

    grid1->addMultiCellWidget(usernameLabel,4,4,0,1);
    grid1->addWidget(m_usernameBox,4,2);
    grid1->addWidget(groupLabel,4,3);
    grid1->addWidget(m_groupBox,4,4);

    for (int c=1; c<=4; c++)
       grid1->setColStretch(c,1);

    grid1->setRowStretch(6,1);

    // Connect
    connect( findCreated,  SIGNAL(toggled(bool)),   SLOT(fixLayout()) );
    connect( bg,  SIGNAL(clicked(int)), this,   SLOT(fixLayout()) );
    connect( sizeBox, SIGNAL(highlighted(int)), this, SLOT(slotSizeBoxChanged(int)));


    // ************ Page Three

    pages[2] = new QWidget( this, "page3" );

    typeBox =new KComboBox(FALSE, pages[2], "typeBox");
    QLabel * typeL   =new QLabel(typeBox, i18n("File &type:"), pages[2], "type");
    textEdit=new KLineEdit(pages[2], "textEdit" );
    QLabel * textL   =new QLabel(textEdit, i18n("C&ontaining text:"), pages[2], "text");

    const QString containingtext
      = i18n("<qt>If specified, only files that contain this text"
	      " are found. Note that not all file types from the list"
		  " above are supported. Please refer to the documentation"
		  " for a list of supported file types."
	      "</qt>");
    QToolTip::add(textEdit,containingtext);
    QWhatsThis::add(textL,containingtext);

    caseContextCb  =new QCheckBox(i18n("Case s&ensitive"), pages[2]);
    binaryContextCb  =new QCheckBox(i18n("Include &binary files"), pages[2]);
    regexpContentCb  =new QCheckBox(i18n("Regular e&xpression"), pages[2]);

    const QString binaryTooltip
      = i18n("<qt>This lets you search in any type of file, "
       "even those that usually do not contain text (for example "
	   "program files and images).</qt>");
    QToolTip::add(binaryContextCb,binaryTooltip);

    QPushButton* editRegExp = 0;
    if ( !KTrader::self()->query("KRegExpEditor/KRegExpEditor").isEmpty() ) {
      // The editor is available, so lets use it.
      editRegExp = new QPushButton(i18n("&Edit..."), pages[2], "editRegExp");
    }

    metainfokeyEdit=new KLineEdit(pages[2], "textEdit" );
    metainfoEdit=new KLineEdit(pages[2], "textEdit" );
    QLabel * textMetaInfo = new QLabel(metainfoEdit, i18n("fo&r:"), pages[2], "text");
    QLabel * textMetaKey = new QLabel(metainfokeyEdit, i18n("Search &metainfo sections:"), pages[2], "text");

    // Setup
    typeBox->insertItem(i18n("All Files & Folders"));
    typeBox->insertItem(i18n("Files"));
    typeBox->insertItem(i18n("Folders"));
    typeBox->insertItem(i18n("Symbolic Links"));
    typeBox->insertItem(i18n("Special Files (Sockets, Device Files...)"));
    typeBox->insertItem(i18n("Executable Files"));
    typeBox->insertItem(i18n("SUID Executable Files"));
    typeBox->insertItem(i18n("All Images"));
    typeBox->insertItem(i18n("All Video"));
    typeBox->insertItem(i18n("All Sounds"));

    initMimeTypes();
    initSpecialMimeTypes();

    for ( KMimeType::List::ConstIterator it = m_types.begin();
          it != m_types.end(); ++it )
    {
      KMimeType::Ptr typ = *it;
      typeBox->insertItem(typ->pixmap( KIcon::Small ), typ->comment());
    }

    if ( editRegExp ) {
      // The editor was available, so lets use it.
      connect( regexpContentCb, SIGNAL(toggled(bool) ), editRegExp, SLOT(setEnabled(bool)) );
      editRegExp->setEnabled(false);
      connect( editRegExp, SIGNAL(clicked()), this, SLOT( slotEditRegExp() ) );
    }
    else
        regexpContentCb->hide();

    // Layout
    tmp = sizeEdit->fontMetrics().width(" 00000 ");
    sizeEdit->setMinimumSize(tmp, sizeEdit->sizeHint().height());

    QGridLayout *grid2 = new QGridLayout( pages[2], 5, 4,
					  KDialog::marginHint(),
					  KDialog::spacingHint() );
    grid2->addWidget( typeL, 0, 0 );
    grid2->addWidget( textL, 1, 0 );
    grid2->addMultiCellWidget( typeBox, 0, 0, 1, 3 );
    grid2->addMultiCellWidget( textEdit, 1, 1, 1, 3 );
    grid2->addWidget( regexpContentCb, 2, 2);
    grid2->addWidget( caseContextCb, 2, 1 );
    grid2->addWidget( binaryContextCb, 3, 1);

    grid2->addWidget( textMetaKey, 4, 0 );
    grid2->addWidget( metainfokeyEdit, 4, 1 );
    grid2->addWidget( textMetaInfo, 4, 2, AlignHCenter  );
    grid2->addWidget( metainfoEdit, 4, 3 );

    metainfokeyEdit->setText("*");

    if ( editRegExp ) {
      // The editor was available, so lets use it.
      grid2->addWidget( editRegExp, 2, 3 );
    }

    addTab( pages[0], i18n("Name/&Location") );
    addTab( pages[2], i18n("C&ontents") );
    addTab( pages[1], i18n("&Properties") );


    // Setup
    const QString whatsmetainfo
      = i18n("<qt>Search within files' specific comments/metainfo<br>"
	     "These are some examples:<br>"
	     "<ul>"
	     "<li><b>Audio files (mp3...)</b> Search in id3 tag for a title, an album</li>"
	     "<li><b>Images (png...)</b> Search images with a special resolution, comment...</li>"
	     "</ul>"
	     "</qt>");
    const QString whatsmetainfokey
      = i18n("<qt>If specified, search only in this field<br>"
	     "<ul>"
	     "<li><b>Audio files (mp3...)</b> This can be Title, Album...</li>"
	     "<li><b>Images (png...)</b> Search only in Resolution, Bitdepth...</li>"
	     "</ul>"
	     "</qt>");
    QWhatsThis::add(textMetaInfo,whatsmetainfo);
    QToolTip::add(metainfoEdit,whatsmetainfo);
    QWhatsThis::add(textMetaKey,whatsmetainfokey);
    QToolTip::add(metainfokeyEdit,whatsmetainfokey);


    fixLayout();
    loadHistory();
}

KfindTabWidget::~KfindTabWidget()
{
  delete pages[0];
  delete pages[1];
  delete pages[2];
}

void KfindTabWidget::setURL( const KURL & url )
{
  KConfig *conf = KGlobal::config();
  conf->setGroup("History");
  m_url = url;
  QStringList sl = conf->readPathListEntry("Directories");
  dirBox->clear(); // make sure there is no old Stuff in there

  if(!sl.isEmpty()) {
    dirBox->insertStringList(sl);
    // If the _searchPath already exists in the list we do not
    // want to add it again
    int indx = sl.findIndex(m_url.url());
    if(indx == -1)
      dirBox->insertItem(m_url.url(), 0); // make it the first one
    else
      dirBox->setCurrentItem(indx);
  }
  else {
    QDir m_dir("/lib");
    dirBox ->insertItem( m_url.url() );
    dirBox ->insertItem( "file:" + QDir::homeDirPath() );
    dirBox ->insertItem( "file:/" );
    dirBox ->insertItem( "file:/usr" );
    if (m_dir.exists())
      dirBox ->insertItem( "file:/lib" );
    dirBox ->insertItem( "file:/home" );
    dirBox ->insertItem( "file:/etc" );
    dirBox ->insertItem( "file:/var" );
    dirBox ->insertItem( "file:/mnt" );
  }
}

void KfindTabWidget::initMimeTypes()
{
    KMimeType::List tmp = KMimeType::allMimeTypes();
    KSortedMimeTypeList sortedList;
    for ( KMimeType::List::ConstIterator it = tmp.begin();
          it != tmp.end(); ++it )
    {
      KMimeType * type = *it;
      if ((!type->comment().isEmpty())
         && (!type->name().startsWith("kdedevice/"))
         && (!type->name().startsWith("all/")))
        sortedList.append(type);
    }
    sortedList.sort();
    for ( KMimeType *type = sortedList.first(); type; type = sortedList.next())
    {
       m_types.append(type);
    }
}

void KfindTabWidget::initSpecialMimeTypes()
{
    KMimeType::List tmp = KMimeType::allMimeTypes();

    for ( KMimeType::List::ConstIterator it = tmp.begin(); it != tmp.end(); ++it )
    {
      KMimeType * type = *it;

      if(!type->comment().isEmpty()) {
        if(type->name().startsWith("image/"))
           m_ImageTypes.append(type->name());
        else if(type->name().startsWith("video/"))
          m_VideoTypes.append(type->name());
        else if(type->name().startsWith("audio/"))
          m_AudioTypes.append(type->name());
      }
    }
}

void KfindTabWidget::saveHistory()
{
  save_pattern(nameBox, "History", "Patterns");
  save_pattern(dirBox, "History", "Directories");
}

void KfindTabWidget::loadHistory()
{
  // Load pattern history
  KConfig *conf = KGlobal::config();
  conf->setGroup("History");
  QStringList sl = conf->readListEntry("Patterns");
  if(!sl.isEmpty())
    nameBox->insertStringList(sl);
  else
    nameBox->insertItem("*");

  sl = conf->readPathListEntry("Directories");
  if(!sl.isEmpty()) {
    dirBox->insertStringList(sl);
    // If the _searchPath already exists in the list we do not
    // want to add it again
    int indx = sl.findIndex(m_url.url());
    if(indx == -1)
      dirBox->insertItem(m_url.url(), 0); // make it the first one
    else
      dirBox->setCurrentItem(indx);
  }
  else {
    QDir m_dir("/lib");
    dirBox ->insertItem( m_url.url() );
    dirBox ->insertItem( "file:" + QDir::homeDirPath() );
    dirBox ->insertItem( "file:/" );
    dirBox ->insertItem( "file:/usr" );
    if (m_dir.exists())
      dirBox ->insertItem( "file:/lib" );
    dirBox ->insertItem( "file:/home" );
    dirBox ->insertItem( "file:/etc" );
    dirBox ->insertItem( "file:/var" );
    dirBox ->insertItem( "file:/mnt" );
  }
}

void KfindTabWidget::slotEditRegExp()
{
  if ( ! regExpDialog )
    regExpDialog = KParts::ComponentFactory::createInstanceFromQuery<QDialog>( "KRegExpEditor/KRegExpEditor", QString::null, this );

  KRegExpEditorInterface *iface = static_cast<KRegExpEditorInterface *>( regExpDialog->qt_cast( "KRegExpEditorInterface" ) );
  if ( !iface )
       return;

  iface->setRegExp( textEdit->text() );
  bool ok = regExpDialog->exec();
  if ( ok )
    textEdit->setText( iface->regExp() );
}

void KfindTabWidget::setFocus()
{
  nameBox->setFocus();
  nameBox->lineEdit()->selectAll();
}

void KfindTabWidget::slotSizeBoxChanged(int index)
{
  sizeEdit->setEnabled((bool)(index != 0));
  sizeUnitBox->setEnabled((bool)(index != 0));
}

void KfindTabWidget::setDefaults()
{
    QDate dt = KGlobal::locale()->calendar()->addYears(QDate::currentDate(), -1);

    fromDate ->setDate(dt);
    toDate ->setDate(QDate::currentDate());

    timeBox->setValue(1);
    betweenType->setCurrentItem(1);

    typeBox ->setCurrentItem(0);
    sizeBox ->setCurrentItem(0);
    sizeUnitBox ->setCurrentItem(1);
    sizeEdit->setValue(1);
}

/*
  Checks if dates are correct and popups a error box
  if they are not.
*/
bool KfindTabWidget::isDateValid()
{
  // All files
  if ( !findCreated->isChecked() ) return TRUE;

  if (rb[1]->isChecked())
  {
    if (timeBox->value() > 0 ) return TRUE;

    KMessageBox::sorry(this, i18n("Unable to search within a period which is less than a minute."));
    return FALSE;
  }

  // If we can not parse either of the dates or
  // "from" date is bigger than "to" date return FALSE.
  QDate hi1, hi2;

  QString str;
  if ( ! fromDate->getDate(&hi1).isValid() ||
       ! toDate->getDate(&hi2).isValid() )
    str = i18n("The date is not valid.");
  else if ( hi1 > hi2 )
    str = i18n("Invalid date range.");
  else if ( QDate::currentDate() < hi1 )
    str = i18n("Unable to search dates in the future.");

  if (!str.isNull()) {
    KMessageBox::sorry(0, str);
    return FALSE;
  }
  return TRUE;
}

void KfindTabWidget::setQuery(KQuery *query)
{
	int size;
  bool itemAlreadyContained(false);
  // only start if we have valid dates
  if (!isDateValid()) return;

  query->setPath(KURL(dirBox->currentText().stripWhiteSpace()));

  for (int idx=0; idx<dirBox->count(); idx++)
     if (dirBox->text(idx)==dirBox->currentText())
        itemAlreadyContained=true;

  if (!itemAlreadyContained)
     dirBox->insertItem(dirBox->currentText().stripWhiteSpace(),0);

  QString regex = nameBox->currentText().isEmpty() ? "*" : nameBox->currentText();
  query->setRegExp(regex, caseSensCb->isChecked());
  itemAlreadyContained=false;
  for (int idx=0; idx<nameBox->count(); idx++)
     if (nameBox->text(idx)==nameBox->currentText())
        itemAlreadyContained=true;

  if (!itemAlreadyContained)
     nameBox->insertItem(nameBox->currentText(),0);

  query->setRecursive(subdirsCb->isChecked());

  switch (sizeUnitBox->currentItem())
  {
     case 0:
         size = 1; //one byte
			break;
     case 2:
         size = 1048576; //1M
			break;
		case 1:
		default:
			size=1024; //1k
			break;
  }
  size = sizeEdit->value() * size;
  if (size < 0)  // overflow
     if (KMessageBox::warningYesNo(this, i18n("Size is too big... Set maximum size value?"), i18n("Error"))
           == KMessageBox::Yes)
		{
         sizeEdit->setValue(INT_MAX);
	   	sizeUnitBox->setCurrentItem(0);
		   size = INT_MAX;
		}
     else
        return;

  switch (sizeBox->currentItem())
  {
    case 1:
      query->setSizeRange(size, -1);
      break;
    case 2:
      query->setSizeRange(-1, size);
      break;
    case 3:
      query->setSizeRange(size,size);
      break;
    default:
      query->setSizeRange(-1, -1);
  }

  // dates
  QDateTime epoch;
  epoch.setTime_t(0);

  // Add date predicate
  if (findCreated->isChecked()) { // Modified
    if (rb[0]->isChecked()) { // Between dates
      QDate q1, q2;
      fromDate->getDate(&q1);
      toDate->getDate(&q2);

      // do not generate negative numbers .. find doesn't handle that
      time_t time1 = epoch.secsTo(q1);
      time_t time2 = epoch.secsTo(q2.addDays(1)) - 1; // Include the last day

      query->setTimeRange(time1, time2);
    }
    else
    {
       time_t cur = time(NULL);
       time_t minutes = cur;

       switch (betweenType->currentItem())
       {
          case 0: // minutes
                 minutes = timeBox->value();
 	              break;
          case 1: // hours
                 minutes = 60 * timeBox->value();
 	              break;
          case 2: // days
                 minutes = 60 * 24 * timeBox->value();
 	              break;
          case 3: // months
                 minutes = 60 * 24 * (time_t)(timeBox->value() * 30.41667);
 	              break;
          case 4: // years
                 minutes = 12 * 60 * 24 * (time_t)(timeBox->value() * 30.41667);
 	              break;
       }

       query->setTimeRange(cur - minutes * 60, 0);
    }
  }
  else
    query->setTimeRange(0, 0);

  query->setUsername( m_usernameBox->currentText() );
  query->setGroupname( m_groupBox->currentText() );

  query->setFileType(typeBox->currentItem());

  int id = typeBox->currentItem()-10;

  if ((id >= -3) && (id < (int) m_types.count()))
  {
    switch(id)
    {
      case -3:
        query->setMimeType( m_ImageTypes );
        break;
      case -2:
        query->setMimeType( m_VideoTypes );
        break;
      case -1:
        query->setMimeType( m_AudioTypes );
        break;
      default:
        query->setMimeType( m_types[id]->name() );
     }
  }
  else
  {
     query->setMimeType( QString::null );
  }

  //Metainfo
  query->setMetaInfo(metainfoEdit->text(), metainfokeyEdit->text());

  //Use locate to speed-up search ?
  query->setUseFileIndex(useLocateCb->isChecked());

  query->setContext(textEdit->text(), caseContextCb->isChecked(),
  	binaryContextCb->isChecked(), regexpContentCb->isChecked());
}

QString KfindTabWidget::date2String(const QDate & date) {
  return(KGlobal::locale()->formatDate(date, true));
}

QDate &KfindTabWidget::string2Date(const QString & str, QDate *qd) {
  return *qd = KGlobal::locale()->readDate(str);
}

void KfindTabWidget::getDirectory()
{
  QString result =
  KFileDialog::getExistingDirectory( dirBox->text(dirBox->currentItem()).stripWhiteSpace(),
                                     this );

  if (!result.isEmpty())
  {
    for (int i = 0; i < dirBox->count(); i++)
      if (result == dirBox->text(i)) {
	dirBox->setCurrentItem(i);
	return;
      }
    dirBox->insertItem(result, 0);
    dirBox->setCurrentItem(0);
  }
}

void KfindTabWidget::beginSearch()
{
///  dirlister->openURL(KURL(dirBox->currentText().stripWhiteSpace()));

  saveHistory();
  setEnabled( FALSE );
}

void KfindTabWidget::endSearch()
{
  setEnabled( TRUE );
}

/*
  Disables/enables all edit fields depending on their
  respective check buttons.
*/
void KfindTabWidget::fixLayout()
{
  int i;
  // If "All files" is checked - disable all edits
  // and second radio group on page two

  if(! findCreated->isChecked())  {
    fromDate->setEnabled(FALSE);
    toDate->setEnabled(FALSE);
    timeBox->setEnabled(FALSE);
    for(i=0; i<2; i++)
      rb[i]->setEnabled(FALSE);
    betweenType->setEnabled(FALSE);
  }
  else {
    for(i=0; i<2; i++)
      rb[i]->setEnabled(TRUE);

    fromDate->setEnabled(rb[0]->isChecked());
    toDate->setEnabled(rb[0]->isChecked());
    timeBox->setEnabled(rb[1]->isChecked());
    betweenType->setEnabled(rb[1]->isChecked());
  }

  // Size box on page three
  sizeEdit->setEnabled(sizeBox->currentItem() != 0);
  sizeUnitBox->setEnabled(sizeBox->currentItem() != 0);
}

bool KfindTabWidget::isSearchRecursive()
{
  return subdirsCb->isChecked();
}


/**
   Digit validator. Allows only digits to be typed.
**/
KDigitValidator::KDigitValidator( QWidget * parent, const char *name )
  : QValidator( parent, name )
{
  r = new QRegExp("^[0-9]*$");
}

KDigitValidator::~KDigitValidator()
{
  delete r;
}

QValidator::State KDigitValidator::validate( QString & input, int & ) const
{
  if (r->search(input) < 0) {
    // Beep on user if he enters non-digit
    QApplication::beep();
    return QValidator::Invalid;
  }
  else
    return QValidator::Acceptable;
}

//*******************************************************
//             Static utility functions
//*******************************************************
static void save_pattern(QComboBox *obj,
			 const QString & group, const QString & entry)
{
  // QComboBox allows insertion of items more than specified by
  // maxCount() (QT bug?). This API call will truncate list if needed.
  obj->setMaxCount(15);

  // make sure the current item is saved first so it will be the
  // default when started next time
  QStringList sl;
  QString cur = obj->currentText();
  sl.append(cur);
  for (int i = 0; i < obj->count(); i++) {
    if( cur != obj->text(i) ) {
      sl.append(obj->text(i));
    }
  }

  KConfig *conf = KGlobal::config();
  conf->setGroup(group);
  conf->writePathEntry(entry, sl);
}

#include "kftabdlg.moc"
