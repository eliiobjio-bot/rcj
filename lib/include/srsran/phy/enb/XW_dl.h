/**
 * Copyright 2013-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

/******************************************************************************
 *  File:         enb_dl.h
 *
 *  Description:  ENB downlink object.
 *
 *                This module is a frontend to all the downlink data and control
 *                channel processing modules for the ENB transmitter side.
 *
 *  Reference:
 *****************************************************************************/

#ifndef SRSRAN_XW_DL_H
#define SRSRAN_XW_DL_H

#include <stdbool.h>

#include "srsran/phy/ch_estimation/refsignal_dl.h"
#include "srsran/phy/common/phy_common.h"
#include "srsran/phy/dft/ofdm.h"
#include "srsran/phy/phch/dci.h"
#include "srsran/phy/phch/pbch.h"
#include "srsran/phy/phch/pcfich.h"
#include "srsran/phy/phch/pdcch.h"
#include "srsran/phy/phch/pdsch.h"
#include "srsran/phy/phch/pdsch_cfg.h"
#include "srsran/phy/phch/phich.h"
#include "srsran/phy/phch/pmch.h"
#include "srsran/phy/phch/ra.h"
#include "srsran/phy/phch/regs.h"
#include "srsran/phy/sync/pss.h"
#include "srsran/phy/sync/sss.h"

#include "srsran/phy/enb/enb_ul.h"
#include "srsran/phy/enb/enb_dl.h"
#include "srsran/phy/ue/ue_dl.h"

#include "srsran/phy/utils/debug.h"
#include "srsran/phy/utils/vector.h"

#include "srsran/config.h"
SRSRAN_API int  XW_IFFT(int N,cf_t *in,cf_t *out);

SRSRAN_API int  XW_FFT(int N,cf_t *in,cf_t *out);

SRSRAN_API int StarLink_TxDL_PFCCH(float Phi0, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_TxDL_PSYCH(char *InputBits, int InputBits_Length, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_TxDL_PSBCH(char *InputBits, int InputBits_Length, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_Tx_Filter_PTDCH(int channel_type,int cyc,int sf_len,float *InputI, float *InputQ, int SymbolLength, float *OutputI, float *OutputQ, int *OutputLength,cf_t* Wv_buffer);

SRSRAN_API int XW_TXDL_PTDCH(char *InputBit,int length,float *PTDCH_i,float *PTDCH_q,int *PTDCH_len);

SRSRAN_API int StarLink_TxDL_PDCH_11(char *InputBits, int InputBits_Length, int DataType, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_TxDL_PDCH_12(char *InputBits, int InputBits_Length, int DataType, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_TxDL_PSCH_11(char *InputBits, int InputBits_Length, char *PUI, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_TxDL_PSCH_11_new(char *InputBits, int InputBits_Length, char *PUI, int EpuiState, char *EPUI, int MCS, int Band, float *Output_i, float *Output_q, int *OutputLength,float *CommoMemory);

SRSRAN_API int StarLink_TxDL_PSCH_12(char *InputBits, int InputBits_Length, char *PUI, int EpuiState, char *EPUI, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_TxDL_PSCH_12_new(char *InputBits, int InputBits_Length, char *PUI, int EpuiState, char *EPUI, int MCS, int Band, float *Output_i, float *Output_q, int *OutputLength,float *CommoMemory);

SRSRAN_API int StarLink_TxDL_PSCH_51(char *InputBits, int InputBits_Length, char *PUI, int EpuiState, char *EPUI, int MCS, int Band, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_TxDL_PSCH_51_new(char *InputBits, int InputBits_Length, char *PUI, int EpuiState, char *EPUI, int MCS, int Band, float *Output_i, float *Output_q, int *OutputLength,float *CommoMemory);

SRSRAN_API int StarLink_TxDL_PSCH_52(char *InputBits, int InputBits_Length, char *PUI, int EpuiState, char *EPUI, int MCS, int Band, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_TxDL_PSCH_52_new(char *InputBits, int InputBits_Length, char *PUI, int EpuiState, char *EPUI, int MCS, int Band, float *Output_i, float *Output_q, int *OutputLength,float *CommoMemory);

SRSRAN_API int XW_resample(int sf_len,float* InputI,float* InputQ,int InputLength,cf_t* Wv_buffer,int XW_buffer_len);

SRSRAN_API int StarLink_Tx_Filter(int channel_type,int cyc,int sf_len,float *InputI, float *InputQ, int SymbolLength, float *OutputI, float *OutputQ, int *OutputLength,cf_t* Wv_buffer);

SRSRAN_API int StarLink_TxDL_PMBCH(char *InputBits, int InputBits_Length, float *Output_i, float *Output_q, int *OutputLength);

SRSRAN_API int StarLink_Add_CRC(char *InputBits, int InputBitsLength, int CrcType, int MaskState, char *CrcOut, int *CrcOutLen);

SRSRAN_API int StarLink_TurboEncode(char *TxTurboInputData, int length_k, char **Turbo_Out);

SRSRAN_API int StarLink_TurboInterleaver(char *InputBit, int length_k, char *TurboInterOutBit);

SRSRAN_API int StarLink_GcdCalculate(int a, int b);

SRSRAN_API int StarLink_TurboRateMatch(char **Turbo_Out, int length_k, int Delta_N, char *RateMatchOut, int *RateMatchOutLength);

SRSRAN_API int XW_Interleaver(char *InputBit,int Length,char *OutputBit);

SRSRAN_API int XW_Scrambling(char  *input,int length,char *Scrb);

SRSRAN_API int XW_Modulation(char *InputBit,int length,int Qm,float *ModOut_i,float *ModOut_q);

SRSRAN_API int StarLink_Root_Raised_Cosine_Filter(float *h, int span,int sps,float alpha);

SRSRAN_API int StarLink_FastConv(int sf_len,float *BaseBand_in_I, float *BaseBand_in_Q, int lena,int lenb,float dataina_i[],float dataina_q[],float datainb_i[],float datainb_q[],float fastconv_i[],float fastconv_q[], int *fastconv_len,cf_t* Wv_buffer);

SRSRAN_API int DS_Scrambling_code_generate(char Scrambling_code,char *output_i,char *output_q);

SRSRAN_API int gen_OVSF1(int SF,int SF_i,char *OVSF);

SRSRAN_API int gen_OVSF(int SF,int SF_i,char *OVSF);

SRSRAN_API int Spread(float Input_i[], float Input_q[],int Input_Length,char *OVSF, int SF, float *Output_i, float *Output_q);

SRSRAN_API int psc_ssc_code_generate(char *psc, char *ssc);

SRSRAN_API int StarLink_TxDL_DS_SCH(char Scrambling_code,char k_s,float Power,float *Output_i, float *Output_q);

SRSRAN_API int StarLink_TxDL_DS_CPICH(char *Scrambling_i,char *Scrambling_q,char k_s,float Power,float *Output_i, float *Output_q);

SRSRAN_API int StarLink_TxDL_DS_PBCH(char Channel_type,char *InputBits,char *Scrambling_i,char *Scrambling_q,char k_s,int SF_i_in,float Power,float *Output_i, float *Output_q);

SRSRAN_API int StarLink_TxDL_DS_PDTCH23(char Channel_type, char *InputBits, char *Scrambling_i,char *Scrambling_q,char k_s,int SF_i_in,float Power,float *Output_i, float *Output_q);

SRSRAN_API int StarLink_TxDL_DS_PDTCH_T(char *InputBits, char *Scrambling_i,char *Scrambling_q,char k_s,int SF_i,float Power,int SubFrame_offset,float *Output_i, float *Output_q);

SRSRAN_API int Filter(float *InputI, float *InputQ, int SymbolLength, int rate,float *OutputI, float *OutputQ, int *OutputLength,cf_t* Wv_buffer);

SRSRAN_API int xw_phy_printf_time();

SRSRAN_API int StarLink_Tx_multi_cc_Filter_PTDCH(int channel_type, int cyc, int sf_len, float *InputI, float *InputQ, int SymbolLength, 
float *OutputI, float *OutputQ, int *OutputLength, cf_t *Wv_buffer);

SRSRAN_API int XW_StarLink_TxDL_PMBCH(uint8_t *pduBits,int pduBitlen,char *turboInputbit,int turboInputbit_len,
float *data_modulate_i,float *data_modulate_q,int *data_after_modulate_length,int channel_time,int sf_len,
float *OutputI,float *OutputQ,int *OutputLength,cf_t *Wv_buffer );

SRSRAN_API int XW_StarLink_TxDL_PDCH(int slot_allocation,int PDCH_data_type, uint8_t *pduBits,int pduBitlen,char *turboInputbit,int turboInputbit_len,
float *data_modulate_i,float *data_modulate_q,int *data_after_modulate_length,int sf_len,int *slot_num,
float *OutputI,float *OutputQ,int *OutputLength,cf_t *Wv_buffer);

SRSRAN_API int XW_StarLink_TxDL_PSCH(int BandID,uint16_t pui, int slot_allocation, 
uint8_t *pduBits,int pduBitlen,char *turboInputbit,int turboInputbit_len,
float *data_modulate_i,float *data_modulate_q,int *data_after_modulate_length,int sf_len,
float *OutputI,float *OutputQ,int *OutputLength,cf_t *Wv_buffer,int *slot_num,int *symbol_rate);

SRSRAN_API int XW_StarLink_TxDL_PTDCH(uint8_t *pduBits,int pduBitlen,char *turboInputbit,int turboInputbit_len,
float *data_modulate_i,float *data_modulate_q,int *data_after_modulate_length,int *channel_time,int sf_len,
float *OutputI,float *OutputQ,int *OutputLength,cf_t *Wv_buffer );

SRSRAN_API int XW_StarLink_TxDL_PFCCH(float *data_modulate_i,float *data_modulate_q,int *data_after_modulate_length,int channel_time,int sf_len,
float *OutputI,float *OutputQ,int *OutputLength,cf_t *Wv_buffer);

SRSRAN_API int XW_StarLink_TxDL_PSBCH(uint8_t *pduBits,int pduBitlen,char *turboInputbit,int turboInputbit_len,
float *data_modulate_i,float *data_modulate_q,int *data_after_modulate_length,int channel_time,int sf_len,
float *OutputI,float *OutputQ,int *OutputLength,cf_t *Wv_buffer);

SRSRAN_API int StarLink_Tx_multi_cc_Filter(int channel_type, int cyc, int sf_len, float *InputI, float *InputQ, int SymbolLength, 
float *OutputI, float *OutputQ, int *OutputLength, cf_t *Wv_buffer);

SRSRAN_API int xw_dl_get_dl_config_para(int coreNum,srsran_enb_dl_t *q,int burst_ID_all[16][5],int band_all[16][5],int cc_all[16][5],int slot_allocation_all[16][5],int bit_len_all[16][5],int slot_num_all[16][5],
char *PUI,char *EPUI,char *turboInputData_all );

SRSRAN_API int StarLink_TxDL_PSCH_gen(char *InputBits, int InputBits_Length, char *PUI, char *EPUI, int Band_i, int EpuiState, int MCS, float *Output_i, float *Output_q, int *OutputLength,float *CommoMemory);

#endif