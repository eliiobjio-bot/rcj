/*
 * Copyright (c) CY Technologies Co., Ltd. 2023-2024. All rights reserved.
 * Description:
 * Create: 2023/8/07
 */
#include <assert.h>
#include <iostream>
#include <memory>
#include "../hdr/cy_scpi_channel.h"
std::shared_ptr<CYSATScpiParse> CYSCPI_CHANNEL::mp_scpi = nullptr;
int CYSCPI_CHANNEL::HandleScpiInput(char* pszCommand, std::string& output)
{
    std::cout << "Scpi channel received cmd:" << pszCommand << std::endl;
	if(mp_scpi == nullptr)
	{
        // var mp_scpi is inited in enb.cc init function
        return -1;
	}
    // lockContext();

    mp_scpi->RemoteCommandProcess(pszCommand, output);
    // unlockContext();
    return 0;
}
