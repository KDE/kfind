
#include <sys/param.h>
#if __NetBSD_Version__ > 103080000
#define UVM
#endif
#if defined(__OpenBSD__)
#define UVM
#endif

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#ifdef UVM
#include <uvm/uvm_extern.h>
#else
#include <vm/vm_swap.h>
#endif

void KMemoryWidget::fetchValues()
{
  int mib[2];
  size_t len;
#ifdef UVM
#if __NetBSD_Version__ > 106000000
  struct  uvmexp_sysctl uvmexp;
#else
  struct  uvmexp uvmexp;
#endif
#else
  struct swapent *swaplist;
  int64_t nswap, rnswap, totalswap, freeswap, usedswap;
#endif
#if __NetBSD_Version__ > 106170000 /* 1.6Q+ */
  quad_t memory;
#else
  int memory;
#endif
  
  /* memory */
#if __NetBSD_Version__ > 106170000 /* 1.6Q+ */
  mib[0] = CTL_HW;
  mib[1] = HW_PHYSMEM64; 
#else 
  mib[0] = CTL_HW;
  mib[1] = HW_PHYSMEM;
#endif
  len = sizeof(memory);
  if( sysctl(mib,2,&memory,&len,NULL,0)< 0 )
    Memory_Info[TOTAL_MEM]    = NO_MEMORY_INFO;
  else
    Memory_Info[TOTAL_MEM]    = memory;

#ifdef UVM
  mib[0] = CTL_VM;
#if __NetBSD_Version__ > 106000000
  mib[1] = VM_UVMEXP2;
#else
  mib[1] = VM_UVMEXP;
#endif
  len = sizeof(uvmexp);
  if ( sysctl(mib, 2, &uvmexp, &len, NULL, 0) < 0 ) {
    Memory_Info[FREE_MEM]     = NO_MEMORY_INFO;
    Memory_Info[ACTIVE_MEM]   = NO_MEMORY_INFO;
    Memory_Info[INACTIVE_MEM] = NO_MEMORY_INFO;
    Memory_Info[SWAP_MEM]     = NO_MEMORY_INFO;
    Memory_Info[FREESWAP_MEM] = NO_MEMORY_INFO;
    Memory_Info[CACHED_MEM]   = NO_MEMORY_INFO;
  } else {
    t_memsize pgsz = MEMORY(uvmexp.pagesize);
    Memory_Info[FREE_MEM]     = pgsz * uvmexp.free;
    Memory_Info[ACTIVE_MEM]   = pgsz * uvmexp.active;
    Memory_Info[INACTIVE_MEM] = pgsz * uvmexp.inactive;
    Memory_Info[SWAP_MEM]     = pgsz * uvmexp.swpages;
    Memory_Info[FREESWAP_MEM] = pgsz * (uvmexp.swpages - uvmexp.swpginuse);
#if __NetBSD_Version__ > 106000000
    Memory_Info[CACHED_MEM]   = pgsz * (uvmexp.filepages + uvmexp.execpages);
#else
    Memory_Info[CACHED_MEM]   = NO_MEMORY_INFO;
#endif
    }
#else
  Memory_Info[FREE_MEM]       = NO_MEMORY_INFO;
  Memory_Info[ACTIVE_MEM]     = NO_MEMORY_INFO;
  Memory_Info[INACTIVE_MEM]   = NO_MEMORY_INFO;

  /* swap */
  totalswap = freeswap = usedswap = 0;
  nswap = swapctl(SWAP_NSWAP,0,0);
  if ( nswap > 0 ) {
    if ( (swaplist = (struct swapent *)malloc(nswap * sizeof(*swaplist))) ) {
      rnswap = swapctl(SWAP_STATS,swaplist,nswap);
      if ( rnswap < 0 || rnswap > nswap )
	totalswap = freeswap = -1;	/* Error */
      else {
	while ( rnswap-- > 0 ) {
	  totalswap += swaplist[rnswap].se_nblks;
	  usedswap += swaplist[rnswap].se_inuse;
	}
	freeswap = totalswap - usedswap;
      }
    } else
      totalswap = freeswap = -1;	/* Error */

    if ( totalswap == -1 ) {
	Memory_Info[SWAP_MEM]     = NO_MEMORY_INFO;
	Memory_Info[FREESWAP_MEM] = NO_MEMORY_INFO;
    } else {				
	Memory_Info[SWAP_MEM]     = MEMORY(totalswap);
	Memory_Info[FREESWAP_MEM] = MEMORY(freeswap);
    }
  }
#endif
}
