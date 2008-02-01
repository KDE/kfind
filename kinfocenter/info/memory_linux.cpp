#include <sys/sysinfo.h>
#include <unistd.h>
#include <stdlib.h>
#include <QFile>

void KMemoryWidget::fetchValues()
{
  struct sysinfo info;
  
  sysinfo(&info);	/* Get Information from system... */
  
  /* 
   * The sysinfo.mem_unit member variable is not available in older 2.4 kernels.
   * If you have troubles compiling this code, set mem_unit to "1".
   */
    
  const int mem_unit = info.mem_unit;

  Memory_Info[TOTAL_MEM]    = MEMORY(info.totalram)  * mem_unit; // total physical memory (without swaps)
  Memory_Info[FREE_MEM]     = MEMORY(info.freeram)   * mem_unit; // total free physical memory (without swaps)
  Memory_Info[SHARED_MEM]   = MEMORY(info.sharedram) * mem_unit; 
  Memory_Info[BUFFER_MEM]   = MEMORY(info.bufferram) * mem_unit; 
  Memory_Info[SWAP_MEM]     = MEMORY(info.totalswap) * mem_unit; // total size of all swap-partitions
  Memory_Info[FREESWAP_MEM] = MEMORY(info.freeswap)  * mem_unit; // free memory in swap-partitions
  
  QFile file("/proc/meminfo");
  if (file.open(QIODevice::ReadOnly)) {
	char buf[512];
	while (file.readLine(buf, sizeof(buf) - 1) > 0) {
		if (strncmp(buf,"Cached:",7)==0) {
			unsigned long v;
			v = strtoul(&buf[7],NULL,10);			
			Memory_Info[CACHED_MEM] = MEMORY(v) * 1024; // Cached memory in RAM
			break;
		}
	}
	file.close();
  }
}

