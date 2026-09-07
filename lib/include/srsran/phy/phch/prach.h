/**
 * Copyright 2013-2021 Software Radio Systems Limited
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
 *  File:         prach.h
 *
 *  Description:  Physical random access channel.
 *
 *  Reference:    3GPP TS 36.211 version 10.0.0 Release 10 Sec. 5.7
 *****************************************************************************/

#ifndef SRSRAN_PRACH_H
#define SRSRAN_PRACH_H

#include "srsran/config.h"
#include "srsran/phy/common/phy_common.h"
#include "srsran/phy/common/phy_common_nr.h"
#include "srsran/phy/dft/dft.h"
#include <complex.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <fftw3.h>

#define SRSRAN_PRACH_MAX_LEN (2 * 24576 + 21024) // Maximum Tcp + Tseq

// Long PRACH ZC sequence sequence length
#define SRSRAN_PRACH_N_ZC_LONG 839

// Short PRACH ZC sequence sequence length
#define SRSRAN_PRACH_N_ZC_SHORT 139

/** Generation and detection of RACH signals for uplink.
 *  Currently only supports preamble formats 0-3.
 *  Does not currently support high speed flag.
 *  Based on 3GPP TS 36.211 version 10.7.0 Release 10.
 */

typedef struct {
  int   idx;
  float factor;
  cf_t  phase_array[2 * SRSRAN_PRACH_N_ZC_LONG];
} srsran_prach_cancellation_t;

typedef struct SRSRAN_API {
  // Parameters from higher layers (extracted from SIB2)
  bool     is_nr;
  uint32_t config_idx;
  uint32_t f;            // preamble format
  uint32_t rsi;          // rootSequenceIndex
  bool     hs;           // highSpeedFlag
  uint32_t zczc;         // zeroCorrelationZoneConfig
  uint32_t N_ifft_ul;    // IFFT size for uplink
  uint32_t N_ifft_prach; // IFFT size for PRACH generation

  uint32_t max_N_ifft_ul;

  // Working parameters
  uint32_t N_zc;  // PRACH sequence length
  uint32_t N_cs;  // Cyclic shift size
  uint32_t N_seq; // Preamble length
  float    T_seq; // Preamble length in seconds
  float    T_tot; // Total sequence length in seconds
  uint32_t N_cp;  // Cyclic prefix length

  // Generated tables
  cf_t     seqs[64][SRSRAN_PRACH_N_ZC_LONG];     // Our set of 64 preamble sequences
  cf_t     dft_seqs[64][SRSRAN_PRACH_N_ZC_LONG]; // DFT-precoded seqs
  uint64_t dft_gen_bitmap;    // Bitmap where each bit Indicates if the dft has been generated for sequence i.
  uint32_t root_seqs_idx[64]; // Indices of root seqs in seqs table
  uint32_t N_roots;           // Number of root sequences used in this configuration
  cf_t*    td_signals[64];
  // Containers
  cf_t*  ifft_in;
  cf_t*  ifft_out;
  cf_t*  prach_bins;
  cf_t*  corr_spec;
  float* corr;

  // XW buffer
  float*   XW_prach_buffer_i;//����buffer
  float*   XW_prach_buffer_q;
  int         band;
  int         cc;
  int         sub_cc;
  int         cc1; //PRACH format1
  float     Pa;
  float     Fa;
  float     Ta;
  int         rach_TTI;
  int         XW_source_len;
  int         prach_type;//0:������  1��������
  char     Pdu[4000];
  int         Pdu_len;
  int         crc_flag;
  cf_t*    XW_source_prach;//�洢ԭʼprach�����ź�,������Ϊ16*30
  cf_t*    XW_prach_samples;//����buff�洢λ��ǰ60ms���棬����Ϊ���ݴ�������buffer
  cf_t*    XW_prach_samples_fream;
  int      power_flag;


  // PRACH IFFT
  srsran_dft_plan_t fft;
  srsran_dft_plan_t ifft;

  // ZC-sequence FFT and IFFT
  srsran_dft_plan_t zc_fft;
  srsran_dft_plan_t zc_ifft;

  cf_t* signal_fft;
  float detect_factor;

  uint8_t                     ue_category;
  uint32_t                    deadzone;
  float                       peak_values[65];
  uint32_t                    peak_offsets[65];
  uint32_t                    num_ra_preambles;
  bool                        successive_cancellation;
  bool                        freq_domain_offset_calc;
  srsran_tdd_config_t         tdd_config;
  uint32_t                    current_prach_idx;
  cf_t*                       cross;
  cf_t*                       corr_freq;
  srsran_prach_cancellation_t prach_cancel;
  cf_t                        sub[839 * 2];
  float                       phase[839];

  //============================ ul fft_para =============================//
  float *CommonMemory;
  int CommonMemory_len;
  double time_forward_set;

  double *fftin;
	double *fftout;
  //=============== Freq cal ==============//
  int Nfft_16k, Nfft_80k;
  double *fftin_freq;
	double *fftout_freq;
  double *fftin_16k;
	double *fftout_16k;
	double *fftin_80k;
	double *fftout_80k;
  fftwf_plan *p_forward_16k;
	fftwf_plan *p_back_16k;
	fftwf_plan *p_forward_80k;
	fftwf_plan *p_back_80k;
  
	//================ PSCH ================//
	//===== PSCH52 ul =====//	
	double *fftin_52;
	double *fftout_52;
  double *fftin_38400;
  double *fftout_38400;
  double *fftin_38400_80;
  double *fftout_38400_80;
  double *fftin_17280_15360;
  double *fftout_17280_15360;

  fftwf_plan *p_forward_38400;
	fftwf_plan *p_back_38400;
  fftwf_plan *p_forward_38400_80;
	fftwf_plan *p_back_38400_80;
	fftwf_plan *p_forward_17280_15360;
	fftwf_plan *p_back_17280_15360;
  //==== psch51 ul ====//
	double *fftin_51;
	double *fftout_51;
  double *fftin_51_38400;
  double *fftout_51_38400;
  double *fftin_51_38400_80;
  double *fftout_51_38400_80;
  double *fftin_51_9600_7680;
  double *fftout_51_9600_7680;

  fftwf_plan *p_forward_51_38400;
	fftwf_plan *p_back_51_38400;
  fftwf_plan *p_forward_51_38400_80;
	fftwf_plan *p_back_51_38400_80;
	fftwf_plan *p_forward_9600_7680;
	fftwf_plan *p_back_9600_7680;
  //======== PSCH12 =========//
	double *fftin_12;
	double *fftout_12;
	fftwf_plan *p_forward_7680;
	fftwf_plan *p_back_7680;
	fftwf_plan *p_forward_7680_80;
	fftwf_plan *p_back_7680_80;
	fftwf_plan *p_forward_3456_3072;
	fftwf_plan *p_back_3456_3072;
  //==== PSCH11 ====//
	double *fftin_11;
	double *fftout_11;
	fftwf_plan *p_forward_1920_1536;
	fftwf_plan *p_back_1920_1536;


  //=== 11520*60 FFT/IFFT ===//
  double *fftin_11520_60ms;
	double *fftout_11520_60ms;
  fftwf_plan *p_forward_11520_60ms;
	fftwf_plan *p_back_11520_60ms;
  fftwf_plan *p_forward_11520_60ms_lxg;
	fftwf_plan *p_back_11520_60ms_lxg;


  //============================ dl fft_para =============================//
  	//================ PSCH ================//
	//======== PSCH52 ========//	
  float *CommonMemory_dl;
  int CommonMemory_dl_len;
	double *fftin_dl;
	double *fftout_dl;
  
  //PSCH5_2
  double *fftin_7700;
	double *fftout_7700;
  fftwf_plan *p_forward_7700;
	fftwf_plan *p_back_7700;

  //PSCH5_1
	double *fftin_3860;
	double *fftout_3860;
	fftwf_plan *p_forward_3860;
	fftwf_plan *p_back_3860;

	//PSCH1_2
	double *fftin_7780;
	double *fftout_7780;
	fftwf_plan *p_forward_7780;
	fftwf_plan *p_back_7780;

	//PSCH1_1
	double *fftin_3940;
	double *fftout_3940;
	fftwf_plan *p_forward_3940;
	fftwf_plan *p_back_3940;

  //filter
  double *fftin_9600;
	double *fftout_9600;
	fftwf_plan *p_forward_9600;
	fftwf_plan *p_back_9600;
  fftwf_plan *p_forward_1920;
	fftwf_plan *p_back_1920;

  //IFFT
  fftwf_plan *p_forward_691200;
	fftwf_plan *p_back_691200;

  fftwf_plan *p_forward_691200_lxg;
	fftwf_plan *p_back_691200_lxg;

  fftwf_plan *p_forward_138240;
	fftwf_plan *p_back_138240;


  //
	struct
	{
		int demod_flag;
		int sync_point;
		int best_sync_point;
		float FOE;
		float phase_error; 
		float EVM;

		struct  
		{
			int demod_flag;
			int sync_point;
			int best_sync_point;
			float FOE;
			float phase_error; 
			float EVM;
			int slot_flag[5];
		}multi_cc[16];
	}demod_results;
  
  struct
	{
		int demod_flag;
		int sync_point;
		int best_sync_point;
    int sync_cnt;
		float FOE;
		float phase_error; 
		float EVM;
    

		struct  
		{
			int demod_flag;
			int sync_point;
			int best_sync_point;
			float FOE;
			float phase_error; 
			float EVM;
		}multi_cc[16];
	}demod_results_slot[5];
} srsran_prach_t;

typedef struct SRSRAN_API {
  int      nof_sf;
  uint32_t sf[5];
} srsran_prach_sf_config_t;

///@brief Maximum number of subframe number candidates for PRACH NR configuration
#define PRACH_NR_CFG_MAX_NOF_SF 10

/**
 * @brief PRACH configuration for NR as described in TS 38.211 Tables 6.3.3.2-2, 6.3.3.2-3 and 6.3.3.2-4
 */
typedef struct {
  uint32_t preamble_format;
  uint32_t x;
  uint32_t y;
  uint32_t subframe_number[PRACH_NR_CFG_MAX_NOF_SF];
  uint32_t nof_subframe_number;
  uint32_t starting_symbol; // subframe number
} prach_nr_config_t;

typedef enum SRSRAN_API {
  SRSRAN_PRACH_SFN_EVEN = 0,
  SRSRAN_PRACH_SFN_ANY,
} srsran_prach_sfn_t;

typedef struct {
  bool                is_nr; // Set to true if NR
  uint32_t            config_idx;
  uint32_t            root_seq_idx;
  uint32_t            zero_corr_zone;
  uint32_t            freq_offset;
  uint32_t            num_ra_preambles;
  bool                hs_flag;
  srsran_tdd_config_t tdd_config;
  bool                enable_successive_cancellation;
  bool                enable_freq_domain_offset_calc;
  //debug20240124
  int band;
  int cc;
  int slot;
  int cc1;
} srsran_prach_cfg_t;

typedef struct SRSRAN_API {
  uint32_t f;
  uint32_t t0;
  uint32_t t1;
  uint32_t t2;
} srsran_prach_tdd_loc_t;

typedef struct SRSRAN_API {
  uint32_t               nof_elems;
  srsran_prach_tdd_loc_t elems[6];
} srsran_prach_tdd_loc_table_t;

SRSRAN_API uint32_t srsran_prach_get_preamble_format(uint32_t config_idx);

SRSRAN_API srsran_prach_sfn_t srsran_prach_get_sfn(uint32_t config_idx);

SRSRAN_API bool srsran_prach_tti_opportunity(srsran_prach_t* p, uint32_t current_tti, int allowed_subframe);

SRSRAN_API bool
srsran_prach_tti_opportunity_config_fdd(uint32_t config_idx, uint32_t current_tti, int allowed_subframe);

SRSRAN_API bool srsran_prach_in_window_config_fdd(uint32_t config_idx, uint32_t current_tti, int allowed_subframe);

SRSRAN_API bool srsran_prach_tti_opportunity_config_tdd(uint32_t  config_idx,
                                                        uint32_t  tdd_ul_dl_config,
                                                        uint32_t  current_tti,
                                                        uint32_t* prach_idx);

SRSRAN_API const prach_nr_config_t* srsran_prach_nr_get_cfg_fr1_paired(uint32_t config_idx);

SRSRAN_API bool srsran_prach_nr_tti_opportunity_fr1_paired(uint32_t config_idx, uint32_t current_tti);

SRSRAN_API uint32_t srsran_prach_nr_start_symbol_fr1_paired(uint32_t config_idx);

SRSRAN_API const prach_nr_config_t* srsran_prach_nr_get_cfg_fr1_unpaired(uint32_t config_idx);

SRSRAN_API bool srsran_prach_nr_tti_opportunity_fr1_unpaired(uint32_t config_idx, uint32_t current_tti);

SRSRAN_API uint32_t srsran_prach_nr_start_symbol(uint32_t config_idx, srsran_duplex_mode_t duplex_mode);

SRSRAN_API uint32_t srsran_prach_f_ra_tdd(uint32_t config_idx,
                                          uint32_t tdd_ul_dl_config,
                                          uint32_t current_tti,
                                          uint32_t prach_idx,
                                          uint32_t prach_offset,
                                          uint32_t n_rb_ul);

SRSRAN_API uint32_t srsran_prach_f_id_tdd(uint32_t config_idx, uint32_t tdd_ul_dl_config, uint32_t prach_idx);

SRSRAN_API uint32_t srsran_prach_nof_f_idx_tdd(uint32_t config_idx, uint32_t tdd_ul_dl_config);

SRSRAN_API void srsran_prach_sf_config(uint32_t config_idx, srsran_prach_sf_config_t* sf_config);

SRSRAN_API int srsran_prach_init(srsran_prach_t* p, uint32_t max_N_ifft_ul);

SRSRAN_API int srsran_prach_set_cfg(srsran_prach_t* p, srsran_prach_cfg_t* cfg, uint32_t nof_prb);

SRSRAN_API int srsran_prach_gen(srsran_prach_t* p, uint32_t seq_index, uint32_t freq_offset, cf_t* signal);

SRSRAN_API int srsran_prach_detect(srsran_prach_t* p,
                                   uint32_t        freq_offset,
                                   cf_t*           signal,
                                   uint32_t        sig_len,
                                   uint32_t*       indices,
                                   uint32_t*       ind_len);

SRSRAN_API int srsran_prach_detect_offset(srsran_prach_t* p,
                                          uint32_t        freq_offset,
                                          cf_t*           signal,
                                          uint32_t        sig_len,
                                          uint32_t*       indices,
                                          float*          t_offsets,
                                          float*          peak_to_avg,
                                          uint32_t*       ind_len);

SRSRAN_API void srsran_prach_set_detect_factor(srsran_prach_t* p, float factor);

SRSRAN_API int srsran_prach_free(srsran_prach_t* p);

SRSRAN_API int srsran_prach_print_seqs(srsran_prach_t* p);

SRSRAN_API int srsran_prach_process(srsran_prach_t* p,
                                    cf_t*           signal,
                                    uint32_t*       indices,
                                    float*          t_offsets,
                                    float*          peak_to_avg,
                                    uint32_t*       n_indices,
                                    int             cancellation_idx,
                                    uint32_t        begin,
                                    uint32_t        sig_len);

#endif // SRSRAN_PRACH_H
