/***********************************************************************
 *
 *  kdatecombo.h
 *
 ***********************************************************************/

#ifndef KDATECOMBO_H
#define KDATECOMBO_H

#include <qwidget.h>
#include <qcombobox.h>
#include <qdatetime.h>

/**
  *@author Beppe Grimaldi
  */

class KDatePicker;
class KPopupFrame;

class KDateCombo : public QComboBox  {
   Q_OBJECT

public:
	KDateCombo(QWidget *parent=0, const char *name=0);
	KDateCombo(const QDate & date, QWidget *parent=0, const char *name=0);
	~KDateCombo();

	QDate & getDate(QDate *currentDate);
	bool setDate(const QDate & newDate);

private:
   KPopupFrame * popupFrame;
   KDatePicker * datePicker;

   void initObject(const QDate & date, QWidget *parent, const char *name);

   QString date2String(const QDate &);
   QDate & string2Date(const QString &, QDate * );

protected:
  bool eventFilter (QObject*, QEvent*);
  virtual void mousePressEvent (QMouseEvent * e);
   
protected slots:
   void dateEnteredEvent(QDate);
};

#endif
