/***********************************************************************
 *
 *  kftabdlg.h
 *
 ***********************************************************************/

#ifndef KFTABDLG_H
#define KFTABDLG_H

#include <qtabwidget.h>
#include <qvalidator.h> // for KDigitValidator

#include <kurl.h>
#include <kmimetype.h>

#include "kdatecombo.h"

class QButtonGroup;
class QPushButton;
class QRadioButton;
class QCheckBox;
class QLineEdit;
class QString;
class QDate;
class QRegExp;
class QDialog;
class QComboBox;
class QSpinBox;

class KfDirDialog;

class KfindTabWidget: public QTabWidget
{
  Q_OBJECT

public:
  KfindTabWidget(QWidget * parent = 0, const char *name=0);
  virtual ~KfindTabWidget();
  void initMimeTypes();
  void setQuery(class KQuery * query);
  void setDefaults();

  void beginSearch();
  void endSearch();
  void loadHistory();
  void saveHistory();

  void setURL( const KURL & url );

public slots:
  void setFocus();

private slots:
  void getDirectory();
  void fixLayout();
  void slotSizeBoxChanged(int);
  void slotEditRegExp();

signals:
    void startSearch();

protected:
public:
  QComboBox   *nameBox;
  QComboBox   *dirBox;
  // for first page
  QCheckBox   *subdirsCb;
  QCheckBox *useLocateCb;
  // for third page
  QComboBox *typeBox;
  QLineEdit * textEdit;
  QCheckBox *caseSensCb;
  QComboBox *m_usernameBox;
  QComboBox *m_groupBox;
  //for fourth page
  QLineEdit *metainfoEdit;
  QLineEdit *metainfokeyEdit;
  QDate dt;
  
private:
  bool isDateValid();

  QString date2String(const QDate &);
  QDate &string2Date(const QString &, QDate * );

  QWidget *pages[3];

  //1st page
  QPushButton *browseB;

  KfDirDialog *dirselector;

  //2nd page
  QCheckBox   *findCreated;
  QComboBox   *betweenType;
  QButtonGroup *bg;
  QRadioButton *rb[2];
  KDateCombo * fromDate;
  KDateCombo * toDate;
  QSpinBox *timeBox;

  //3rd page
  QComboBox *sizeBox;
  QComboBox *sizeUnitBox;
  QSpinBox *sizeEdit;
  QCheckBox *caseContextCb;
  QCheckBox *regexpContentCb;
  QDialog *regExpDialog;

  KURL m_url;

  KMimeType::List m_types;
};

class KDigitValidator : public QValidator
{
  Q_OBJECT

public:
  KDigitValidator(QWidget * parent, const char *name = 0 );
  ~KDigitValidator();

  QValidator::State validate(QString & input, int &) const;

 private:
  QRegExp *r;
};

#endif
