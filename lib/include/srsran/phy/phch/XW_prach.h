

/******************************************************************************
 *  File:         XW_prach.h
 *
 *  Description:  Physical random access channel.
 *
 *  Reference:    
 *****************************************************************************/

#ifndef SRSRAN_XW_PRACH_H
#define SRSRAN_XW_PRACH_H

#include "srsran/config.h"
#include "srsran/phy/common/phy_common.h"
//#include "srsran/phy/common/XW_ul_common.h"
#include "srsran/phy/common/phy_common_nr.h"
#include "srsran/phy/dft/dft.h"
#include <complex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <fftw3.h>

//#define SRSRAN_PRACH_MAX_LEN (2 * 24576 + 21024) // Maximum Tcp + Tseq

// Long PRACH ZC sequence sequence length
//#define SRSRAN_PRACH_N_ZC_LONG 839

// Short PRACH ZC sequence sequence length
//#define SRSRAN_PRACH_N_ZC_SHORT 139

/** Generation and detection of RACH signals for uplink.
 *  Based on 3GPP TS 36.211 version 10.7.0 Release 10.
 */

SRSRAN_API int XW_prach_init(srsran_prach_t* p, int symbol_sz);

SRSRAN_API bool XW_prach_tti(srsran_prach_t* p, uint32_t current_tti);

SRSRAN_API int XW_prach_detect_offset(srsran_prach_t* p,uint32_t sig_len,float *Ta,int *prach_nof_det,int *CRC_check,float *prach_ta);

SRSRAN_API int XW_prach_detect_offset_format1(srsran_prach_t *p, uint32_t sig_len, float *Ta, int *prach_nof_det, int *demod_len, float *demod, float *prach_ta);

SRSRAN_API int XW_pdch_detect_offset(srsran_prach_t* p,uint32_t sig_len,float *Ta,int *prach_nof_det,int *CRC_check,float *prach_ta, int slot_num, int mode);

SRSRAN_API int XW_pdch_detect_offset_pyf_zzy(srsran_prach_t* p,uint32_t sig_len,float *Ta,int *prach_nof_det,int *CRC_check,float *prach_ta, int slot_num, int mode,int *Mode, int voice_type);

SRSRAN_API int XW_psych_detect_offset(srsran_prach_t *p, uint32_t sig_len, float *Ta, int *prach_nof_det, int *CRC_check);

SRSRAN_API int XW_psych_detect_offset_pyf_zzy(srsran_prach_t *p, uint32_t sig_len, float *Ta, float *Pa,float *Fa, int *CRC_check,float forword_back_sum_time);

SRSRAN_API int XW_psch_detect_offset2(srsran_prach_t *p, uint32_t sig_len, float *Ta, int *prach_nof_det, int *CRC_check, float *psch_ta, int slot_num, int type,float forword_back_sum_time);

SRSRAN_API int XW_psch_detect_offset_52(srsran_prach_t *p, uint32_t sig_len, float *Ta, int *prach_nof_det, int *CRC_check, float *psch_ta, int slot_num, int type,float forword_back_sum_time);

SRSRAN_API int XW_prach_detect_offset_ptuch(srsran_prach_t* p,uint32_t sig_len,float *Ta,int *prach_nof_det,int *CRC_check,float *prach_ta);

SRSRAN_API int XW_PRACH_Generation(srsran_prach_t* p,int *data_len);

SRSRAN_API int XW_UL_Scrambling_PUI(char *input, int length, int channel, int Type, int Band, char *Scrb);

SRSRAN_API int FreqError_rawCal_new2(int len, int mOrder, float input_i[], float input_q[], double sample_rate, double *freqerr,double *fftwfIn, double *fftwfOut,fftwf_plan p_forward,fftwf_plan p_backward,int Nfft);

SRSRAN_API int PSCH52_Ref_Signal_Generate(int type, int Band, cf_t **signal_out, int *Out_signal_len);

SRSRAN_API int XW_RXUL_PSCH_52(cf_t *PDCH, int length, char *OutputBit, int *OutputBit_len, int slot_num, int mode, int Band, char *PUI);

SRSRAN_API void FFT_freq_shift(float FOE_test, cf_t *in_buffer, float *BaseBandSignal_I, float *BaseBandSignal_Q);

SRSRAN_API int XW_ul_Modulation(char *InputBit,int length,int Qm,cf_t* p);

SRSRAN_API int XW_ul_StarLink_Root_Raised_Cosine_Filter(cf_t* h, int span,int sps,float alpha);

SRSRAN_API int XW_ul_StarLink_FastConv(int sf_len,cf_t *dataina, int lena,int lenb,cf_t *datainb, int *fastconv_len,cf_t* out_buffer);

SRSRAN_API int resample(cf_t *Rx_prach_data,int data_len,int band,int cc,int sample_in,int sample_out,cf_t *Rx_prach_resample,cf_t *temp_buff,int *out_data_len);

SRSRAN_API int resample_ptuch(cf_t *Rx_prach_data,int data_len,int band,int cc,int sample_in,int sample_out,cf_t *Rx_prach_resample,cf_t *temp_buff,int *out_data_len);

SRSRAN_API int resample_psch_5(cf_t *Rx_prach_data,int data_len,int band,int cc,int sample_in,int sample_out,cf_t *Rx_prach_resample,cf_t *temp_buff,int *out_data_len);

SRSRAN_API int resample_ptuch_pyf_zzy(cf_t *Rx_prach_data,int data_len,int band,int cc,int sample_in,int sample_out,cf_t *Rx_prach_resample,cf_t *temp_buff,int *out_data_len);

SRSRAN_API int resample_psch_5_pyf_zzy(cf_t *Rx_prach_data,int data_len,int band,int cc,int sample_in,int sample_out,cf_t *Rx_prach_resample,cf_t *temp_buff,int *out_data_len);

SRSRAN_API int resample_ptuch_zx(cf_t *Rx_prach_data,int data_len,int band,int cc,int sample_in,int sample_out,cf_t *Rx_prach_resample,cf_t *temp_buff,int *out_data_len);

SRSRAN_API int resample_psch_5_52(cf_t *Rx_prach_data,int data_len,int band,int cc,int sample_in,int sample_out,cf_t *Rx_prach_resample,cf_t *temp_buff,int *out_data_len);

SRSRAN_API int XW_prach_FreqError(int data_len,int mOrder,cf_t *Rx_resample,int sample,float *FOE);

SRSRAN_API int XW_maxValueCal_flag(cf_t* data,int len,int *index);

SRSRAN_API int XW_maxValueCal_flag_pyf_zzy(cf_t *data, int len, int *index,float *crest);

SRSRAN_API int XW_UL_bit_syn(cf_t* data,int len,int OP,int *offset);

SRSRAN_API int XW_RXUL_PRACH(cf_t* PRACH,int length,char *OutputBit);

SRSRAN_API int XW_RXUL_PRACH_format1(float *Demod, int length, char *OutputBit);

SRSRAN_API int XW_RXUL_PDCH(cf_t* PDCH,int length,char *OutputBit, int *OutputBit_len,  int slot_num, int mode);

SRSRAN_API int XW_RXUL_PSYCH(cf_t* PSYCH,int length,char *OutputBit, int *OutputBit_len);

SRSRAN_API int XW_RXUL_PTUCH(cf_t* PRACH,int length,char *OutputBit);

SRSRAN_API int XW_fastconv_flip(int sf_len,cf_t *dataina, int lena,int lenb,cf_t *datainb, int *fastconv_len,cf_t* out_buffer);

SRSRAN_API int XW_power_trig(srsran_prach_t* prach,int flip_type,int data_len,int *prach_tti_point,int  *flag);

SRSRAN_API int time_trigger_judge_signal(cf_t *in_buffer,int in_buffer_len,int ul_type,int slot_location,int thresholds_val,int prinf_flag);

SRSRAN_API int time_trigger_judge_signal_mul_cc(cf_t *in_buffer, int in_buffer_len, int ul_type, int slot_location, int thresholds_val, int prinf_flag);

SRSRAN_API int ul_slot_allocation_translate(int multi_slot_ID[20],int slot_location_tmp[16][5],int muilti_cc_Flag);

SRSRAN_API int XW_Siganl_power_Noise_power_Calculate(cf_t *data_in,int signal_power_start,int signal_power_len,int noise_power_start,int noise_power_len,float *signal_power,float *noise_power,float *RSSI,float *SQI );

SRSRAN_API int XW_psch_detect_offset_multi_cc(srsran_xw_ul_sf_cfg_t *xw_ul_sf,int Type, cf_t* in_buffer, int band, int cc, int slot_allocation,int *CRC_check,float *Ta, float *Fa,char *PUI, char *Outputbit,int *Bitlen);

SRSRAN_API int XW_pdch_detect_offset_multi_cc(srsran_xw_ul_sf_cfg_t *xw_ul_sf,int Type, cf_t* in_buffer, int band, int cc, int slot_allocation,int *CRC_check,float *Ta, float *Fa,char *Outputbit,int *Bitlen);

SRSRAN_API int XW_PSCH_FFT_Para_init2(srsran_prach_t *xw_para,int Nfft_80k,int Nfft_16k,uint32_t cc_Num);

// SRSRAN_API int XW_PSCH_FFT_Para_init(srsran_prach_t *xw_para,int Nfft_80k,int Nfft_16k,int coreNum);

// SRSRAN_API int XW_PSCH_FFT_Para_Uninit(srsran_prach_t *xw_para,int coreNum);

SRSRAN_API int XW_just_fft_fftshift(int len,float dataina_i[],float dataina_q[],float fastconv_i[],float fastconv_q[],double *fftwfIn, double *fftwfOut,fftwf_plan p_forward,fftwf_plan p_backward);

SRSRAN_API int XW_just_ifft_ifftshift(int len,float dataina_i[],float dataina_q[],float fastconv_i[],float fastconv_q[],double *fftwfIn, double *fftwfOut,fftwf_plan p_forward,fftwf_plan p_backward);

SRSRAN_API void golayDecode(char received[72], char decoded[12]);

SRSRAN_API int XW_RXUL_Channel_Separation2(int band_start, float *data_in_ori_fft_i,float *data_in_ori_fft_q,int data_in_ori_fft_len,int ul_type,int band ,int cc,
                                           int *Samplerate,float *data_out_fft_i,float *data_out_fft_q,int *data_out_fft_len);

// SRSRAN_API int XW_RXUL_Channel_Separation(float *data_in_ori_fft_i,float *data_in_ori_fft_q,int data_in_ori_fft_len,int ul_type,int type,int band ,int cc ,
// 	                                      float *data_out_fft_i,float *data_out_fft_q,int *data_out_fft_len);

SRSRAN_API int XW_RXUL_Channel_Separation_new(float *data_in_ori_fft_i,float *data_in_ori_fft_q,int data_in_ori_fft_len,int ul_type,int band ,int cc,
                                   int *Type,int *Samplerate,float *data_out_fft_i,float *data_out_fft_q,int *data_out_fft_len);
SRSRAN_API int multi_cc_filter(srsran_prach_t *xw_fft_para,int per_cc,int type,float *Input_i,float *Input_q,int data_60ms_len,float *filter_i,float *filter_q,float *CommonMemory);
SRSRAN_API int multi_cc_filter_new(srsran_prach_t *xw_fft_para,int per_cc,int type,float *Input_i,float *Input_q,int data_60ms_len,float *filter_i,float *filter_q);
// SRSRAN_API int XW_PSCH_SLOT_Config(int per_cc,int slot_allocation_all[16][5],int slot_allocation_per_cc[5],int *slot_start);

SRSRAN_API int XW_PSCH_SLOT_Config_new(int per_cc,int slot_allocation_all[16][5],int *slot_start);

SRSRAN_API int per_cc_slot_start(int Burst_ID[16][5],int per_cc,int *slot_start);

SRSRAN_API int xw_output_clear2(srsran_prach_t *xw_fft_para,int coreNum);

// SRSRAN_API int xw_output_clear(srsran_prach_t *xw_fft_para,int coreNum);

SRSRAN_API int XW_RXUL_PDCH_demod_new2(int Second_flag,srsran_prach_t *xw_fft_para,int Type,int Band_i,int CC_num,int per_cc,int Slot_i,int Nfft_16k,int Nfft_80k,float *Input_i,float *Input_q,int data_in,int Samplerate,char *OutputBit,int *OutputBit_len,float *EVM,float *CommonMemory);

SRSRAN_API int XW_RXUL_PSCH_demod_new2(int Second_flag,srsran_prach_t *xw_fft_para,int Type,int Band_i,int CC_num,int per_cc,int Slot_i,int Nfft_16k,int Nfft_80k,float *Input_i,float *Input_q,int data_in,int Samplerate,char *OutputBit,int *OutputBit_len,char *PUI,float *EVM,float *CommonMemory);

// SRSRAN_API int XW_RXUL_PSCH_demod_new(srsran_prach_t *xw_para,int slot_start,int Type,int Band_i,int Nfft,int per_cc,float *Input_i,float *Input_q,int data_in,double Samplerate_in,int *slot_allocation,int *CRC_check,char *OutputBit,int *OutputBit_len,char *PUI,float *Ta,float *Fa,float *CommonMemory);

SRSRAN_API int XW_RXUL_PSCH_demod_new_for_mul_slot(int Second_flag,srsran_prach_t *xw_fft_para,int Type,int Band_i,int Nfft_16k,int Nfft_80k,int CC_num,int per_cc,float *Input_i,float *Input_q,int data_in,double Samplerate_in,float *temp_ifft_i,float *temp_ifft_q,int *slot_allocation,int *CRC_check,char *OutputBit,int *OutputBit_len,char *PUI,float *CommonMemory);

SRSRAN_API int XW_PDCH_Parameter_config(int Type,float time_forward,int *Symbol_Rate,int *Slot_len,int *Symbol_len,float *Guard_len,int *UW_Len,int *channel,int *time_ms,int *time_ms_data_ori,int *data_all_len,int *slot_num);

SRSRAN_API int XW_PSCH_Parameter_config2(int Type,float time_forward,int *Symbol_Rate,int *Slot_len,int *Symbol_len,float *Guard_len,char *UW,int *UW_Len,int *channel,int *time_ms,int *time_ms_data_ori,int *data_all_len,int *slot_num);

SRSRAN_API int XW_PSCH_Parameter_config(int Type,float time_forward,double *Samplerate_out,double *Symbol_Rate,int *Slot_len,int *Symbol_len,double *Guard_len,char *UW,int *UW_Len,int *channel,int *time_ms,int *time_ms_data_ori,int *data_all_len,int *slot_num);

// SRSRAN_API int XW_PSCH_Space_order(float **temp_after_fft_i,float **temp_after_fft_q,float **after_filter_i,float **after_filter_q,float **temp_ifft_i,float **temp_ifft_q,
// 	float **add_fre_error_i,float **add_fre_error_q,float **ref_data_i,float **ref_data_q,float **Rec_I,float **Rec_Q,float **Ref_I,float **Ref_Q,
// 	float *Temp_save,int data_60ms_len,int data_all_len,int Symbol_len,int *cnt_out_len);

SRSRAN_API int XW_PSCH_Space_order_new(float **temp_after_fft_i,float **temp_after_fft_q,float **after_filter_i,float **after_filter_q,float **add_fre_error_i,float **add_fre_error_q,float **slot_i_ifft_i,float **slot_i_ifft_q,
	                    float **ref_data_i,float **ref_data_q,float **Rec_I,float **Rec_Q,float **Ref_I,float **Ref_Q,float *Temp_save,int data_60ms_len,int data_all_len,int Symbol_len,int *cnt_out_len);

SRSRAN_API int XW_PSCH_60ms_ifft_switch(int per_cc,int type,int len,float dataina_i[],float dataina_q[],float fastconv_i[],float fastconv_q[],srsran_prach_t *xw_para);

SRSRAN_API int XW_PSCH_60ms_80_fft_switch(int per_cc,int type,int len,float dataina_i[],float dataina_q[],float fastconv_i[],float fastconv_q[],srsran_prach_t *xw_para);

SRSRAN_API int XW_RXUL_PSCH_Filter_Cal2(int type,float *temp_after_fft_i,float *temp_after_fft_q,int data_len_cal,float *after_filter_i,float *after_filter_q);

SRSRAN_API int XW_RXUL_Filter_Cal(int type,float *temp_after_fft_i,float *temp_after_fft_q,int data_len_cal,float *after_filter_i,float *after_filter_q);

SRSRAN_API int XW_RXUL_PSCH_Filter_Cal(int type,float time_forward_or_back,double Symbol_Rate,float *temp_after_fft_i,float *temp_after_fft_q,int temp_after_fft_len,float *after_filter_i,float *after_filter_q);

// SRSRAN_API int XW_RXUL_PSCH_fft(int type,int len,float dataina_i[],float dataina_q[],float fastconv_i[],float fastconv_q[],srsran_prach_t *xw_para);

SRSRAN_API int XW_RXUL_PSCH_ifft(int per_cc,int type,int len,float dataina_i[],float dataina_q[],float fastconv_i[],float fastconv_q[],srsran_prach_t *xw_para);

// SRSRAN_API int FFT_float_new(double *DataI,double *DataQ,float *dataRealpart, float *dataImagepart, int fftN, int fftFlag,fftwf_plan p_forward,fftwf_plan p_backward);

// SRSRAN_API int FreqError_rawCal_new(int len, int mOrder, float input_i[], float input_q[], double sample_rate, double *freqerr,double *fftwfIn, double *fftwfOut,fftwf_plan p_forward,fftwf_plan p_backward,int Nfft,float *CommonMemory);

SRSRAN_API int freq_error_com(float *Rec_I, float *Rec_Q,float Samplerate,float FOE,int len);

SRSRAN_API int XW_add_frequency_error(float *data_in_i,float *data_in_q,int data_in_len,double frequen_error,int time_ms,double Samplerate_out,float *data_out_i,float *data_out_q);

SRSRAN_API int XW_PDCH_sync_cricle_cal(srsran_prach_t *xw_para,int CC_num,int per_cc,int slot_i,int type,int data_rate,float *data_in_i,float *data_in_q,int data_in_len,float *data_ref_i,float *data_ref_q,int *sync_point);

SRSRAN_API int XW_PSCH_Ref_Signal_Gen(int type,int band,float **Ref_data_i,float **Ref_data_q,int *Ref_data_len);

SRSRAN_API int XW_PSCH_Sync_Cal2(int type,int CC_num,int per_cc,int Slot_i,float *data_in_i,float *data_in_q,int data_in_len,float *data_ref_i,float *data_ref_q,int ref_len,int *sync_point,srsran_prach_t *xw_fft_para);

// SRSRAN_API int XW_PSCH_Sync_Cal(int per_cc,int type,float *data_in_i,float *data_in_q,int data_in_len,float *data_ref_i,float *data_ref_q,int ref_len,int *sync_point,srsran_prach_t *xw_para,float *CommonMemory);

SRSRAN_API int XW_sync_cal2(float *data_in_i,float *data_in_q,int data_in_len,float *data_ref_i,float *data_ref_q,int ref_len,int *sync_point,double *fftwfIn, double *fftwfOut,fftwf_plan p_forward,fftwf_plan p_backward);

SRSRAN_API int XW_sync_cal(float *data_in_i,float *data_in_q,int data_in_len,float *data_ref_i,float *data_ref_q,int ref_len,int *sync_point,float *peak_,double *fftwfIn, double *fftwfOut,fftwf_plan p_forward,fftwf_plan p_backward);

// SRSRAN_API void fastconv_flip_new(int lena,int lenb,float dataina_i[],float dataina_q[],float datainb_i[],float datainb_q[],float fastconv_i[],float fastconv_q[],double *fftwfIn, double *fftwfOut,fftwf_plan p_forward,fftwf_plan p_backward);

SRSRAN_API int XW_PSCH_UW_Modulation2(int Type,char *UW,int UW_Len,int Band_i,int channel,float *UW_ModOut_i,float *UW_ModOut_q);

SRSRAN_API int XW_PSCH_UW_Modulation(int Type,char *UW,int UW_Len,int Band_i,int channel,float *UW_ModOut_i,float *UW_ModOut_q);

SRSRAN_API int XW_PSCH_UW_Map2(int Type,int *UW0_len,float *UW0_i,float *UW0_q,float *UW_ModOut_i,float *UW_ModOut_q);

SRSRAN_API int XW_PSCH_UW_Map(int Type,int *UW0_len,float *UW0_i,float *UW0_q,float *UW_ModOut_i,float *UW_ModOut_q);

SRSRAN_API int XW_best_sample_point_cal2(int over_sample,int Sync_point,double Guard_len, int UW0_len,float *UW_ModOut_i,float *UW_ModOut_q,float *Rec_I,float *Rec_Q,float *Ref_I,float *Ref_Q,
	                         float *add_freq_error_ifft_i,float *add_freq_error_ifft_q,float *UW0_i,float *UW0_q,int *Demod_flag,int *Sync_point_best,float *Phase_error);

SRSRAN_API int XW_best_sample_point_cal(int over_sample,int Sync_point,double Guard_len, int UW0_len,float *UW_ModOut_i,float *UW_ModOut_q,
	float *Rec_I,float *Rec_Q,float *Ref_I,float *Ref_Q,float *add_freq_error_ifft_i,float *add_freq_error_ifft_q,float *UW0_i,float *UW0_q,
	int *Demod_flag,int *Sync_point_best,double *Phase_error);

SRSRAN_API int PhaseError_rawCal(int len, float intput_i[], float intput_q[], float *phase_err);

SRSRAN_API void EVM_cal_i_q(float *Input_i,float *Input_q,float *Ref_I,float *Ref_Q,int length,int Qm,float *EVM);

SRSRAN_API int phase_error_com(float *Rec_I, float *Rec_Q,float Phase_error,int Symbol_len);

SRSRAN_API int min_value_new(float *Input,int len,int *index);

SRSRAN_API int XW_Demod_and_Decode2(int Demod_flag,int Sync_point_best,int Symbol_len,int Type,int Band_i,float *Rec_I,float *Rec_Q,float *Ref_I,float *Ref_Q,
	                    float *add_freq_error_ifft_i,float *add_freq_error_ifft_q,int OP,double Phase_error,char *OutputBit,int *OutputBit_len,char *PUI,float *EVM);

// SRSRAN_API int XW_Demod_and_Decode(int Demod_flag,int Sync_point_best,int Symbol_len,int Type,int Band_i,float *Rec_I,float *Rec_Q,float *Ref_I,float *Ref_Q,
// 	float *add_freq_error_ifft_i,float *add_freq_error_ifft_q,int OP,double Phase_error,char *OutputBit,int *OutputBit_len,char *PUI);

SRSRAN_API int XW_Demod_and_Decode_new(int Demod_flag,int Sync_point_best,int Symbol_len,int Type,int Band_i,float *Rec_I,float *Rec_Q,float *Ref_I,float *Ref_Q,
	float *add_freq_error_ifft_i,float *add_freq_error_ifft_q,int OP,double Phase_error,char *OutputBit,int *OutputBit_len,char *PUI,float *EVM);

SRSRAN_API int XW_RXUL_PSCH_new(float *PSCH_i,float *PSCH_q,int length,int Type,int Band,char *PUI,char *OutputBit,int *OutputBit_len);

// SRSRAN_API int StarLink_TurboDeRateMatch_new_1(char *InputData, int length_input, int Delta_N, char *DeRateMatchOut, int *DeRateMatchOutLength);

// SRSRAN_API int StarLink_TurboDecode_WindowMaxLogMap_1(char *DeRateMatchOut, int DeRateMatchOut_Length, char *TurboOut);

#endif // SRSRAN_PRACH_H
