/*
 * Copyright (c) CY Technologies Co., Ltd. 2023-2024. All rights reserved.
 * Description:
 * Create: 2023/8/07
 */

#ifndef CY_SCPI_CHANNEL_H
#define CY_SCPI_CHANNEL_H
#include <memory>
#include "mng_scpiparse.h"
#include <mutex>
class CYSCPI_CHANNEL
{
	CYSCPI_CHANNEL() = delete;
public:
	static std::shared_ptr<CYSATScpiParse> mp_scpi;
	static int HandleScpiInput(char *cmd, std::string& data);
	//int(const std::string&, std::string&)
private:
	static std::mutex tcplock;
};


#endif  // A_PROJECT_DEMO_CHANNEL_H
