#ifndef _CEYEAR_52_ERRORCODE_HEADERFILE_
#define _CEYEAR_52_ERRORCODE_HEADERFILE_

#ifndef uint32_t
typedef unsigned int	uint32_t;
typedef unsigned char	uint8_t;

typedef unsigned short  uint16_t;

#endif

typedef struct tL1L2MessageHeader
{
    uint16_t nMessageType;
    uint16_t nMessageLen;
} L1L2MessageHdr, *PL1L2MessageHdr;

// Message header for Sfn and Slot Number Info
typedef struct tSfnSlot
{
    uint32_t nSFN:10;       // system frame number 0->1023
    uint32_t nSlot:9;       // slot number 0->319
    uint32_t nCarrierIdx:4; // carrier index, 0->15
    uint32_t nRsv:9;
} SFN_SlotStruct, *PSFN_SlotStruct;


typedef enum
{
	CY_GLOBAL_EC_OK					= 0,
	CY_GLOBAL_EC_FILELOCK 			= -1000,
	CY_GLOBAL_EC_HASHTABLE 			= -1500,
	CY_GLOBAL_EC_MULTILINK			= -2000,
	CY_GLOBAL_EC_SIGNAL				= -3000,
	CY_GLOBAL_EC_TARGET				= -4000,
	CY_GLOBAL_EC_TCPSERVER			= -5000,
	CY_GLOBAL_EC_TCPSERVERFUNC		= -6000,
	CY_GLOBAL_EC_VXI11				= -7000,
	CY_GLOBAL_EC_LOG				= -8000,

	CY_GLOBAL_EC_COMMANDEXEC		= -15000,
	CY_GLOBAL_EC_COMMANDPARSER		= -16000,
	
	CY_GLOBAL_EC_SCPICOMMON			= -19000,
	CY_GLOBAL_EC_SCPIGPRFGEN		= -20000,
	CY_GLOBAL_EC_SCPIGRPFMEAS		= -21000,
	CY_GLOBAL_EC_SCPIMMWAVEMEAS		= -22000,
	CY_GLOBAL_EC_SCPILINELOSS		= -23000,
	CY_GLOBAL_EC_SUBINSTRUMANAGER	= -24000,
	CY_GLOBAL_EC_SUBINSTRU			= -25000,

	CY_GLOBAL_EC_IFCOMMU			= -30000,
	CY_GLOBAL_EC_RFCOMMU			= -31000,
	CY_GLOBAL_EC_RFHEADCOMMU		= -32000,
	CY_GLOBAL_EC_PHYCOMMU			= -33000,

	CY_GLOBAL_EC_WAVEFORM			= -40000,
	CY_GLOBAL_EC_DEVICE				= -41000,
	CY_GLOBAL_EC_RFCOMP				= -42000,
	CY_GLOBAL_EC_RFHEADCOMP			= -43000,
	
	CY_GLOBAL_EC_DUT_GEN_TASK		= -49000,
	CY_GLOBAL_EC_DUT_MEAS_TASK		= -50000,
	CY_GLOBAL_EC_HEAPTIMER			= -51000,

}CY_GLOBAL_ERROR_CODE;

typedef enum
{
	CY_RELIABILITY_NO_ERROR							= 0,								// Measurement values available, no error detected
	CY_RELIABILITY_MEASUREMENT_TIMEOUT				= 1,								// The measurement has been stopped after the configured measurement timeout.Measurement results can be available. However, at least a part of the measurement provides only INValidresults or has not completed the full statistic count
	CY_RELIABILITY_CAPTURE_BUFFER_OVERFLOW			= 2,								// The measurement configuration results in a capture length that exceeds the available memory
	CY_RELIABILITY_INPUT_OVERDRIVEN					= 3,								// The accuracy of measurement results can be impaired because the input signal level was too high / too low
	CY_RELIABILITY_INPUT_UNDERDRIVEN				= 4,								// The accuracy of measurement results can be impaired because the input signal level was too high / too low

	CY_RELIABILITY_TRIGGER_TIMEOUT					= 6,								// The measurement could not be started or continued because no trigger event was detected
	CY_RELIABILITY_ACQUISTITION_ERROR				= 7,								// The R&S CMP200 could not properly decode the RF input signal
	CY_RELIABILITY_SYNC_ERROR						= 8,								// The R&S CMP200 could not synchronize to the RF input signal
	CY_RELIABILITY_UNCAL							= 9,								// Due to an inappropriate configuration of resolution bandwidth, video bandwidth or sweep time, the measurement results are not within the specified data sheet limits

	CY_RELIABILITY_REFERENCE_FREQUENCY_ERROR		= 15,								// The instrument has been configured to use an external reference signal. But the reference oscillator could not be phase-locked to the external signal (for example signal level too low, frequency out of range or reference signal not available at all)
	CY_RELIABILITY_RF_NOT_AVAILABLE					= 16,								// The measurement could not be started because the configured RF input path was not active
	CY_RELIABILITY_RF_LEVEL_NOT_SETTLED				= 17,								// The measurement could not be started because the R&S CMP200 was not yet ready to deliver stable results after a change of the input signal power / the input signal frequency
	CY_RELIABILITY_RF_FREQUENCY_NOT_SETTLED			= 18,								// The measurement could not be started because the R&S CMP200 was not yet ready to deliver stable results after a change of the input signal power / the input signal frequency
	CY_RELIABILITY_CALL_NOT_ESTABLISHED				= 19,								// For measurements: The measurement could not be started because no signaling connection to the DUT was established
	CY_RELIABILITY_CALL_TYPE_NOT_USABLE				= 20,								// For measurements: The measurement could not be started because the established signaling connection had wrong properties
	CY_RELIABILITY_CALL_LOST						= 21,								// For measurements: The measurement was interrupted because the signaling connection to the DUT was lost

	CY_RELIABILITY_MISSING_OPTION					= 23,								// An action cannot be executed due to a missing option. For GPRF generator: The ARB file cannot be played due to a missing option
	CY_RELIABILITY_INVALID_RF_SETTINGS				= 24,								// The desired RF TX level or RF RX reference level could not be applied
	CY_RELIABILITY_LEVEL_OVERRANGE					= 25,								// The RF TX level is in overrange. The signal quality can be degraded
	CY_RELIABILITY_RESOURCE_CONFLICT				= 26,								// The application could not be started or has been stopped due to a conflicting hardware resource or software option that is allocated by another application.Stop the application that has allocated the conflicting resources and try again

	CY_RELIABILITY_UNEXPECTED_PARAMETER_CHANGE		= 28,								// One or more measurement configuration parameters were changed while the measurement completed. The results were not obtained with these new parameter values. Repeat the measurement. This situation can only occur in remote single-shot mode
	CY_RELIABILITY_INVAILD_RF_FREQUENCY_SETTINGS	= 29,								// The desired RF TX frequency or RF RX frequency could not be applied
	CY_RELIABILITY_FILE_NOT_FOUND					= 30,								// The specified file could not be found

	CY_RELIABILITY_ARB_FILENUMOF_SAMPLES_INVALID	= 39,								// The ARB file contains a not supported number of samples. The number of samples must be greater than 999 and a multiple of four
	CY_RELIABILITY_ARB_FILE_CRC_ERROR				= 40,								// The cyclic redundancy check of the ARB file failed. The ARB file is corrupt and not reliable

	CY_RELIABILITY_ARB_HEADER_TAG_INVALID			= 42,								// The ARB file selected in the GPRF generator contains an invalid header tag
	CY_RELIABILITY_ARB_SEGMENT_OVERFLOW				= 43,								// The number of segments in the multi-segment ARB file is higher than the allowed maximum
	CY_RELIABILITY_ARB_FILE_NOT_FOUND				= 44,								// The selected ARB file could not be found
	CY_RELIABILITY_ARB_MEMORY_OVERFLOW				= 45,								// The ARB file length is greater than the available memory
	CY_RELIABILITY_ARB_SAMPLE_RATE_OUT_OF_RANGE		= 46,								// The clock rate of the ARB file is either too high or too low
	CY_RELIABILITY_ARB_CYCLES_OUT_OF_RANGE			= 47,								// The repetition mode equals "Single Shot" and the playback length is too long.Reduce the playback length or set the repetition mode to "Continuous".<Length> = (<Cycles> * <Samples> + <Additional Samples>) / <Clock Rate>

	CY_RELIABILITY_CONNECTION_ERROR					= 52,								// A connection setup failed or a connection was lost

	CY_RELIABILITY_INVALID_CONRRECTION_DATA			= 61,								// The installed software version is incompatible with the calibration data of the instrument (level correction data)
	CY_RELIABILITY_MISSING_CORRECTION_DATA			= 62,								// The level correction data are missing. The instrument must be calibrated by Rohde & Schwarz
	CY_RELIABILITY_INVALID_IQ_ALIGNMENT				= 63,								// The IQ correction data are invalid. Perform a self-alignment with mode "IQ", as described in the R&S CMP200 base unit manual

	CY_RELIABILITY_WRONG_STANDARD					= 70,								// The standard of the measured signal does not match the configured standard
	CY_RELIABILITY_WRONG_BANDWIDTH					= 71,								// The bandwidth of the measured signal does not match the configured bandwidth
	CY_RELIABILITY_WRONG_BURST_TYPE					= 72,								// The burst type of the measured signal does not match the configured burst type
	CY_RELIABILITY_MIMO_SIGNAL_DETECTED				= 73,								// The measurement expects a SISO signal and detected a MIMO signal. Use a MIMO receive mode to measure this signal
	CY_RELIABILITY_MORE_STREAMS_THAN_ANTENNAS		= 74,								// The measured signal has more streams than expected due to the configured number of antennas. Increase the configured number of antennas to measure this signal
	CY_RELIABILITY_MATRIX_INVERSION_FAILED			= 75,								// The inversion of the channel matrix failed for a MIMO measurement. Check that the antennas are connected correctly to the instrument
	CY_RELIABILITY_SIG_CRC_FAILED					= 76,								// The cyclic redundancy check of a SIGNAL field failed
	CY_RELIABILITY_PARITY_CHECK_FAILED				= 77,								// The parity check of a SIGNAL field failed
	CY_RELIABILITY_BURSTS_NOT_IDENTICAL				= 78,								// In training mode for composite MIMO measurements, at least some symbols of sequential bursts need to be identical to be used as training data. Setting a fix scrambler initialization can solve this problem
	CY_RELIABILITY_WRONG_MODULATION					= 79,								// The modulation type of the measured signal does not match the configured modulation type

	CY_RELIABILITY_OCXO_OVEN_TEMPEARATURE_TOO_LOW	= 93,								// The accuracy of measurement results can be impaired because the oven-controlled crystal oscillator has a too low temperature. After switching-on the instrument, the OCXO requires a warm-up phase to reach its operating temperature

	CY_RELIABILITY_FIRMWARE_ERROR					= 101,								// Indicates a firmware or software error. If you encounter this error for the first time, restart the instrument
	CY_RELIABILITY_UNIDENTIFIED_ERROR				= 102,								// Indicates an error not covered by other reliability values. For troubleshooting, follow the steps described for "101 (firmware error)"
	CY_RELIABILITY_PARAMETER_ERROR					= 103,								// Indicates that the measurement could not be performed due to internal conflicting parameter settings
	CY_RELIABILITY_NOT_FUNCTIONAL					= 104,								// The application could not be started with the configured parameter set
	
}CY_RELIABILITY_INDICATOR_CODE;

/*设置错误码*/
#define AFXPOSTERRORCODE(nErrorCode) CErrorCode::PostErrorCode(__FILE__,__LINE__,__FUNCTION__,nErrorCode)

class CErrorCode
{

public:
	CErrorCode ();     
	~CErrorCode();

	static int PostErrorCode(const char* pszFileName,int nLineNo,const char* pszFuntion,int nErrorCode);
	
private:
	
public:
	
private:
	
};

#endif