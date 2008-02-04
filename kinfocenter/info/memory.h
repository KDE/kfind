#ifndef _MEMORY_H_KDEINFO_INCLUDED_
#define _MEMORY_H_KDEINFO_INCLUDED_

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

#include <kcmodule.h>
#include <kaboutdata.h>
class QStringList;

/* better to use quint64, because some 32bit-machines have more total
   memory (with swap) than just the 4GB which fits into a 32bit-long */
typedef quint64 t_memsize;

#define COLOR_USED_MEMORY QColor(255,0,0)
#define COLOR_USED_SWAP   QColor(255,134,64)
#define COLOR_FREE_MEMORY QColor(127,255,212)

class KMemoryWidget:public KCModule {
  Q_OBJECT

  public:
    KMemoryWidget(QWidget *parent, const QVariantList &);
    ~KMemoryWidget();

    QString quickHelp() const;

  private:
    QString Not_Available_Text;
    QTimer *timer;

    bool ram_colors_initialized,
	swap_colors_initialized,
	all_colors_initialized;

    QColor ram_colors[4];
    QString ram_text[4];

    QColor swap_colors[2];
    QString swap_text[2];

    QColor all_colors[3];
    QString all_text[3];

    void fetchValues();

    bool Display_Graph(int widgetindex,
		      int count,
		      t_memsize total,
		      t_memsize *used,
		      QColor *color,
		      QString *text);

    void paintEvent(QPaintEvent *);
};


#endif // _MEMORY_H_KDEINFO_INCLUDED_

