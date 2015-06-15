#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "memtester.h"

#define IS_PRESENT(X) ((X & (1LL<<63)) >> 63)
#define GET_PFN(X) (X & 0x007FFFFFFFFFFFFF)

int get_phy_addr(ull *out_phy_addr, ull pid, ull virt_addr) {
	int pagemap_fd;
	char pagemap_path[256];
	ull pagesize;
	ull pagesizemask;
	ull index;
	ull offset;
	ull seek_result;

	if (pid > 0) {
		sprintf(pagemap_path, "/proc/%llu/pagemap", pid);
	} else {	// pid == 0
		sprintf(pagemap_path, "/proc/self/pagemap");
	}

	pagemap_fd = open(pagemap_path, O_RDONLY);
	if (pagemap_fd == -1) {
		fprintf(stderr, "failed to open %s\n", pagemap_path);
		return -1;
	}

	pagesize = (ull)memtester_pagesize();
	pagesizemask = ~(pagesize - 1);

	index = (((virt_addr & pagesizemask) / pagesize) * sizeof(ull));
	offset = (virt_addr & (~pagesizemask));

	seek_result = (ull)lseek64(pagemap_fd, (off64_t)index, SEEK_SET);
	if (seek_result != index) {
		fprintf(stderr, "failed to seek\n");
		close(pagemap_fd);
		return -2;
	}

	if (read(pagemap_fd, (void *)out_phy_addr, (size_t)sizeof(ull)) != sizeof(ull)) {
		fprintf(stderr, "failed to read\n");
		close(pagemap_fd);
		return -3;
	}

	close(pagemap_fd);

	if (IS_PRESENT(*out_phy_addr)) {

		*out_phy_addr = GET_PFN(*out_phy_addr);
		*out_phy_addr = *out_phy_addr * pagesize;
		*out_phy_addr = *out_phy_addr + offset;

		return 0;

	} else {
		fprintf(stderr, "invalide pfn\n");
		return -4;
	}
}
