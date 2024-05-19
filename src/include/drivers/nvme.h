#pragma once

#include "driver.h"

class NVMe
{
public:
	bool Init(PciDeviceInfo& info);
	void Recv(uint32_t cmd, void* buf) {}
};