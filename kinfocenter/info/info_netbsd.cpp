/*
 * info_netbsd.cpp is part of the KDE program kcminfo.  This displays
 * various information about the NetBSD system it's running on.
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
#include <sys/mount.h>
#include <stdio.h>	/* for NULL */
#include <stdlib.h>	/* for malloc(3) */
#include <fstab.h>

#include <QFile>
#include <QFontMetrics>
#include <Q3StrList>
#include <QTextStream>

#include <kdebug.h>
#include <kio/global.h> /* for KIO::convertSize() */

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
	{ 1, HW_MACHINE_ARCH,	"Architecture" },
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
		value = QString::number(num);
	}
	new Q3ListViewItem(lBox, hw_info_mib->title, value);
   }

   return true;
}

// this is used to find out which devices are currently
// on system
static bool GetDmesgInfo(Q3ListView *lBox, const char *filter,
	void func(Q3ListView *, QString s))
{
        QFile *dmesg = new QFile("/var/run/dmesg.boot");
	bool usepipe = false;
	FILE *pipe = NULL;
	QTextStream *t;
	bool seencpu = false;
	QString s;
	bool found = false;

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
	while(!(s = t->readLine().toLocal8Bit()).isNull()) {
		if (!seencpu) {
			if (s.contains("cpu"))
				seencpu = true;
			else
				continue;
		}
		if (s.contains("boot device") ||
			s.contains("WARNING: old BSD partition ID!"))
			break;

		if (!filter || s.contains(QRegExp(filter))) {
			if (func)
				func(lBox, s);
			else
				olditem = new Q3ListViewItem(lBox, olditem, s);
			found = true;
		}
	}

	delete t;
	if (pipe)
		pclose(pipe);
	else {
		dmesg->close();
		delete dmesg;
	}

	return found;
}


void
AddIRQLine(Q3ListView *lBox, QString s)
{
	int irqnum;
	QString s2;
	char numstr[3];
	bool ok;

	s2 = s.mid(s.indexOf(QRegExp("[ (]irq "))+5);
	irqnum = s2.remove(QRegExp("[^0-9].*")).toInt(&ok);
	if (ok)
		snprintf(numstr, 3, "%02d", irqnum);
	else {
		// this should never happen
		strcpy(numstr, "??");
	}

	new Q3ListViewItem(lBox, numstr, s);
}

bool GetInfo_IRQ (Q3ListView *lBox)
{
	lBox->addColumn(i18n("IRQ"));
	lBox->addColumn(i18n("Device"));
	lBox->setSorting(0);
	lBox->setShowSortIndicator(false);
	(void) GetDmesgInfo(lBox, "[ (]irq ", AddIRQLine);
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
	lbox->setSorting(false);

	if (!GetDmesgInfo(lbox, "audio", NULL))
		new Q3ListViewItem(lbox, i18n("No audio devices found."));

	// append information for each audio devices found
	Q3ListViewItem *lvitem = lbox->firstChild();
	for(; lvitem; lvitem = lvitem->nextSibling()) {
		QString s, s2;
		int pos;
		char *dev;

		s = lvitem->text(0);
		// The autoconf message is in form 'audio0 at auvia0: ...'
		if (s.find("audio") == 0 && (pos = s.find(" at ")) > 0) {
			s2 = s.mid(pos+4); // skip " at "
			s2.remove(QRegExp("[:\n\t ].*"));
			dev = strdup(s2.toAscii().data());

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

	// remove the 'waiting %d seconds for devices to settle' message
	Q3ListViewItem *lvitem = lbox->firstChild();
	for(; lvitem; lvitem = lvitem->nextSibling()) {
		QString s = lvitem->text(0);

		if (s.contains("seconds for devices to settle")) {
			lbox->removeItem(lvitem);
			break;
		}
	}
	
	return true;
}

bool GetInfo_Partitions (Q3ListView *lbox)
{
	int num; // number of mounts
#ifdef HAVE_STATVFS
	struct statvfs *mnt; // mount data pointer
#else
	struct statfs *mnt; // mount data pointer
#endif

	// get mount info
	if (!(num=getmntinfo(&mnt, MNT_WAIT))) {
		kError() << "getmntinfo failed" << endl;
		return false;
	}

	// table headers
	lbox->addColumn(i18n("Device"));
	lbox->addColumn(i18n("Mount Point"));
	lbox->addColumn(i18n("FS Type"));
	lbox->addColumn(i18n("Total Size"));
	lbox->addColumn(i18n("Free Size"));
	lbox->addColumn(i18n("Total Nodes"));
	lbox->addColumn(i18n("Free Nodes"));
	lbox->addColumn(i18n("Flags"));

	// mnt points into a static array (no need to free it)
	for(; num--; ++mnt) {
		unsigned long long big[2];
		QString vv[5];

#ifdef HAVE_STATVFS
		big[0] = big[1] = mnt->f_frsize; // coerce the product
#else
		big[0] = big[1] = mnt->f_bsize; // coerce the product
#endif
		big[0] *= mnt->f_blocks;
		big[1] *= mnt->f_bavail; // FIXME: use f_bfree if root?

		// convert to strings
		vv[0] = KIO::convertSize(big[0]);
		vv[1] = QString("%1 (%2%)")
				.arg(KIO::convertSize(big[1]))
				.arg(mnt->f_blocks ? mnt->f_bavail*100/mnt->f_blocks : 0);

		vv[2] = QString("%L1").arg(mnt->f_files);
		vv[3] = QString("%L1 (%2%) ")
				.arg(mnt->f_ffree)
				.arg(mnt->f_files ? mnt->f_ffree*100/mnt->f_files : 0);

		vv[4].clear();
#ifdef HAVE_STATVFS
#define MNTF(x) if (mnt->f_flag & ST_##x) vv[4] += QLatin1String(#x " ");
#else
#define MNTF(x) if (mnt->f_flags & MNT_##x) vv[4] += QLatin1String(#x " ");
#endif
		MNTF(ASYNC)
		MNTF(DEFEXPORTED)
		MNTF(EXKERB)
		MNTF(EXNORESPORT)
		MNTF(EXPORTANON)
		MNTF(EXPORTED)
		MNTF(EXPUBLIC)
		MNTF(EXRDONLY)
#ifndef HAVE_STATVFS
		MNTF(IGNORE)
#endif
		MNTF(LOCAL)
		MNTF(NOATIME)
		MNTF(NOCOREDUMP)
		MNTF(NODEV)
		MNTF(NODEVMTIME)
		MNTF(NOEXEC)
		MNTF(NOSUID)
		MNTF(QUOTA)
		MNTF(RDONLY)
		MNTF(ROOTFS)
		MNTF(SOFTDEP)
		MNTF(SYMPERM)
		MNTF(SYNCHRONOUS)
		MNTF(UNION)
#undef MNTF

		// put it in the table
		// FIXME: there're more data but we have limited args (this is wrong! just add!)
		new Q3ListViewItem(lbox,
			// FIXME: names need pad space
			mnt->f_mntfromname,
			mnt->f_mntonname,
			mnt->f_fstypename,
			vv[0], vv[1], vv[2], vv[3], vv[4]);
	}

	// job well done
	return true;
}

bool GetInfo_XServer_and_Video (Q3ListView *lBox)
{
	return GetInfo_XServer_Generic( lBox );
}
