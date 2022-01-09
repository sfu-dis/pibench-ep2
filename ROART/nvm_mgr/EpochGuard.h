#pragma once

#include "threadinfo.h"

namespace NVMMgr_ns {
class EpochGuard {
  public:
    EpochGuard() { JoinNewEpoch(); }
    ~EpochGuard() { LeaveThisEpoch(); }
    static void DeleteNode(void *node) { 
    #ifdef MEMORY_FOOTPRINT
    	switch (((PART_ns::N*)node)->getType()) {
		    case PART_ns::NTypes::N4: {
		        pmem_deallocated += sizeof(PART_ns::NTypes::N4);
		    }
		    case PART_ns::NTypes::N16: {
		        pmem_deallocated += sizeof(PART_ns::NTypes::N16);
		    }
		    case PART_ns::NTypes::N48: {
		        pmem_deallocated += sizeof(PART_ns::NTypes::N48);
		    }
		    case PART_ns::NTypes::N256: {
		        pmem_deallocated += sizeof(PART_ns::NTypes::N256);
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