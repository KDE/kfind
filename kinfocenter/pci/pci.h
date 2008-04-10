/* Retrieve information about PCI subsystem trough libpci library from
   pciutils package. This should be possible on Linux, BSD and AIX.

   Author: Konrad Rzepecki <hannibal@megapolis.pl>
*/

#ifndef _PCI_H_
#define _PCI_H_

#include <QTreeWidget>

bool GetInfo_PCIUtils(QTreeWidget* tree);

#endif //_PCI_H_
