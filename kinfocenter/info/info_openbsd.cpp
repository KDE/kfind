/*
 * info_netbsd.cpp is part of the KDE program kcminfo.  This displays
 * various information about the OpenBSD system it's running on.
 *
 * Originally written by Jaromir Dolecek <dolecek@ics.muni.cz>. CPU info
 * code has been imported from implementation of processor.cpp for KDE 1.0
 * by David Brownlee <abs@NetBSD.org> as found in NetBSD packages collection.
 * Hubert Feyer <hubertf@NetBSD.org> enhanced the sound information printing
 * quite a lot, too.
 *
 * The code is placed into public domain. Do whatever you want with it.
 */

#define INFO_CPU_AVAILABLE
#define INFO_IRQ_AVAILABLE
#define INFO_DMA_AVAILABLE
#define INFO_PCI_AVAILABLE
#define INFO_IOPORTS_AVAILABLE
#define INFO_SOUND_AVAILABLE
#define INFO_DEVICES_AVAILABLE
#define INFO_SCSI_AVAILABLE
#define INFO_PARTITIONS_AVAILABLE
#define INFO_XSERVER_AVAILABLE


/*
 * all following functions should return true, when the Information
 * was filled into the lBox-Widget. Returning false indicates that
 * information was not available.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <stdio.h>	/* for NULL */
#include <stdlib.h>	/* for malloc(3) */

#include <QFile>
#include <QFontMetrics>
#include <Q3StrList>
#include <QTextStream>

#include <kdebug.h>

typedef struct
  {
  int	string;
  int	name;
  const char	*title;
  } hw_info_mib_list_t;

bool GetInfo_CPU(Q3ListView *lBox)
{
  static hw_info_mib_list_t hw_info_mib_list[]= {
	{ 1, HW_MODEL,		"Model" },
	{ 1, HW_MACHINE,	"Machine" },
	{ 0, HW_NCPU,		"Number of CPUs" },
	{ 0, HW_PAGESIZE,	"Pagesize" },
	{ 0,0,0 }
	};
  hw_info_mib_list_t *hw_info_mib;

  int mib[2], num;
  char *buf;
  size_t len;
  QString value;

  lBox->addColumn(i18n("Information"));
  lBox->addColumn(i18n("Value"));

  for ( hw_info_mib = hw_info_mib_list ;  hw_info_mib->title ; ++hw_info_mib )
  {
	mib[0] = CTL_HW;
	mib[1] = hw_info_mib->name;
	if ( hw_info_mib->string ) {
		sysctl(mib,2,NULL,&len,NULL,0);
		if ( (buf = (char*)malloc(len)) ) {
			sysctl(mib,2,buf,&len,NULL,0);
			value = QString::fromLocal8Bit(buf);
			free(buf);
		}
		else {
			value = QString("Unknown");
		}
	}
	else {
		len = sizeof(num);
		sysctl(mib,2,&num,&len,NULL,0);
		value.sprintf("%d", num);
	}
	new Q3ListViewItem(lBox, hw_info_mib->title, value);
   }

   return true;
}

// this is used to find out which devices are currently
// on system
static bool GetDmesgInfo(Q3ListView *lBox, const char *filter,
	void func(Q3ListView *, QString s, void **, bool))
{
        QFile *dmesg = new QFile("/var/run/dmesg.boot");
	bool usepipe=false;
	FILE *pipe=NULL;
	QTextStream *t;
	bool seencpu=false;
	void *opaque=NULL;
	QString s;
	bool found=false;

	if (dmesg->exists() && dmesg->open(QIODevice::ReadOnly)) {
		t = new QTextStream(dmesg);
	}
	else {
		delete dmesg;
		pipe = popen("/sbin/dmesg", "r");
		if (!pipe) return false;
		usepipe = true;
		t = new QTextStream(pipe, QIODevice::ReadOnly);
	}

	Q3ListViewItem *olditem = NULL;
	while(!(s = t->readLine()).isNull()) {
		if (!seencpu) {
			if (s.contains("cpu"))
				seencpu = true;
			else
				continue;
		}
		if (s.contains("boot device") ||
			s.contains("WARNING: old BSD partition ID!"))
			break;

		if (!filter || s.contains(filter)) {
			if (func) {
				func(lBox, s, &opaque, false);
			}
			else {
				olditem = new Q3ListViewItem(lBox, olditem, s);
			}
			found = true;
		}
	}
	if (func) {
		func(lBox, s, &opaque, true);
	}
	//lBox->triggerUpdate();

	delete t;
	if (pipe) {
		pclose(pipe);
	}
	else {
		dmesg->close();
		delete dmesg;
	}

	return found;
}


void AddIRQLine(Q3ListView *lBox, QString s, void **opaque, bool ending)
{
	Q3StrList *strlist = (Q3StrList *) *opaque;
	const char *str;
	int pos, irqnum=0;
	const char *p;
	p = s.toLatin1();

	if (!strlist) {
		strlist = new Q3StrList();
		*opaque = (void *) strlist;
	}
	if (ending) {
		str = strlist->first();
		for(;str; str = strlist->next()) {
			new Q3ListViewItem(lBox, str);
		}
		delete strlist;
		return;
	}

	pos = s.find(" irq ");
	irqnum = (pos < 0) ? 0 : atoi(&p[pos+5]);
	if (irqnum) {
		s.sprintf("%02d%s", irqnum, p);
	}
	else {
		s.sprintf("??%s", p);
	}
	strlist->inSort(s.toLatin1());
}

bool GetInfo_IRQ (Q3ListView *lBox)
{
	lBox->addColumn(i18n("IRQ"));
	lBox->addColumn(i18n("Device"));
	(void) GetDmesgInfo(lBox, " irq ", AddIRQLine);
	return true;
}

bool GetInfo_DMA (Q3ListView *)
{
  return false;
}

bool GetInfo_PCI (Q3ListView *lbox)
{
	if (!GetDmesgInfo(lbox, "at pci", NULL))
		new Q3ListViewItem(lbox, i18n("No PCI devices found."));
	return true;
}

bool GetInfo_IO_Ports (Q3ListView *lbox)
{
	if (!GetDmesgInfo(lbox, "port 0x", NULL))
		new Q3ListViewItem(lbox, i18n("No I/O port devices found."));
	return true;
}

bool GetInfo_Sound (Q3ListView *lbox)
{
	if (!GetDmesgInfo(lbox, "audio", NULL))
		new Q3ListViewItem(lbox, i18n("No audio devices found."));

	// append information on any audio devices found
	Q3ListViewItem *lvitem = lbox->firstChild();
	for(; lvitem; lvitem = lvitem->nextSibling()) {
		QString s;
		int pos, len;
		const char *start, *end;
		char *dev;

		s = lvitem->text(0);
		if ((pos = s.find("at ")) >= 0) {
			pos += 3;	// skip "at "
			start = end = s.toAscii();
			for(; *end && (*end!=':') && (*end!='\n'); end++);
			len = end - start;
			dev = (char *) malloc(len + 1);
			strncpy(dev, start, len);
			dev[len] = '\0';

			GetDmesgInfo(lbox, dev, NULL);

			free(dev);
		}
	}

	return true;
}

bool GetInfo_Devices (Q3ListView *lBox)
{
	(void) GetDmesgInfo(lBox, NULL, NULL);
	return true;
}

bool GetInfo_SCSI (Q3ListView *lbox)
{
	if (!GetDmesgInfo(lbox, "scsibus", NULL))
		new Q3ListViewItem(lbox, i18n("No SCSI devices found."));
	return true;
}

bool GetInfo_Partitions (Q3ListView *lbox)
{
	QString s;
	char *line, *orig_line;
	const char *device, *mountpoint, *type, *flags;
	FILE *pipe = popen("/sbin/mount", "r");
	QTextStream *t;

	if (!pipe) {
		kError(0) << i18n("Unable to run /sbin/mount.") << endl;
		return false;
	}
	t = new QTextStream(pipe, QIODevice::ReadOnly);

	lbox->addColumn(i18n("Device"));
	lbox->addColumn(i18n("Mount Point"));
	lbox->addColumn(i18n("FS Type"));
	lbox->addColumn(i18n("Mount Options"));

	Q3ListViewItem *olditem = 0;
	while (!(s = t->readLine()).isNull()) {
		orig_line = line = strdup(s.toLatin1());

		device = strsep(&line, " ");

		(void) strsep(&line, " "); // consume word "on"
		mountpoint = strsep(&line, " ");

		(void) strsep(&line, " "); // consume word "type"
		type = strsep(&line, " ");

		flags = line;

		olditem = new Q3ListViewItem(lbox, olditem, device, mountpoint,
					type, flags);

		free(orig_line);
	}

	delete t;
	pclose(pipe);
	return true;
}

bool GetInfo_XServer_and_Video (Q3ListView *lBox)
{
	return GetInfo_XServer_Generic( lBox );
}
