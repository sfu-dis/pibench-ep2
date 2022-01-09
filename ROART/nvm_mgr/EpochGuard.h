#pragma once

#include "threadinfo.h"

namespace NVMMgr_ns {
class EpochGuard {
  public:
    EpochGuard() { JoinNewEpoch(); }
    ~EpochGuard() { LeaveThisEpoch(); }
    static void DeleteNode(void *node) { 
    #ifdef MEMORY_FOOTPRINT
    	switch ((N*)node->getType()) {
		    case NTypes::N4: {
		        pmem_deallocated += sizeof(N4);
		    }
		    case NTypes::N16: {
		        pmem_deallocated += sizeof(N16);
		    }
		    case NTypes::N48: {
		        pmem_deallocated += sizeof(N48);
		    }
		    case NTypes::N256: {
		        pmem_deallocated += sizeof(N256);
		    }
		    default: {
		    	printf("Not a N type node!!!\n");
		        // assert(false);
		    }
	    }
	#endif
    	MarkNodeGarbage(node); 
    }
};
} // namespace NVMMgr_ns