/*
 * Copyright (c) CY Technologies Co., Ltd. 2023-2024. All rights reserved.
 * Description:
 * Create: 2023/8/07
 */
#include <assert.h>
#include <iostream>
#include <memory>
#include "../hdr/mng_scpi_channel.h"
std::shared_ptr<CYSATScpiParse> CYSCPI_CHANNEL::mp_scpi = nullptr;
std::mutex CYSCPI_CHANNEL::tcplock;
int CYSCPI_CHANNEL::HandleScpiInput(char* pszCommand, std::string& output)
{
    std::cout << "Scpi channel received cmd:" << pszCommand << std::endl;
	if(mp_scpi == nullptr)
	{
        // var mp_scpi is inited in enb.cc init function
        std::cout <<" exsits before send ,no construct mp_scpi." << std::endl;
        return -1;
	}
    // lockContext();
    tcplock.try_lock();
    mp_scpi->RemoteCommandProcess(pszCommand, output);
    tcplock.unlock();
    // unlockContext();
    return 0;
}
