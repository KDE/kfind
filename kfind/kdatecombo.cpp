/*******************************************************************
* kdatecombo.cpp
* 
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
******************************************************************/

#include "kdatecombo.h"

#include <QTimer>
#include <QApplication>
#include <QDesktopWidget>
//Added by qt3to4:
#include <QKeyEvent>
#include <QEvent>

#include <kglobal.h>
#include <klocale.h>
#include <kdatepicker.h>
#include <QDebug>

KDateCombo::KDateCombo(QWidget *parent) : KComboBox(parent)
{
  setEditable( false );

  QDate date = QDate::currentDate();
  initObject(date);
}

KDateCombo::KDateCombo(const QDate & date, QWidget *parent) : KComboBox(parent)
{
  setEditable( false );

  initObject(date);
}

void KDateCombo::initObject(const QDate & date)
{
  setValidator(0);
  popupFrame = new QFrame(this, Qt::Popup);
  popupFrame->setFrameStyle(QFrame::Box | QFrame::Raised);
  popupFrame->setMidLineWidth(2);
  popupFrame->installEventFilter(this);
  datePicker = new KDatePicker(date, popupFrame);
  datePicker->setMinimumSize(datePicker->sizeHint());
  datePicker->installEventFilter(this);
  popupFrame->resize(datePicker->width() + 2 * popupFrame->frameWidth(),
                     datePicker->height() + 2 * popupFrame->frameWidth());
  setDate(date);

  connect(datePicker, SIGNAL(dateSelected(QDate)), this, SLOT(dateEnteredEvent(QDate)));
  connect(datePicker, SIGNAL(dateEntered(QDate)), this, SLOT(dateEnteredEvent(QDate)));
}

KDateCombo::~KDateCombo()
{
  delete datePicker;
  delete popupFrame;
}

QString KDateCombo::date2String(const QDate & date)
{
  return(KGlobal::locale()->formatDate(date, KLocale::ShortDate));
}

QDate & KDateCombo::string2Date(const QString & str, QDate *qd)
{
  return *qd = KGlobal::locale()->readDate(str);
}

QDate & KDateCombo::getDate(QDate *currentDate)
{
  return string2Date(currentText(), currentDate);
}

bool KDateCombo::setDate(const QDate & newDate)
{
  if (newDate.isValid())
  {
    if (count())
      clear();
    addItem(date2String(newDate));
    return true;
  }
  return false;
}

void KDateCombo::dateEnteredEvent(const QDate &newDate)
{
  QDate tempDate = newDate;
  if (!tempDate.isValid())
     tempDate = datePicker->date();
  popupFrame->hide();
  setDate(tempDate);
}

void KDateCombo::mousePressEvent (QMouseEvent * e)
{
  if (e->button() & Qt::LeftButton)
  {
    if  (rect().contains( e->pos()))
    {
      QDate tempDate;
      getDate(& tempDate);
      datePicker->setDate(tempDate);

        // Make sure the whole popup is visible.
        const QPoint pos = mapToGlobal(QPoint(0, height()));
        QRect desktopGeometry = QApplication::desktop()->screenGeometry(pos);

        int x = pos.x();
        int y = pos.y();
        int w = popupFrame->width();
        int h = popupFrame->height();
        if ( x + w > desktopGeometry.x() + desktopGeometry.width() ) {
            x = desktopGeometry.width() - w;
        }
        if ( y + h > desktopGeometry.y() + desktopGeometry.height() ) {
            y = desktopGeometry.height() - h;
        }
        if ( x < desktopGeometry.x() ) {
            x = 0;
        }
        if ( y < desktopGeometry.y() ) {
            y = 0;
        }

        // Pop the thingy up.
        popupFrame->move( x, y );
        popupFrame->show();
        datePicker->setFocus();
    }
  }
}

bool KDateCombo::eventFilter (QObject*, QEvent* e)
{
  if ( e->type() == QEvent::MouseButtonPress )
  {
      QMouseEvent *me = (QMouseEvent *)e;
      QPoint p = mapFromGlobal( me->globalPos() );
      if (rect().contains( p ) )
      {
        QTimer::singleShot(10, this, SLOT(dateEnteredEvent()));
        return true;
      }
  }
  else if ( e->type() == QEvent::KeyRelease )
  {
      QKeyEvent *k = (QKeyEvent *)e;

      if (k->key()==Qt::Key_Escape) {
        popupFrame->hide();
        return true;
      }
      else {
        return false;
      }
  }

  return false;
}
