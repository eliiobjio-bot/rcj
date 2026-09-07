
/**********************************************************************************************
 *  File:         phy_common.h
 *
 *  Description:  Common parameters and lookup functions for PHY
 *
 *  Reference:    3GPP TS 36.211 version 10.0.0 Release 10
 *********************************************************************************************/
#ifndef SRSRAN_XW_COMMON_H
#define SRSRAN_XW_COMMON_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


#include "srsran/config.h"

SRSRAN_API int XW_ul_PhaseError_rawCal(int len, cf_t *intput, float *phase_err);

SRSRAN_API int XW_DeModulation(float *InputData_i,float *InputData_q,int length,int Qm,char *OutputBit);

SRSRAN_API int XW_DeModulation_lxg_temp(float *InputData_i, float *InputData_q, int length, int Qm, float *OutputBit);

SRSRAN_API int XW_ul_Scrambling(char  *input,int length,char *Scrb);

SRSRAN_API int XW_ul_Scrambling_lxg(float *input, int length, float *Scrb);

SRSRAN_API int XW_DeInterleaver(char *InputBit,int Length,char *OutputBit);

SRSRAN_API int XW_DeInterleaver_lxg(float *InputBit, int Length, float *OutputBit);

SRSRAN_API int StarLink_TurboDeRateMatch(float *InputData, int length_input, int Delta_N, float *DeRateMatchOut, int *DeRateMatchOutLength);

SRSRAN_API int StarLink_TurboDeRateMatch_new(char *InputData, int length_input, int Delta_N, char *DeRateMatchOut, int *DeRateMatchOutLength);

SRSRAN_API int StarLink_TurboDeRateMatch_lxg(float *InputData, int length_input, int Delta_N, float *DeRateMatchOut, int *DeRateMatchOutLength);

SRSRAN_API int StarLink_TurboDecode(char *TurboOut, float *dk, int dk_Length, int num_iter, int K, int L_c);

SRSRAN_API int StarLink_DeTurboInterGen_Char(char *InputBit, int length_k, int *TurboInterleaverTable, char *TurboInterleaverOut);

SRSRAN_API int StarLink_TurboDecode_WindowMaxLogMap(char *DeRateMatchOut, int DeRateMatchOut_Length, char *TurboOut);

SRSRAN_API int StarLink_TurboInterleaverTableGen(int *InputBit, int length_k, int *TurboInterleaverTable);

SRSRAN_API int ul_StarLink_GcdCalculate(int a, int b);

SRSRAN_API int StarLink_Logmap1(float *rec_s, int K, float *L_a, float *L_a11, float *L_tail);

SRSRAN_API int StarLink_DeTurboInter(float *InputBit, int length_k, int *TurboInterleaverTable, float *DeTurboInterleaverOut);

SRSRAN_API int StarLink_DeTurboInter_Char(char *InputBit, int length_k, int *TurboInterleaverTable, char *DeTurboInterleaverOut);

SRSRAN_API int StarLink_DeTurboInterGen(float *InputBit, int length_k, int *TurboInterleaverTable, float *TurboInterleaverOut);

SRSRAN_API int StarLink_DeTurboInterGen_Char(char *InputBit, int length_k, int *TurboInterleaverTable, char *TurboInterleaverOut);

SRSRAN_API int StarLink_Check_CRC(char *bits,int N,int DeCRC_Type,int MaskState);

SRSRAN_API int StarLink_Check_CRC_fix(char *bits,int N,int DeCRC_Type,int MaskState);

SRSRAN_API int XW_DL_Byte_to_Bit(uint8_t *pduBits,int Byte_len,char *Outbit);

SRSRAN_API int XW_frequency_move_para_config(int slot_num,int symbol_rate,int oversample_rate,int band,int cc,int *center,int *Width);

SRSRAN_API int XW_MCS_PUI_Config(int len,int *MCS,int *EpuiState,char *PUI,int PUI_start,int PUI_fix_num);

SRSRAN_API int XW_DL_PUI_EPUI(int bye_pui,char *EPUI,char *PUI);

SRSRAN_API int XW_Power_adjust_for_max(int datain_len,float power_factor,cf_t *data_ifft);

SRSRAN_API int XW_Per_Slot_ifft_and_power_adjust(int slot_num,int sf_len,int *slot_allocation,int multi_cc_num,int slot_begin,cf_t *data_fft,cf_t *data_ifft);

SRSRAN_API int XW_Frequency_move(int OutputLength,int oversample_rate,int sf_len,int slot_allocation, int slot_num,int symbol_rate,int band,int cc,cf_t *data_fft,cf_t *Wv_buffer);

SRSRAN_API int  XW_ud_FFT(int N,cf_t *in,cf_t *out);

SRSRAN_API int  XW_ud_IFFT(int N,cf_t *in,cf_t *out);

SRSRAN_API int EVM_cal(cf_t *Input_i,cf_t *Ref_I,int length,int Qm,float *EVM);

SRSRAN_API int min_value(float *Input,int len,int *index);

SRSRAN_API int XW_all_Slot_ifft(cf_t *Temp,cf_t *data_fft,cf_t *data_ifft);

#endif // SRSRAN_PHY_COMMON_H
