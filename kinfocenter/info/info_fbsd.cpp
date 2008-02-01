/*
 * info_fbsd.cpp is part of the KDE program kcminfo.  This displays
 * various information about the system (hopefully a FreeBSD system)
 * it's running on.
 *
 * All of the devinfo bits were blatantly stolen from the devinfo utility
 * provided with FreeBSD 5.0 (and later).  No gross hacks were harmed
 * during the creation of info_fbsd.cpp.  Thanks Mike.
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
#include "config-infocenter.h" // HAVE_DEVINFO_H

#include <sys/types.h>
#include <sys/sysctl.h>

#ifdef HAVE_DEVINFO_H
	extern "C" {
		#include <devinfo.h>
	}
#endif

#include <errno.h>
#include <fstab.h>
#include <string.h>

#include <Qt3Support/Q3Dict>
#include <QFile>

#include <QTextStream>

class Device {
public:
	Device (QString n=QString(), QString d=QString())
		{name=n; description=d;}
	QString name, description;
};

void ProcessChildren(QString name);
QString GetController(const QString &line);
Device *GetDevice(const QString &line);

#ifdef HAVE_DEVINFO_H
extern "C" {
	int print_irq(struct devinfo_rman *rman, void *arg);
	int print_dma(struct devinfo_rman *rman, void *arg);
	int print_ioports(struct devinfo_rman *rman, void *arg);
	int print_resource(struct devinfo_res *res, void *arg);
}
#endif

bool GetInfo_CPU (Q3ListView *lBox)
{
	// Modified 13 July 2000 for SMP by Brad Hughes - bhughes@trolltech.com

	int ncpu;
	size_t len;

	len = sizeof(ncpu);
	sysctlbyname("hw.ncpu", &ncpu, &len, NULL, 0);

	QString cpustring;
	for (int i = ncpu; i > 0; i--) {
		/* Stuff for sysctl */
		char *buf;
		int i_buf;

		// get the processor model
		sysctlbyname("hw.model", NULL, &len, NULL, 0);
		buf = new char[len];
		sysctlbyname("hw.model", buf, &len, NULL, 0);

		// get the TSC speed if we can
		len = sizeof(i_buf);
		if (sysctlbyname("machdep.tsc_freq", &i_buf, &len, NULL, 0) != -1) {
			cpustring = i18n("CPU %1: %2, %3 MHz", i, buf, i_buf/1000000);
		} else {
			cpustring = i18n("CPU %1: %2, unknown speed", i, buf);
		}

		/* Put everything in the listbox */
		new Q3ListViewItem(lBox, cpustring);

		/* Clean up after ourselves, this time I mean it ;-) */
		delete buf;
	}

	return true;
}

bool GetInfo_IRQ (Q3ListView *lbox)
{
#ifdef HAVE_DEVINFO_H
	/* systat lists the interrupts assigned to devices as well as how many were
	   generated.  Parsing its output however is about as fun as a sandpaper
	   enema.  The best idea would probably be to rip out the guts of systat.
	   Too bad it's not very well commented */
	/* Oh neat, current now has a neat little utility called devinfo */
	if (devinfo_init())
		return false;
	devinfo_foreach_rman(print_irq, lbox);
	return true;
#else
	return false;
#endif
}

bool GetInfo_DMA (Q3ListView *lbox)
{
#ifdef HAVE_DEVINFO_H
	/* Oh neat, current now has a neat little utility called devinfo */
	if (devinfo_init())
		return false;
	devinfo_foreach_rman(print_dma, lbox);
	return true;
#else
	return false;
#endif
}

bool GetInfo_IO_Ports (Q3ListView *lbox)
{
#ifdef HAVE_DEVINFO_H
	/* Oh neat, current now has a neat little utility called devinfo */
	if (devinfo_init())
		return false;
	devinfo_foreach_rman(print_ioports, lbox);
	return true;
#else
	return false;
#endif
}

bool GetInfo_Sound (Q3ListView *lbox)
{
	QFile *sndstat = new QFile("/dev/sndstat");
	QTextStream *t;
	QString s;
	Q3ListViewItem *olditem = 0;

	if (!sndstat->exists() || !sndstat->open(QIODevice::ReadOnly)) {

		s = i18n("Your sound system could not be queried.  /dev/sndstat does not exist or is not readable.");
		olditem = new Q3ListViewItem(lbox, olditem, s);
	} else {
		t = new QTextStream(sndstat);
		while (!(s=t->readLine()).isNull()) {
			olditem = new Q3ListViewItem(lbox, olditem, s);
		}

		delete t;
		sndstat->close();
	}

	delete sndstat;
	return true;
}

bool GetInfo_SCSI (Q3ListView *lbox)
{
	FILE *pipe;
	QFile *camcontrol = new QFile("/sbin/camcontrol");
	QTextStream *t;
	QString s;
	Q3ListViewItem *olditem = 0;

	if (!camcontrol->exists()) {
		s = i18n ("SCSI subsystem could not be queried: /sbin/camcontrol could not be found");
		olditem = new Q3ListViewItem(lbox, olditem, s);
	} else if ((pipe = popen("/sbin/camcontrol devlist 2>&1", "r")) == NULL) {
		s = i18n ("SCSI subsystem could not be queried: /sbin/camcontrol could not be executed");
		olditem = new Q3ListViewItem(lbox, olditem, s);
	} else {

		/* This prints out a list of all the scsi devies, perhaps eventually we could
		   parse it as opposed to schlepping it into a listbox */

		t = new QTextStream(pipe, QIODevice::ReadOnly);

		while (true) {
			s = t->readLine();
			if ( s.isEmpty() )
				break;
			olditem = new Q3ListViewItem(lbox, olditem, s);
		}

		delete t;
		pclose(pipe);
	}

	delete camcontrol;

	if (!lbox->childCount())
		return false;

	return true;
}

bool GetInfo_PCI (Q3ListView *lbox)
{
	FILE *pipe;
	QFile *pcicontrol;
	QString s, cmd;
	Q3ListViewItem *olditem = 0;

	pcicontrol = new QFile("/usr/sbin/pciconf");

	if (!pcicontrol->exists()) {
		delete pcicontrol;
		pcicontrol = new QFile("/usr/X11R6/bin/scanpci");
		if (!pcicontrol->exists()) {
			delete pcicontrol;
			pcicontrol = new QFile("/usr/X11R6/bin/pcitweak");
			if (!pcicontrol->exists()) {
				QString s;
				s = i18n("Could not find any programs with which to query your system's PCI information");
				(void) new Q3ListViewItem(lbox, 0, s);
				delete pcicontrol;
				return true;
			} else {
				cmd = "/usr/X11R6/bin/pcitweak -l 2>&1";
			}
		} else {
			cmd = "/usr/X11R6/bin/scanpci";
		}
	} else {
		cmd = "/usr/sbin/pciconf -l -v 2>&1";
	}
	delete pcicontrol;

	if ((pipe = popen(cmd.toLatin1(), "r")) == NULL) {
		s = i18n ("PCI subsystem could not be queried: %1 could not be executed", cmd);
		olditem = new Q3ListViewItem(lbox, olditem, s);
	} else {

		/* This prints out a list of all the pci devies, perhaps eventually we could
		   parse it as opposed to schlepping it into a listbox */

		pclose(pipe);
		GetInfo_ReadfromPipe(lbox, cmd.toLatin1(), true);
	}

	if (!lbox->childCount()) {
		s = i18n("The PCI subsystem could not be queried, this may need root privileges.");
		olditem = new Q3ListViewItem(lbox, olditem, s);
		return true;
	}

	return true;
}

bool GetInfo_Partitions (Q3ListView *lbox)
{
	struct fstab *fstab_ent;

	if (setfsent() != 1) /* Try to open fstab */ {
		int s_err = errno;
		QString s;
		s = i18n("Could not check filesystem info: ");
		s += strerror(s_err);
		(void)new Q3ListViewItem(lbox, 0, s);
	} else {
		lbox->addColumn(i18n("Device"));
		lbox->addColumn(i18n("Mount Point"));
		lbox->addColumn(i18n("FS Type"));
		lbox->addColumn(i18n("Mount Options"));

		while ((fstab_ent=getfsent())!=NULL) {
			new Q3ListViewItem(lbox, fstab_ent->fs_spec,
					  fstab_ent->fs_file, fstab_ent->fs_vfstype,
					  fstab_ent->fs_mntops);
		}

		lbox->setSorting(0);
		lbox->header()->setClickEnabled(true);

		endfsent(); /* Close fstab */
	}
	return true;
}

bool GetInfo_XServer_and_Video (Q3ListView *lBox)
{
	return GetInfo_XServer_Generic( lBox );
}

bool GetInfo_Devices (Q3ListView *lbox)
{
	QFile *f = new QFile("/var/run/dmesg.boot");
	if (f->open(QIODevice::ReadOnly)) {
		QTextStream qts(f);
		Q3Dict<Q3ListViewItem> lv_items;
		Device *dev;
		QString line, controller;
		lbox->setRootIsDecorated(true);
		lbox->addColumn("Device");
		lbox->addColumn("Description");
		while ( !(line=qts.readLine()).isNull() ) {
			controller = GetController(line);
			if (controller.isNull())
				continue;
			dev=GetDevice(line);
			if (!dev)
				continue;
			// Ewww assuing motherboard is the only toplevel controller is rather gross
			if (controller == "motherboard") {
				if (!lv_items[dev->name]) {
					lv_items.insert(dev->name, new Q3ListViewItem(lbox, dev->name, dev->description) );
				}
			} else {
				Q3ListViewItem *parent=lv_items[controller];
				if (parent && !lv_items[dev->name]) {
					lv_items.insert(dev->name, new Q3ListViewItem(parent, dev->name, dev->description) );
				}
			}
		}
		return true;
	}
	return false;
}

QString GetController(const QString &line)
{
		if ( ( (line.startsWith("ad")) || (line.startsWith("afd")) || (line.startsWith("acd")) ) && (line.find(":") < 6) ) {
			QString controller = line;
			controller.remove(0, controller.find(" at ")+4);
			if (controller.find("-slave") != -1) {
				controller.remove(controller.find("-slave"), controller.length());
			} else if (controller.find("-master") != -1) {
				controller.remove(controller.find("-master"), controller.length());
			} else
				controller=QString();
			if (!controller.isNull())
				return controller;
		}
		if (line.find(" on ") != -1) {
			QString controller;
			controller = line;
			controller.remove(0, controller.find(" on ")+4);
			if (controller.find(" ") != -1)
				controller.remove(controller.find(" "), controller.length());
			return controller;
		}
			return QString();
}

Device *GetDevice(const QString &line)
{
	Device *dev;
	int colon = line.find(":");
	if (colon == -1)
		return 0;
	dev = new Device;
	dev->name = line.mid(0, colon);
	dev->description = line.mid(line.find("<")+1, line.length());
	dev->description.remove(dev->description.find(">"), dev->description.length());
	return dev;
}

#ifdef HAVE_DEVINFO_H

int print_irq(struct devinfo_rman *rman, void *arg)
{
	Q3ListView *lbox = (Q3ListView *)arg;
        if (strcmp(rman->dm_desc, "Interrupt request lines")==0) {
		(void)new Q3ListViewItem(lbox, 0, rman->dm_desc);
		devinfo_foreach_rman_resource(rman, print_resource, arg);
        }
        return(0);
}

int print_dma(struct devinfo_rman *rman, void *arg)
{
	Q3ListView *lbox = (Q3ListView *)arg;
        if (strcmp(rman->dm_desc, "DMA request lines")==0) {
		(void)new Q3ListViewItem(lbox, lbox->lastItem(), rman->dm_desc);
		devinfo_foreach_rman_resource(rman, print_resource, arg);
        }
        return(0);
}

int print_ioports(struct devinfo_rman *rman, void *arg)
{
	Q3ListView *lbox = (Q3ListView *)arg;

	if (strcmp(rman->dm_desc, "I/O ports")==0) {
		(void)new Q3ListViewItem(lbox, lbox->lastItem(), rman->dm_desc);
		devinfo_foreach_rman_resource(rman, print_resource, arg);
        }
	else if (strcmp(rman->dm_desc, "I/O memory addresses")==0) {
		(void)new Q3ListViewItem(lbox, lbox->lastItem(), rman->dm_desc);
		devinfo_foreach_rman_resource(rman, print_resource, arg);
	}
        return(0);
}

int print_resource(struct devinfo_res *res, void *arg)
{
        struct devinfo_dev      *dev;
        struct devinfo_rman     *rman;
        int                     hexmode;

	Q3ListView *lbox;

	lbox = (Q3ListView *)arg;

	QString s, tmp;

        rman = devinfo_handle_to_rman(res->dr_rman);
        hexmode =  (rman->dm_size > 100) || (rman->dm_size == 0);
        tmp.sprintf(hexmode ? "0x%lx" : "%lu", res->dr_start);
	s += tmp;
        if (res->dr_size > 1) {
                tmp.sprintf(hexmode ? "-0x%lx" : "-%lu",
                    res->dr_start + res->dr_size - 1);
		s += tmp;
	}

        dev = devinfo_handle_to_device(res->dr_device);
        if ((dev != NULL) && (dev->dd_name[0] != 0)) {
                tmp.sprintf(" (%s)", dev->dd_name);
        } else {
                tmp.sprintf(" ----");
        }
	s += tmp;

	(void)new Q3ListViewItem(lbox, lbox->lastItem(), s);
        return(0);
}

#endif
