#pragma once

#include <stddef.h>
#include <stdint.h>

struct PciDeviceInfo
{
	uint16_t vendId, devId;
	uint64_t bar0, bar1, bar2, bar3, bar4;
};

class Driver
{
public:
	virtual bool Init(PciDeviceInfo& info) = 0;
	virtual void Recv(uint32_t cmd, void* buf) = 0;
};