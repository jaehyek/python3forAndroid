#ifndef	__MEMTESTER_GET_PHY_ADDR_H__
#define	__MEMTESTER_GET_PHY_ADDR_H__

#include "types.h"

#ifdef __LP64__
#define GET_VIRT_ADDR(X) (((ull)X) & 0xFFFFFFFFFFFFFFFF)
#else
#define GET_VIRT_ADDR(X) ((ull)(((ul)X) & 0x00000000FFFFFFFF))
#endif

int get_phy_addr(ull *out_phy_addr, ull pid, ull virt_addr);

#endif 	// __MEMTESTER_GET_PHY_ADDR_H__
