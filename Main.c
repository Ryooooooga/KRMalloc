#include "MemoryAllocator.h"

int main(void) {
	void* p0 = MemoryAllocate(1024);
	MemoryDump(stdout);
	void* p1 = MemoryAllocate(1024*1024);
	MemoryDump(stdout);
	void* p2 = MemoryAllocate(1024);
	MemoryDump(stdout);
	
	MemoryFree(p1);
	MemoryDump(stdout);
	MemoryFree(p0);
	MemoryDump(stdout);
	MemoryFree(p2);
	MemoryDump(stdout);
	
	return 0;
}
