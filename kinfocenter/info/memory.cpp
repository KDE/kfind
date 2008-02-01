/*
 *  memory.cpp
 *
 *  prints memory-information and shows a graphical display.
 *
 *  Copyright (c) 1999-2002 Helge Deller <deller@gmx.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 */

#include <sys/param.h>		/* for BSD */

#include <QLayout>
#include <QPainter>
#include <QtGui/qdrawutil.h>

//Added by qt3to4:
#include <QPixmap>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <klocale.h>
#include <kglobal.h>
#include <kdialog.h>
#include <kseparator.h>
#include <KPluginFactory>

#include "memory.h"

enum {				/* entries for Memory_Info[] */
    TOTAL_MEM = 0,		/* total physical memory (without swaps) */
    FREE_MEM,			/* total free physical memory (without swaps) */
#if !defined(__svr4__) || !defined(sun)
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
    SHARED_MEM,			/* shared memory size */
    BUFFER_MEM,			/* buffered memory size */
#else
    ACTIVE_MEM,
    INACTIVE_MEM,
#endif
#endif
    CACHED_MEM,			/* cache memory size (located in ram) */
    SWAP_MEM,			/* total size of all swap-partitions */
    FREESWAP_MEM,		/* free memory in swap-partitions */
    MEM_LAST_ENTRY
};

/*
   all fetchValues()-functions should put either
   their results _OR_ the value NO_MEMORY_INFO into Memory_Info[]
*/
static t_memsize Memory_Info[MEM_LAST_ENTRY];

#define MEMORY(x)	((t_memsize) (x))	/* it's easier... */
#define NO_MEMORY_INFO	MEMORY(-1)		/* DO NOT CHANGE */
#define ZERO_IF_NO_INFO(value) ((value) != NO_MEMORY_INFO ? (value) : 0)


/******************/
/* Implementation */
/******************/

static QLabel *MemSizeLabel[MEM_LAST_ENTRY][2];

enum { MEM_RAM_AND_HDD, MEM_RAM, MEM_HDD, MEM_LAST };
static QLabel *Graph[MEM_LAST];
static QLabel *GraphLabel[MEM_LAST];

#define SPACING 16

static QString formatted_unit(t_memsize value)
{
    if (value > (1024 * 1024))
        if (value > (1024 * 1024 * 1024))
            return i18n("%1 GiB", KGlobal::locale()->formatNumber(value / (1024 * 1024 * 1024.0), 2));
        else
            return i18n("%1 MiB", KGlobal::locale()->formatNumber(value / (1024 * 1024.0), 2));
    else
        return i18n("%1 KiB", KGlobal::locale()->formatNumber(value / 1024.0, 2));
}

K_PLUGIN_FACTORY_DECLARATION(KInfoModulesFactory)

KMemoryWidget::KMemoryWidget(QWidget *parent, const QVariantList &)
:  KCModule(KInfoModulesFactory::componentData(),parent)
{

    KAboutData *about =
    new KAboutData(I18N_NOOP("kcminfo"), 0,
		ki18n("KDE Panel Memory Information Control Module"),
		0, KLocalizedString(), KAboutData::License_GPL,
		ki18n("(c) 1998 - 2002 Helge Deller"));

    about->addAuthor(ki18n("Helge Deller"), KLocalizedString(), "deller@gmx.de");
    setAboutData( about );

    QString title, initial_str;
    QLabel *Widget = 0;
    int i, j;

    ram_colors_initialized =
    swap_colors_initialized =
    all_colors_initialized = false;

    setButtons(Help);

    /* default string for no Information... */
    Not_Available_Text = i18n("Not available.");

    QVBoxLayout *top = new QVBoxLayout(this);
    top->setMargin(0);
    top->setSpacing(KDialog::spacingHint());

    QHBoxLayout *hbox = new QHBoxLayout();
    top->addLayout(hbox);

    /* stretch the left side */
    hbox->addStretch();

    /* first create the Informationtext-Widget */
    QVBoxLayout *vbox = new QVBoxLayout();
    hbox->addItem( vbox );
    vbox->setSpacing(0);
    for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; ++i) {
	switch (i) {
	case TOTAL_MEM:
	    title = i18n("Total physical memory:");
	    break;
	case FREE_MEM:
	    title = i18n("Free physical memory:");
	    break;
#if !defined(__svr4__) || !defined(sun)
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
	case SHARED_MEM:
	    title = i18n("Shared memory:");
	    break;
	case BUFFER_MEM:
	    title = i18n("Disk buffers:");
	    break;
#else
	case ACTIVE_MEM:
	    title = i18n("Active memory:");
	    break;
	case INACTIVE_MEM:
	    title = i18n("Inactive memory:");
	    break;
#endif
#endif
	case CACHED_MEM:
	    title = i18n("Disk cache:");
	    break;
	case SWAP_MEM:
	    vbox->addSpacing(SPACING);
	    title = i18n("Total swap memory:");
	    break;
	case FREESWAP_MEM:
	    title = i18n("Free swap memory:");
	    break;
	default:
	    title = "";
	    break;
	};
	Widget = new QLabel(title, this);
	Widget->setAlignment( Qt::AlignLeft );
	vbox->addWidget(Widget, 1);
    }

    /* then the memory-content-widgets */
    for (j = 0; j < 2; j++) {
	vbox = new QVBoxLayout();
        hbox->addItem( vbox );
	vbox->setSpacing(0);
	for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; ++i) {
	    if (i == SWAP_MEM)
		vbox->addSpacing(SPACING);
	    Widget = new QLabel(this);
	    Widget->setAlignment( Qt::AlignRight );
	    MemSizeLabel[i][j] = Widget;
	    vbox->addWidget(Widget, 1);
	}
    }

    /* stretch the right side */
    hbox->addStretch();

    KSeparator *line = new KSeparator(Qt::Horizontal, this);
    top->addWidget(line);

    /* now the Graphics */
    QString hint;
    hbox = new QHBoxLayout();
    top->addItem(hbox);
    hbox->setSpacing(1);
    for (i = MEM_RAM_AND_HDD; i < MEM_LAST; i++) {
	hbox->addSpacing(SPACING);
	vbox = new QVBoxLayout();
        hbox->addItem( vbox );

	switch (i) {
	case MEM_RAM_AND_HDD:
	    title = i18n("Total Memory");
	    hint = i18n("This graph gives you an overview of the "
			"<b>total sum of physical and virtual memory</b> "
			"in your system.");
	    break;
	case MEM_RAM:
	    title = i18n("Physical Memory");
	    hint = i18n("This graph gives you an overview of "
			"the <b>usage of physical memory</b> in your system."
			"<p>Most operating systems (including Linux) "
			"will use as much of the available physical "
			"memory as possible as disk cache, "
			"to speed up the system performance.</p>"
			"<p>This means that if you have a small amount "
			"of <b>Free Physical Memory</b> and a large amount of "
			"<b>Disk Cache Memory</b>, your system is well "
			"configured.</p>");
	    break;
	case MEM_HDD:
	    title = i18n("Swap Space");
	    hint = i18n("The swap space is the <b>virtual memory</b> "
			"available to the system. "
			"<p>It will be used on demand and is provided "
			"through one or more swap partitions and/or swap files.</p>");
	    break;
	default:
	    hint = title = QString();
	    break;
	};

	if (hint.length())
	  hint = "<qt>" + hint + "</qt>";

	Widget = new QLabel("<b>" + title + "</b>", this);
	Widget->setAlignment( Qt::AlignCenter );
	Widget->setToolTip( hint);
	vbox->addWidget(Widget);
	vbox->addSpacing(SPACING / 2);

        QLabel *g = new QLabel(this);
	g->setMinimumWidth(2 * SPACING);
	g->setMinimumHeight(3 * SPACING);
	g->setAttribute(Qt::WA_NoSystemBackground, true);
	g->setToolTip( hint); // add the tooltip
	Graph[i] = g;
	vbox->addWidget(g, 2);
	vbox->addSpacing(SPACING / 2);

	Widget = new QLabel(this);	/* xx MB used. */
	Widget->setAlignment( Qt::AlignCenter );
	Widget->setToolTip( hint);
	GraphLabel[i] = Widget;
	vbox->addWidget(Widget);
    }
    hbox->addSpacing(SPACING);

    timer = new QTimer(this);
    timer->start(100);
    QObject::connect(timer, SIGNAL(timeout()), this,
		     SLOT(update()));

    fetchValues();
}

KMemoryWidget::~KMemoryWidget()
{
    /* stop the timer */
    timer->stop();
}


QString KMemoryWidget::quickHelp() const
{
  return i18n("<h1>Memory Information</h1>"
	" This display shows you the current memory usage of your system."
	" The values are updated on a regular basis and give you an"
	" overview of the physical and virtual used memory." );
}


/* Graphical Memory Display */
bool KMemoryWidget::Display_Graph(int widgetindex,
				int count,
			      	t_memsize total,
			      	t_memsize * used,
			      	QColor * color,
				QString *text)
{
    QLabel *graph = Graph[widgetindex];
    int width = graph->width();
    int height = graph->height();
    QPixmap pm(width, height);
    QPainter paint;

    paint.begin(&pm, this);

    QPen pen(QColor(0, 0, 0));

    if (! ZERO_IF_NO_INFO(total)) {
	paint.fillRect(1, 1, width - 2, height - 2,
		       QBrush(QColor(128, 128, 128)));
	paint.setPen(pen);
	paint.drawRect(graph->rect());
	GraphLabel[widgetindex]->setText(Not_Available_Text);
	paint.end();
	bitBlt(graph, 0, 0, &pm);
	return false;
    }

    int startline = height-2;
    int	percent, localheight;
    t_memsize last_used = 0;

    while (count--) {
	last_used = *used;

    	percent = (((qint64)last_used) * 100) / total;

    	if (count)
		localheight = ((height-2) * percent) / 100;
	else
		localheight = startline;

	if (localheight>0) {
		paint.fillRect(1, startline, width-2, -localheight, *color);

    		if (localheight >= SPACING) {
			paint.drawText(0, startline-localheight, width, localheight,
				Qt::AlignCenter | Qt::TextWordWrap,
				QString("%1 %2%").arg(*text).arg(percent));
		}
    	}

	startline -= localheight;

	++used;
	++color;
	++text;
    }

    /* draw surrounding box */
    paint.setPen(pen);
    QRect r = graph->rect();
    qDrawShadePanel(&paint, r.x(), r.y(), r.width(), r.height(), palette().active(), true, 1);
    paint.end();
    graph->setPixmap(pm);
    //bitBlt(graph, 0, 0, &pm);

    GraphLabel[widgetindex]->setText(i18n("%1 free", formatted_unit(last_used)));

    return true;
}

/* update_Values() is the main-loop for updating the Memory-Information */
void KMemoryWidget::paintEvent(QPaintEvent *)
{
    int i;
    bool ok1;
    QLabel *label;
    t_memsize used[5];

    fetchValues();			/* get the Information from memory_linux, memory_fbsd */

    /* update the byte-strings */
    for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; i++) {
	label = MemSizeLabel[i][0];
	if (Memory_Info[i] == NO_MEMORY_INFO)
	    label->clear();
	else
	    label->setText(i18n("%1 bytes =",
			   KGlobal::locale()->
			       formatNumber(Memory_Info[i], 0)));
    }

    /* update the MB-strings */
    for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; i++) {
	label = MemSizeLabel[i][1];
	label->setText((Memory_Info[i] != NO_MEMORY_INFO)
		       ? formatted_unit(Memory_Info[i])
		       : Not_Available_Text);
    }

    /* display graphical output (ram, hdd, at last: HDD+RAM) */
    /* be careful ! Maybe we have not all info available ! */

    /* RAM usage: */
    /* don't rely on the SHARED_MEM value since it may refer to
     * the size of the System V sharedmem in 2.4.x. Calculate instead! */

    used[1] = 0;
#if !defined(__svr4__) || !defined(sun)
#if !defined(__NetBSD__) && !defined(__OpenBSD__)
    used[1] = ZERO_IF_NO_INFO(Memory_Info[BUFFER_MEM]);
#endif
#endif
    used[2] = ZERO_IF_NO_INFO(Memory_Info[CACHED_MEM]);
    used[3] = ZERO_IF_NO_INFO(Memory_Info[FREE_MEM]);
    used[0] = ZERO_IF_NO_INFO(Memory_Info[TOTAL_MEM]) - used[1] - used[2] - used[3];
    if (!ram_colors_initialized) {
		ram_colors_initialized = true;
		ram_text[0] = i18n("Application Data");
		ram_colors[0] = COLOR_USED_MEMORY; /* used+shared */
		ram_text[1] = i18n("Disk Buffers");
		ram_colors[1] = QColor(24,131,5); /* buffer */
		ram_text[2] = i18n("Disk Cache");
		ram_colors[2] = QColor(33,180,7); /* cached */
		ram_text[3] = i18n("Free Physical Memory");
		ram_colors[3] = COLOR_FREE_MEMORY; /* free */
    }
    ok1 = Display_Graph(MEM_RAM, 4, Memory_Info[TOTAL_MEM],
		      used, ram_colors, ram_text);

    /* SWAP usage: */
    used[1] = ZERO_IF_NO_INFO(Memory_Info[FREESWAP_MEM]);
    used[0] = ZERO_IF_NO_INFO(Memory_Info[SWAP_MEM]) - used[1];
    if (!swap_colors_initialized) {
		swap_colors_initialized = true;
		swap_text[0] = i18n("Used Swap");
		swap_colors[0] = COLOR_USED_SWAP; /* used */
		swap_text[1] = i18n("Free Swap");
		swap_colors[1] = COLOR_FREE_MEMORY; /* free */
    }
    Display_Graph(MEM_HDD, 2, Memory_Info[SWAP_MEM],
		      used, swap_colors, swap_text);

    /* RAM + SWAP usage: */
    if (Memory_Info[SWAP_MEM] == NO_MEMORY_INFO ||
	Memory_Info[FREESWAP_MEM] == NO_MEMORY_INFO)
	    Memory_Info[SWAP_MEM] = Memory_Info[FREESWAP_MEM] = 0;

    used[1] = Memory_Info[SWAP_MEM] - Memory_Info[FREESWAP_MEM];
    used[2] = Memory_Info[FREE_MEM] + Memory_Info[FREESWAP_MEM];
    used[0] = (Memory_Info[TOTAL_MEM]+Memory_Info[SWAP_MEM])-used[1]-used[2];
    if (!all_colors_initialized) {
		all_colors_initialized = true;
		all_text[0] = i18n("Used Physical Memory");
		all_colors[0] = COLOR_USED_MEMORY; /* used ram */
		all_text[1] = i18n("Used Swap");
		all_colors[1] = COLOR_USED_SWAP; /* used swap */
		all_text[2] = i18n("Total Free Memory");
		all_colors[2] = COLOR_FREE_MEMORY; /* free ram+swap*/
    }
    Display_Graph(MEM_RAM_AND_HDD, 3,
		  ok1	? Memory_Info[TOTAL_MEM] + Memory_Info[SWAP_MEM]
		  	: NO_MEMORY_INFO,
		  used, all_colors, all_text);
}


/* Include system-specific code */

#ifdef __linux__
#include "memory_linux.cpp"
#elif defined(__APPLE__)
#include "memory_osx.cpp"
#elif defined(sgi) && sgi
#include "memory_sgi.cpp"
#elif defined(__svr4__) && defined(sun)
#include "memory_solaris.cpp"
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#include "memory_fbsd.cpp"
#elif defined(__hpux)
#include "memory_hpux.cpp"
#elif defined(__NetBSD__) || defined(__OpenBSD__)
#include "memory_netbsd.cpp"
#elif __osf__
#include "memory_tru64.cpp"
#else

/* Default for unsupported systems */
void KMemoryWidget::fetchValues()
{
    int i;
    for (i = TOTAL_MEM; i < MEM_LAST_ENTRY; ++i)
	Memory_Info[i] = NO_MEMORY_INFO;
}

#endif
#include "memory.moc"
