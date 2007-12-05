
#ifndef _kinfocenter_kcrootonly_
#define _kinfocenter_kcrootonly_

#include <kcmodule.h>
class KComponentData;

class KCRootOnly: public KCModule {
public:
	KCRootOnly(const KComponentData &inst, QWidget *parent);
};

#endif
