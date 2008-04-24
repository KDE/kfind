/*
 * kcmioslaveinfo.cpp
 *
 * Copyright 2001 Alexander Neundorf <neundorf@kde.org>
 * Copyright 2001 George Staikos  <staikos@kde.org>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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
 */

#include <QFile>
#include <QLabel>
#include <QLayout>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTextCodec>
#include <QWhatsThis>
#include <QComboBox>

#include <kconfig.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kglobal.h>
#include <kiconloader.h>
#include <kio/job.h>
#include <klocale.h>
#include <kprotocolinfo.h>
#include <kstandarddirs.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>

#include "kcmioslaveinfo.h"

K_PLUGIN_FACTORY(SlaveFactory, registerPlugin<KCMIOSlaveInfo>();)
K_EXPORT_PLUGIN( SlaveFactory("kcmioslaveinfo"))

KCMIOSlaveInfo::KCMIOSlaveInfo(QWidget *parent, const QVariantList &) :
	KCModule(SlaveFactory::componentData(), parent), m_tfj(NULL) {
	QVBoxLayout *layout=new QVBoxLayout(this);
	layout->setMargin(0);

	setQuickHelp(i18n("Overview of the installed ioslaves and supported protocols."));
	setButtons(KCModule::Help);
	
	QHBoxLayout* selectionLayout = new QHBoxLayout();
	
	ioSlaves = new QComboBox(this);
	QLabel* ioSlavesLabel = new QLabel(i18n("Select the protocol documentation to display:"));
	ioSlavesLabel->setBuddy(ioSlaves);
	
	connect(ioSlaves, SIGNAL(currentIndexChanged(const QString&) ), SLOT( showInfo(const QString&) ));

	selectionLayout->addWidget(ioSlavesLabel);
	selectionLayout->addWidget(ioSlaves);
	
	layout->addLayout(selectionLayout);
	

	//TODO make something useful after 2.1 is released
	m_info=new KTextBrowser(this);
	
	layout->addWidget(m_info);

	QStringList protocols=KProtocolInfo::protocols();
	protocols.sort();
	foreach(const QString &proto, protocols) {
		//m_ioslavesLb->addItem(new QListWidgetItem ( SmallIcon( KProtocolInfo::icon( proto )), proto, m_ioslavesLb));
		ioSlaves->addItem(SmallIcon( KProtocolInfo::icon( proto )), proto);
	};
	//m_ioslavesLb->sort();
	//m_ioslavesLb->setSelected(0, true);

	KAboutData *about = new KAboutData(I18N_NOOP("kcmioslaveinfo"), 0,
			ki18n("KDE Panel System Information Control Module"),
			0, KLocalizedString(), KAboutData::License_GPL,
			ki18n("(c) 2001 - 2002 Alexander Neundorf"));

	about->addAuthor(ki18n("Alexander Neundorf"), KLocalizedString(), "neundorf@kde.org");
	about->addAuthor(ki18n("George Staikos"), KLocalizedString(), "staikos@kde.org");
	setAboutData(about);

}

void KCMIOSlaveInfo::slaveHelp(KIO::Job *, const QByteArray &data) {
	if (data.size() == 0) { // EOF
		QString text = selectHelpBody();
		m_info->setHtml(text);
		return;
	}
	helpData += data;
}

/**
 * Big Hack to only select content of the help documentation
 * The HTML content is cut by recognizing header and footer
 */
QString KCMIOSlaveInfo::selectHelpBody() {
	int index = helpData.indexOf("<meta http-equiv=\"Content-Type\"");
	index = helpData.indexOf("charset=", index) + 8;
	QString charset = helpData.mid(index, helpData.indexOf( '\"', index) - index);
	QString text = QTextCodec::codecForName(charset.toLatin1())->toUnicode(helpData);
	index = text.indexOf("<div class=\"titlepage\">"); //Start documentation "identifier"
	text = text.mid(index);
	index = text.indexOf("<div style=\"background-color: #white; color: black;                  margin-top: 20px; margin-left: 20px;                  margin-right: 20px;\"><div style=\"position: absolute; left: 20px;\">");  //End documentation "identifier"
	text = text.left(index);

	return text;
}

void KCMIOSlaveInfo::slotResult(KJob *) {
	m_tfj = NULL;
}

void KCMIOSlaveInfo::showInfo(const QString& protocol) {
	QString file = QString("kioslave/%1.docbook").arg(protocol);
	file = KGlobal::locale()->langLookup(file);
	if (m_tfj) {
		m_tfj->kill();
		m_tfj = NULL;
	}

	if (!file.isEmpty()) {
		helpData.clear();
		m_tfj = KIO::get(KUrl(QString("help:/kioslave/%1.html").arg(protocol) ), KIO::Reload, KIO::HideProgressInfo);
		connect(m_tfj, SIGNAL( data( KIO::Job *, const QByteArray &) ), SLOT( slaveHelp( KIO::Job *, const QByteArray &) ));
		connect(m_tfj, SIGNAL( result( KJob * ) ), SLOT( slotResult( KJob * ) ));
		return;
	}

	m_info->setHtml(i18n("<html><body><p style='text-align:center'>Loading documentation of the '%1:/' protocol...</p></body></html>", protocol));
}


#include "kcmioslaveinfo.moc"

