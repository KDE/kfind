/*
    kfinddlg.cpp

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#include "kfinddlg.h"

#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QStatusBar>

#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KGuiItem>
#include <KHelpMenu>
#include <KLocalizedString>
#include <KMessageBox>

#include "kftabdlg.h"
#include "kquery.h"
#include "kfindtreeview.h"

KfindDlg::KfindDlg(const QUrl &url, QWidget *parent)
    : QDialog(parent)
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    setModal(true);
    QWidget::setWindowTitle(i18nc("@title:window", "Find Files/Folders"));

    isResultReported = false;

    QFrame *frame = new QFrame;
    mainLayout->addWidget(frame);

    // create tabwidget
    tabWidget = new KfindTabWidget(frame);
    mainLayout->addWidget(tabWidget);
    tabWidget->setURL(url);

    // prepare window for find results
    win = new KFindTreeView(frame, this);

    mStatusBar = new QStatusBar(frame);
    
    m_labelStatus = new QLabel(mStatusBar);
    m_labelStatus->setAlignment (Qt::AlignLeft | Qt::AlignVCenter);
    m_labelStatus->setText(i18nc("the application is currently idle, there is no active search", "Idle."));

    m_labelProgress = new QLabel(mStatusBar);
    m_labelProgress->setAlignment (Qt::AlignRight | Qt::AlignVCenter);
    m_labelProgress->setText(QString());
    
    mStatusBar->addWidget(m_labelStatus, 1);
    mStatusBar->addWidget(m_labelProgress, 0);
    
    QVBoxLayout *vBox = new QVBoxLayout(frame);
    vBox->addWidget(tabWidget, 0);
    vBox->addWidget(win, 1);
    vBox->addWidget(mStatusBar, 0);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Help | QDialogButtonBox::Close, this);
    m_findButton = new QPushButton;
    buttonBox->addButton(m_findButton, QDialogButtonBox::ActionRole);
    m_stopButton = new QPushButton;
    buttonBox->addButton(m_stopButton, QDialogButtonBox::ActionRole);
    m_saveAsButton = new QPushButton;
    buttonBox->addButton(m_saveAsButton, QDialogButtonBox::ActionRole);
    buttonBox->setOrientation(Qt::Vertical);
    m_findButton->setDefault(true);

    KGuiItem::assign(m_findButton, KStandardGuiItem::find());
    KGuiItem::assign(m_stopButton, KStandardGuiItem::stop());
    KGuiItem::assign(m_saveAsButton, KStandardGuiItem::saveAs());

    m_findButton->setEnabled(true); // Enable "Find"
    m_stopButton->setEnabled(false); // Disable "Stop"
    m_saveAsButton->setEnabled(false); // Disable "Save As..."

    mainLayout->addWidget(buttonBox);

    connect(tabWidget, &KfindTabWidget::startSearch, this, &KfindDlg::startSearch);
    connect(m_findButton, &QPushButton::clicked, this, &KfindDlg::startSearch);
    connect(m_stopButton, &QPushButton::clicked, this, &KfindDlg::stopSearch);
    connect(m_saveAsButton, &QPushButton::clicked, win, &KFindTreeView::saveResults);

    connect(buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &KfindDlg::finishAndClose);

    connect(win, &KFindTreeView::resultSelected, this, &KfindDlg::resultSelected);

    query = new KQuery(frame);
    connect(query, &KQuery::result, this, &KfindDlg::slotResult);
    connect(query, &KQuery::foundFileList, this, &KfindDlg::addFiles);

    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), true);
    buttonBox->button(QDialogButtonBox::Help)->setMenu(helpMenu->menu());
    dirwatch = nullptr;
    
    setFocus();
}

KfindDlg::~KfindDlg()
{
    stopSearch();

    delete dirwatch;
}

void KfindDlg::finishAndClose()
{
    //Stop the current search and closes the dialog
    stopSearch();
    close();
}

void KfindDlg::setProgressMsg(const QString &msg)
{
    m_labelProgress->setText(msg);
}

void KfindDlg::setStatusMsg(const QString &msg)
{
    m_labelStatus->setText(msg);
}

void KfindDlg::startSearch()
{
    tabWidget->setQuery(query);

    isResultReported = false;

    // Reset count - use the same i18n as below
    setProgressMsg(i18n("0 items found"));

    Q_EMIT resultSelected(false);
    Q_EMIT haveResults(false);

    m_findButton->setEnabled(false); // Disable "Find"
    m_stopButton->setEnabled(true); // Enable "Stop"
    m_saveAsButton->setEnabled(false); // Disable "Save As..."

    delete dirwatch;
    dirwatch = new KDirWatch();
    connect(dirwatch, &KDirWatch::created, this, &KfindDlg::slotNewItems);
    connect(dirwatch, &KDirWatch::deleted, this, &KfindDlg::slotDeleteItem);
    dirwatch->addDir(query->url().toLocalFile(), KDirWatch::WatchFiles);

#if 0
    // waba: Watching for updates is disabled for now because even with FAM it causes too
    // much problems. See BR68220, BR77854, BR77846, BR79512 and BR85802
    // There are 3 problems:
    // 1) addDir() keeps looping on recursive symlinks
    // 2) addDir() scans all subdirectories, so it basically does the same as the process that
    // is started by KQuery but in-process, undoing the advantages of using a separate find process
    // A solution could be to let KQuery Q_EMIT all the directories it has searched in.
    // Either way, putting dirwatchers on a whole file system is probably just too much.
    // 3) FAM has a tendency to deadlock with so many files (See BR77854) This has hopefully
    // been fixed in KDirWatch, but that has not yet been confirmed.

    //Getting a list of all subdirs
    if (tabWidget->isSearchRecursive() && (dirwatch->internalMethod() == KDirWatch::FAM)) {
        const QStringList subdirs = getAllSubdirs(query->url().path());
        for (QStringList::const_iterator it = subdirs.constBegin(); it != subdirs.constEnd(); ++it) {
            dirwatch->addDir(*it, true);
        }
    }
#endif

    win->beginSearch(query->url());
    tabWidget->beginSearch();

    setStatusMsg(i18n("Searching..."));
    query->start();
}

void KfindDlg::stopSearch()
{
    query->kill();
}

void KfindDlg::newSearch()
{
    // WABA: Not used any longer?
    stopSearch();

    tabWidget->setDefaults();

    Q_EMIT haveResults(false);
    Q_EMIT resultSelected(false);

    setFocus();
}

void KfindDlg::slotResult(int errorCode)
{
    if (errorCode == 0) {
        setStatusMsg(i18nc("the application is currently idle, there is no active search", "Idle."));
    } else if (errorCode == KIO::ERR_USER_CANCELED) {
        setStatusMsg(i18n("Canceled."));
    } else if (errorCode == KIO::ERR_MALFORMED_URL) {
        setStatusMsg(i18n("Error."));
        KMessageBox::sorry(this, i18n("Please specify an absolute path in the \"Look in\" box."));
    } else if (errorCode == KIO::ERR_DOES_NOT_EXIST) {
        setStatusMsg(i18n("Error."));
        KMessageBox::sorry(this, i18n("Could not find the specified folder."));
    } else {
        setStatusMsg(i18n("Error."));
    }

    m_findButton->setEnabled(true); // Enable "Find"
    m_stopButton->setEnabled(false); // Disable "Stop"
    m_saveAsButton->setEnabled(true); // Enable "Save As..."

    win->endSearch();
    tabWidget->endSearch();
    setFocus();
}

void KfindDlg::addFiles(const QList< QPair<KFileItem, QString> > &pairs)
{
    win->insertItems(pairs);

    if (!isResultReported) {
        Q_EMIT haveResults(true);
        isResultReported = true;
    }

    const QString str = i18np("one item found", "%1 items found", win->itemCount());
    setProgressMsg(str);
}

void KfindDlg::setFocus()
{
    tabWidget->setFocus();
}

void KfindDlg::copySelection()
{
    win->copySelection();
}

void KfindDlg::about()
{
    KAboutApplicationDialog dlg(KAboutData::applicationData(), KAboutApplicationDialog::NoOptions, this);
    dlg.exec();
}

void KfindDlg::slotDeleteItem(const QString &file)
{
    win->removeItem(QUrl::fromLocalFile(file));

    const QString str = i18np("one item found", "%1 items found", win->itemCount());
    setProgressMsg(str);
}

void KfindDlg::slotNewItems(const QString &file)
{
    const QUrl url = QUrl::fromLocalFile(file);

    if (query->url().isParentOf(url)) {
        if (!win->isInserted(url)) {
            query->slotListEntries(QStringList() << file);
        }
    }
}

QStringList KfindDlg::getAllSubdirs(QDir d)
{
    QStringList dirs;
    QStringList subdirs;

    d.setFilter(QDir::Dirs);
    dirs = d.entryList();

    const QStringList::const_iterator end(dirs.constEnd());
    for (QStringList::const_iterator it = dirs.constBegin(); it != end; ++it) {
        if ((*it == QLatin1String(".")) || (*it == QLatin1String(".."))) {
            continue;
        }
        subdirs.append(d.path()+QLatin1Char('/')+*it);
        subdirs += getAllSubdirs(QString(d.path()+QLatin1Char('/')+*it));
    }
    return subdirs;
}
