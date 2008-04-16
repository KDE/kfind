/* Retrieve information about PCI subsystem through libpci library from
   pciutils package. This should be possible on Linux, BSD and AIX.

   Author: Konrad Rzepecki <hannibal@megapolis.pl>
*/

#ifndef KCONTROL_PCI_H
#define KCONTROL_PCI_H

#include <QTreeWidget>

bool GetInfo_PCIUtils(QTreeWidget* tree);

#endif //KCONTROL_PCI_H
