#ifndef SX1280_H_
#define SX1280_H_

#include <cstdint>
#include <functional>

class SX1280 {
public:
    enum {
        REG_LR_FIRMWARE_VERSION_MSB = 0x0153,
        REG_LR_PACKETPARAMS = 0x0903,
        REG_LR_PAYLOADLENGTH = 0x0901,
        REG_LR_CRCSEEDBASEADDR = 0x09C8,
        REG_LR_CRCPOLYBASEADDR = 0x09C6,
        REG_LR_WHITSEEDBASEADDR = 0x09C5,
        REG_LR_RANGINGIDCHECKLENGTH = 0x0931,
        REG_LR_DEVICERANGINGADDR = 0x0916,
        REG_LR_REQUESTRANGINGADDR = 0x0912,
        REG_LR_RANGINGRESULTCONFIG = 0x0924,
        REG_LR_RANGINGRESULTBASEADDR = 0x0961,
        REG_LR_RANGINGRESULTSFREEZE = 0x097F,
        REG_LR_RANGINGRERXTXDELAYCAL = 0x092C,
        REG_LR_RANGINGFILTERWINDOWSIZE = 0x091E,
        REG_LR_RANGINGRESULTCLEARREG = 0x0923,
        REG_LR_SYNCWORDBASEADDRESS1 = 0x09CE,
        REG_LR_SYNCWORDBASEADDRESS2 = 0x09D3,
        REG_LR_SYNCWORDBASEADDRESS3 = 0x09D8,
        REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB = 0x0954,
        REG_LR_SYNCWORDTOLERANCE = 0x09CD,
        REG_LR_PREAMBLELENGTH = 0x09C1,
        REG_LR_BLE_ACCESS_ADDRESS = 0x09CF,
    };

    enum {
        REG_LR_ESTIMATED_FREQUENCY_ERROR_MASK = 0x0FFFFF,
    };

    enum RadioCommand {
        RADIO_GET_STATUS = 0xC0,
        RADIO_WRITE_REGISTER = 0x18,
        RADIO_READ_REGISTER = 0x19,
        RADIO_WRITE_BUFFER = 0x1A,
        RADIO_READ_BUFFER = 0x1B,
        RADIO_SET_SLEEP = 0x84,
        RADIO_SET_STANDBY = 0x80,
        RADIO_SET_FS = 0xC1,
        RADIO_SET_TX = 0x83,
        RADIO_SET_RX = 0x82,
        RADIO_SET_RXDUTYCYCLE = 0x94,
        RADIO_SET_CAD = 0xC5,
        RADIO_SET_TXCONTINUOUSWAVE = 0xD1,
        RADIO_SET_TXCONTINUOUSPREAMBLE = 0xD2,
        RADIO_SET_PACKETTYPE = 0x8A,
        RADIO_GET_PACKETTYPE = 0x03,
        RADIO_SET_RFFREQUENCY = 0x86,
        RADIO_SET_TXPARAMS = 0x8E,
        RADIO_SET_CADPARAMS = 0x88,
        RADIO_SET_BUFFERBASEADDRESS = 0x8F,
        RADIO_SET_MODULATIONPARAMS = 0x8B,
        RADIO_SET_PACKETPARAMS = 0x8C,
        RADIO_GET_RXBUFFERSTATUS = 0x17,
        RADIO_GET_PACKETSTATUS = 0x1D,
        RADIO_GET_RSSIINST = 0x1F,
        RADIO_SET_DIOIRQPARAMS = 0x8D,
        RADIO_GET_IRQSTATUS = 0x15,
        RADIO_CLR_IRQSTATUS = 0x97,
        RADIO_CALIBRATE = 0x89,
        RADIO_SET_REGULATORMODE = 0x96,
        RADIO_SET_SAVECONTEXT = 0xD5,
        RADIO_SET_AUTOTX = 0x98,
        RADIO_SET_AUTOFS = 0x9E,
        RADIO_SET_LONGPREAMBLE = 0x9B,
        RADIO_SET_UARTSPEED = 0x9D,
        RADIO_SET_RANGING_ROLE = 0xA3,
    };

    enum IrqRangingCode {
        IRQ_RANGING_SLAVE_ERROR_CODE = 0x00,
        IRQ_RANGING_SLAVE_VALID_CODE,
        IRQ_RANGING_MASTER_ERROR_CODE,
        IRQ_RANGING_MASTER_VALID_CODE,
    };

    enum IrqErrorCode {
        IRQ_HEADER_ERROR_CODE = 0x00,
        IRQ_SYNCWORD_ERROR_CODE,
        IRQ_CRC_ERROR_CODE,
        IRQ_RANGING_ON_LORA_ERROR_CODE,
    };

    enum IrqValidCode {
        IRQ_HEADER_VALID_CODE = 0x00,
        IRQ_SYNCWORD_VALID_CODE,
    };

    enum RadioStates {
        RF_IDLE = 0x00, //!< The radio is idle
        RF_RX_RUNNING, //!< The radio is in reception state
        RF_TX_RUNNING, //!< The radio is in transmission state
        RF_CAD, //!< The radio is doing channel activity detection
    };

    enum RadioCommandStatus {
        CMD_SUCCESS = 0x01,
        CMD_DATA_AVAILABLE = 0x02,
        CMD_TIMEOUT = 0x03,
        CMD_ERROR = 0x04,
        CMD_FAIL = 0x05,
        CMD_TX_DONE = 0x06,
    };

    enum RadioOperatingModes {
        MODE_SLEEP = 0x00, //! The radio is in sleep mode
        MODE_CALIBRATION, //! The radio is in calibration mode
        MODE_STDBY_RC, //! The radio is in standby mode with RC oscillator
        MODE_STDBY_XOSC, //! The radio is in standby mode with XOSC oscillator
        MODE_FS, //! The radio is in frequency synthesis mode
        MODE_RX, //! The radio is in receive mode
        MODE_TX, //! The radio is in transmit mode
        MODE_CAD //! The radio is in channel activity detection mode
    };

    enum RadioStandbyModes {
        STDBY_RC = 0x00,
        STDBY_XOSC = 0x01,
    };

    enum RadioRegulatorModes {
        USE_LDO = 0x00, //! Use LDO (default value)
        USE_DCDC = 0x01, //! Use DCDC
    };

    enum RadioPacketTypes {
        PACKET_TYPE_GFSK = 0x00,
        PACKET_TYPE_LORA,
        PACKET_TYPE_RANGING,
        PACKET_TYPE_FLRC,
        PACKET_TYPE_BLE,
        PACKET_TYPE_NONE = 0x0F,
    };

    enum RadioRampTimes {
        RADIO_RAMP_02_US = 0x00,
        RADIO_RAMP_04_US = 0x20,
        RADIO_RAMP_06_US = 0x40,
        RADIO_RAMP_08_US = 0x60,
        RADIO_RAMP_10_US = 0x80,
        RADIO_RAMP_12_US = 0xA0,
        RADIO_RAMP_16_US = 0xC0,
        RADIO_RAMP_20_US = 0xE0,
    };

    enum RadioLoRaCadSymbols {
        LORA_CAD_01_SYMBOL = 0x00,
        LORA_CAD_02_SYMBOLS = 0x20,
        LORA_CAD_04_SYMBOLS = 0x40,
        LORA_CAD_08_SYMBOLS = 0x60,
        LORA_CAD_16_SYMBOLS = 0x80,
    };

    enum RadioGfskBleBitrates {
        GFSK_BLE_BR_2_000_BW_2_4 = 0x04,
        GFSK_BLE_BR_1_600_BW_2_4 = 0x28,
        GFSK_BLE_BR_1_000_BW_2_4 = 0x4C,
        GFSK_BLE_BR_1_000_BW_1_2 = 0x45,
        GFSK_BLE_BR_0_800_BW_2_4 = 0x70,
        GFSK_BLE_BR_0_800_BW_1_2 = 0x69,
        GFSK_BLE_BR_0_500_BW_1_2 = 0x8D,
        GFSK_BLE_BR_0_500_BW_0_6 = 0x86,
        GFSK_BLE_BR_0_400_BW_1_2 = 0xB1,
        GFSK_BLE_BR_0_400_BW_0_6 = 0xAA,
        GFSK_BLE_BR_0_250_BW_0_6 = 0xCE,
        GFSK_BLE_BR_0_250_BW_0_3 = 0xC7,
        GFSK_BLE_BR_0_125_BW_0_3 = 0xEF,
    };

    enum RadioGfskBleModIndexes {
        GFSK_BLE_MOD_IND_0_35 = 0,
        GFSK_BLE_MOD_IND_0_50 = 1,
        GFSK_BLE_MOD_IND_0_75 = 2,
        GFSK_BLE_MOD_IND_1_00 = 3,
        GFSK_BLE_MOD_IND_1_25 = 4,
        GFSK_BLE_MOD_IND_1_50 = 5,
        GFSK_BLE_MOD_IND_1_75 = 6,
        GFSK_BLE_MOD_IND_2_00 = 7,
        GFSK_BLE_MOD_IND_2_25 = 8,
        GFSK_BLE_MOD_IND_2_50 = 9,
        GFSK_BLE_MOD_IND_2_75 = 10,
        GFSK_BLE_MOD_IND_3_00 = 11,
        GFSK_BLE_MOD_IND_3_25 = 12,
        GFSK_BLE_MOD_IND_3_50 = 13,
        GFSK_BLE_MOD_IND_3_75 = 14,
        GFSK_BLE_MOD_IND_4_00 = 15,
    };

    enum RadioFlrcBitrates {
        FLRC_BR_1_300_BW_1_2 = 0x45,
        FLRC_BR_1_040_BW_1_2 = 0x69,
        FLRC_BR_0_650_BW_0_6 = 0x86,
        FLRC_BR_0_520_BW_0_6 = 0xAA,
        FLRC_BR_0_325_BW_0_3 = 0xC7,
        FLRC_BR_0_260_BW_0_3 = 0xEB,
    };

    enum RadioFlrcCodingRates {
        FLRC_CR_1_2 = 0x00,
        FLRC_CR_3_4 = 0x02,
        FLRC_CR_1_0 = 0x04,
    };

    enum RadioModShapings {
        RADIO_MOD_SHAPING_BT_OFF = 0x00, //! No filtering
        RADIO_MOD_SHAPING_BT_1_0 = 0x10,
        RADIO_MOD_SHAPING_BT_0_5 = 0x20,
    };

    enum RadioLoRaSpreadingFactors {
        LORA_SF5 = 0x50,
        LORA_SF6 = 0x60,
        LORA_SF7 = 0x70,
        LORA_SF8 = 0x80,
        LORA_SF9 = 0x90,
        LORA_SF10 = 0xA0,
        LORA_SF11 = 0xB0,
        LORA_SF12 = 0xC0,
    };

    enum RadioLoRaBandwidths {
        LORA_BW_0200 = 0x34,
        LORA_BW_0400 = 0x26,
        LORA_BW_0800 = 0x18,
        LORA_BW_1600 = 0x0A,
    };

    enum RadioLoRaCodingRates {
        LORA_CR_4_5 = 0x01,
        LORA_CR_4_6 = 0x02,
        LORA_CR_4_7 = 0x03,
        LORA_CR_4_8 = 0x04,
        LORA_CR_LI_4_5 = 0x05,
        LORA_CR_LI_4_6 = 0x06,
        LORA_CR_LI_4_7 = 0x07,
    };

    enum RadioPreambleLengths {
        PREAMBLE_LENGTH_04_BITS = 0x00, //!< Preamble length: 04 bits
        PREAMBLE_LENGTH_08_BITS = 0x10, //!< Preamble length: 08 bits
        PREAMBLE_LENGTH_12_BITS = 0x20, //!< Preamble length: 12 bits
        PREAMBLE_LENGTH_16_BITS = 0x30, //!< Preamble length: 16 bits
        PREAMBLE_LENGTH_20_BITS = 0x40, //!< Preamble length: 20 bits
        PREAMBLE_LENGTH_24_BITS = 0x50, //!< Preamble length: 24 bits
        PREAMBLE_LENGTH_28_BITS = 0x60, //!< Preamble length: 28 bits
        PREAMBLE_LENGTH_32_BITS = 0x70, //!< Preamble length: 32 bits
    };

    enum RadioFlrcSyncWordLengths {
        FLRC_NO_SYNCWORD = 0x00,
        FLRC_SYNCWORD_LENGTH_4_BYTE = 0x04,
    };

    enum RadioSyncWordLengths {
        GFSK_SYNCWORD_LENGTH_1_BYTE = 0x00, //!< Sync word length: 1 byte
        GFSK_SYNCWORD_LENGTH_2_BYTE = 0x02, //!< Sync word length: 2 bytes
        GFSK_SYNCWORD_LENGTH_3_BYTE = 0x04, //!< Sync word length: 3 bytes
        GFSK_SYNCWORD_LENGTH_4_BYTE = 0x06, //!< Sync word length: 4 bytes
        GFSK_SYNCWORD_LENGTH_5_BYTE = 0x08, //!< Sync word length: 5 bytes
    };

    enum RadioSyncWordRxMatchs {
        RADIO_RX_MATCH_SYNCWORD_OFF = 0x00, //!< No correlator turned on, i.e. do not search for SyncWord
        RADIO_RX_MATCH_SYNCWORD_1 = 0x10,
        RADIO_RX_MATCH_SYNCWORD_2 = 0x20,
        RADIO_RX_MATCH_SYNCWORD_1_2 = 0x30,
        RADIO_RX_MATCH_SYNCWORD_3 = 0x40,
        RADIO_RX_MATCH_SYNCWORD_1_3 = 0x50,
        RADIO_RX_MATCH_SYNCWORD_2_3 = 0x60,
        RADIO_RX_MATCH_SYNCWORD_1_2_3 = 0x70,
    };

    enum RadioPacketLengthModes {
        RADIO_PACKET_FIXED_LENGTH = 0x00, //!< The packet is known on both sides, no header included in the packet
        RADIO_PACKET_VARIABLE_LENGTH = 0x20, //!< The packet is on variable size, header included
    };

    enum RadioCrcTypes {
        RADIO_CRC_OFF = 0x00, //!< No CRC in use
        RADIO_CRC_1_BYTES = 0x10,
        RADIO_CRC_2_BYTES = 0x20,
        RADIO_CRC_3_BYTES = 0x30,
    };

    enum RadioWhiteningModes {
        RADIO_WHITENING_ON = 0x00,
        RADIO_WHITENING_OFF = 0x08,
    };

    enum RadioLoRaPacketLengthsModes {
        LORA_PACKET_VARIABLE_LENGTH = 0x00, //!< The packet is on variable size, header included
        LORA_PACKET_FIXED_LENGTH = 0x80, //!< The packet is known on both sides, no header included in the packet
        LORA_PACKET_EXPLICIT = LORA_PACKET_VARIABLE_LENGTH,
        LORA_PACKET_IMPLICIT = LORA_PACKET_FIXED_LENGTH,
    };

    enum RadioLoRaCrcModes {
        LORA_CRC_ON = 0x20, //!< CRC activated
        LORA_CRC_OFF = 0x00, //!< CRC not used
    };

    enum RadioLoRaIQModes {
        LORA_IQ_NORMAL = 0x40,
        LORA_IQ_INVERTED = 0x00,
    };

    enum RadioRangingIdCheckLengths {
        RANGING_IDCHECK_LENGTH_08_BITS = 0x00,
        RANGING_IDCHECK_LENGTH_16_BITS,
        RANGING_IDCHECK_LENGTH_24_BITS,
        RANGING_IDCHECK_LENGTH_32_BITS,
    };

    enum RadioRangingResultTypes {
        RANGING_RESULT_RAW = 0x00,
        RANGING_RESULT_AVERAGED = 0x01,
        RANGING_RESULT_DEBIASED = 0x02,
        RANGING_RESULT_FILTERED = 0x03,
    };

    enum RadioBleConnectionStates {
        BLE_MASTER_SLAVE = 0x00,
        BLE_ADVERTISER = 0x20,
        BLE_TX_TEST_MODE = 0x40,
        BLE_RX_TEST_MODE = 0x60,
        BLE_RXTX_TEST_MODE = 0x80,
    };

    enum RadioBleCrcTypes {
        BLE_CRC_OFF = 0x00,
        BLE_CRC_3B = 0x10,
    };

    enum RadioBleTestPayloads {
        BLE_PRBS_9 = 0x00, //!< Pseudo Random Binary Sequence based on 9th degree polynomial
        BLE_PRBS_15 = 0x0C, //!< Pseudo Random Binary Sequence based on 15th degree polynomial
        BLE_EYELONG_1_0 = 0x04, //!< Repeated '11110000' sequence
        BLE_EYELONG_0_1 = 0x18, //!< Repeated '00001111' sequence
        BLE_EYESHORT_1_0 = 0x08, //!< Repeated '10101010' sequence
        BLE_EYESHORT_0_1 = 0x1C, //!< Repeated '01010101' sequence
        BLE_ALL_1 = 0x10, //!< Repeated '11111111' sequence
        BLE_ALL_0 = 0x14, //!< Repeated '00000000' sequence
    };

    enum RadioIrqMasks {
        IRQ_RADIO_NONE = 0x0000,
        IRQ_TX_DONE = 0x0001,
        IRQ_RX_DONE = 0x0002,
        IRQ_SYNCWORD_VALID = 0x0004,
        IRQ_SYNCWORD_ERROR = 0x0008,
        IRQ_HEADER_VALID = 0x0010,
        IRQ_HEADER_ERROR = 0x0020,
        IRQ_CRC_ERROR = 0x0040,
        IRQ_RANGING_SLAVE_RESPONSE_DONE = 0x0080,
        IRQ_RANGING_SLAVE_REQUEST_DISCARDED = 0x0100,
        IRQ_RANGING_MASTER_RESULT_VALID = 0x0200,
        IRQ_RANGING_MASTER_TIMEOUT = 0x0400,
        IRQ_RANGING_SLAVE_REQUEST_VALID = 0x0800,
        IRQ_CAD_DONE = 0x1000,
        IRQ_CAD_DETECTED = 0x2000,
        IRQ_RX_TX_TIMEOUT = 0x4000,
        IRQ_PREAMBLE_DETECTED = 0x8000,
        IRQ_RADIO_ALL = 0xFFFF,
    };

    enum RadioDios {
        RADIO_DIO1 = 0x02,
        RADIO_DIO2 = 0x04,
        RADIO_DIO3 = 0x08,
    };

    enum RadioTickSizes {
        RADIO_TICK_SIZE_0015_US = 0x00,
        RADIO_TICK_SIZE_0062_US = 0x01,
        RADIO_TICK_SIZE_1000_US = 0x02,
        RADIO_TICK_SIZE_4000_US = 0x03,
    };

    enum RadioRangingRoles {
        RADIO_RANGING_ROLE_SLAVE = 0x00,
        RADIO_RANGING_ROLE_MASTER = 0x01,
    };

    struct RadioStatus {
        struct
        {
            uint8_t CpuBusy : 1; //!< Flag for CPU radio busy
            uint8_t DmaBusy : 1; //!< Flag for DMA busy
            uint8_t CmdStatus : 3; //!< Command status
            uint8_t ChipMode : 3; //!< Chip mode
        } Fields;
        uint8_t Value;
    };

    typedef struct
    {
        uint8_t WakeUpRTC : 1; //!< Get out of sleep mode if wakeup signal received from RTC
        uint8_t InstructionRamRetention : 1; //!< InstructionRam is conserved during sleep
        uint8_t DataBufferRetention : 1; //!< Data buffer is conserved during sleep
        uint8_t DataRamRetention : 1; //!< Data ram is conserved during sleep
    } SleepParams;

    typedef struct
    {
        uint8_t RC64KEnable : 1; //!< Calibrate RC64K clock
        uint8_t RC13MEnable : 1; //!< Calibrate RC13M clock
        uint8_t PLLEnable : 1; //!< Calibrate PLL
        uint8_t ADCPulseEnable : 1; //!< Calibrate ADC Pulse
        uint8_t ADCBulkNEnable : 1; //!< Calibrate ADC bulkN
        uint8_t ADCBulkPEnable : 1; //!< Calibrate ADC bulkP
    } CalibrationParams;

    typedef struct
    {
        RadioPacketTypes packetType; //!< Packet to which the packet status are referring to.
        union {
            struct
            {
                uint16_t PacketReceived; //!< Number of received packets
                uint16_t CrcError; //!< Number of CRC errors
                uint16_t LengthError; //!< Number of length errors
                uint16_t SyncwordError; //!< Number of sync-word errors
            } Gfsk;
            struct
            {
                uint16_t PacketReceived; //!< Number of received packets
                uint16_t CrcError; //!< Number of CRC errors
                uint16_t HeaderValid; //!< Number of valid headers
            } LoRa;
        };
    } RxCounter;

    typedef struct
    {
        RadioPacketTypes packetType; //!< Packet to which the packet status are referring to.
        union {
            struct
            {
                int8_t RssiSync; //!< The RSSI measured on last packet
                struct
                {
                    bool SyncError : 1; //!< SyncWord error on last packet
                    bool LengthError : 1; //!< Length error on last packet
                    bool CrcError : 1; //!< CRC error on last packet
                    bool AbortError : 1; //!< Abort error on last packet
                    bool HeaderReceived : 1; //!< Header received on last packet
                    bool PacketReceived : 1; //!< Packet received
                    bool PacketControlerBusy : 1; //!< Packet controller busy
                } ErrorStatus; //!< The error status Byte
                struct
                {
                    bool RxNoAck : 1; //!< No acknowledgment received for Rx with variable length packets
                    bool PacketSent : 1; //!< Packet sent, only relevant in Tx mode
                } TxRxStatus; //!< The Tx/Rx status Byte
                uint8_t SyncAddrStatus : 3; //!< The id of the correlator who found the packet
            } Gfsk;
            struct
            {
                int8_t RssiPkt; //!< The RSSI of the last packet
                int8_t SnrPkt; //!< The SNR of the last packet
            } LoRa;
            struct
            {
                int8_t RssiSync; //!< The RSSI of the last packet
                struct
                {
                    bool SyncError : 1; //!< SyncWord error on last packet
                    bool LengthError : 1; //!< Length error on last packet
                    bool CrcError : 1; //!< CRC error on last packet
                    bool AbortError : 1; //!< Abort error on last packet
                    bool HeaderReceived : 1; //!< Header received on last packet
                    bool PacketReceived : 1; //!< Packet received
                    bool PacketControlerBusy : 1; //!< Packet controller busy
                } ErrorStatus; //!< The error status Byte
                struct
                {
                    uint8_t RxPid : 2; //!< PID of the Rx
                    bool RxNoAck : 1; //!< No acknowledgment received for Rx with variable length packets
                    bool RxPidErr : 1; //!< Received PID error
                    bool PacketSent : 1; //!< Packet sent, only relevant in Tx mode
                } TxRxStatus; //!< The Tx/Rx status Byte
                uint8_t SyncAddrStatus : 3; //!< The id of the correlator who found the packet
            } Flrc;
            struct
            {
                int8_t RssiSync; //!< The RSSI of the last packet
                struct
                {
                    bool SyncError : 1; //!< SyncWord error on last packet
                    bool LengthError : 1; //!< Length error on last packet
                    bool CrcError : 1; //!< CRC error on last packet
                    bool AbortError : 1; //!< Abort error on last packet
                    bool HeaderReceived : 1; //!< Header received on last packet
                    bool PacketReceived : 1; //!< Packet received
                    bool PacketControlerBusy : 1; //!< Packet controller busy
                } ErrorStatus; //!< The error status Byte
                struct
                {
                    bool PacketSent : 1; //!< Packet sent, only relevant in Tx mode
                } TxRxStatus; //!< The Tx/Rx status Byte
                uint8_t SyncAddrStatus : 3; //!< The id of the correlator who found the packet
            } Ble;
        };
    } PacketStatus;

    typedef struct
    {
        RadioPacketTypes PacketType; //!< Packet to which the packet parameters are referring to.
        struct
        {
            /*!
			 * \brief Holds the GFSK packet parameters
			 */
            struct
            {
                RadioPreambleLengths PreambleLength; //!< The preamble length for GFSK packet type
                RadioSyncWordLengths SyncWordLength; //!< The synchronization word length for GFSK packet type
                RadioSyncWordRxMatchs SyncWordMatch; //!< The synchronization correlator to use to check synchronization word
                RadioPacketLengthModes HeaderType; //!< If the header is explicit, it will be transmitted in the GFSK packet. If the header is implicit, it will not be transmitted
                uint8_t PayloadLength; //!< Size of the payload in the GFSK packet
                RadioCrcTypes CrcLength; //!< Size of the CRC block in the GFSK packet
                RadioWhiteningModes Whitening; //!< Usage of whitening on payload and CRC blocks plus header block if header type is variable
            } Gfsk;
            /*!
			 * \brief Holds the LORA packet parameters
			 */
            struct
            {
                uint8_t PreambleLength; //!< The preamble length is the number of LORA symbols in the preamble. To set it, use the following formula @code Number of symbols = PreambleLength[3:0] * ( 2^PreambleLength[7:4] ) @endcode
                RadioLoRaPacketLengthsModes HeaderType; //!< If the header is explicit, it will be transmitted in the LORA packet. If the header is implicit, it will not be transmitted
                uint8_t PayloadLength; //!< Size of the payload in the LORA packet
                RadioLoRaCrcModes Crc; //!< Size of CRC block in LORA packet
                RadioLoRaIQModes InvertIQ; //!< Allows to swap IQ for LORA packet
            } LoRa;
            /*!
			 * \brief Holds the FLRC packet parameters
			 */
            struct
            {
                RadioPreambleLengths PreambleLength; //!< The preamble length for FLRC packet type
                RadioFlrcSyncWordLengths SyncWordLength; //!< The synchronization word length for FLRC packet type
                RadioSyncWordRxMatchs SyncWordMatch; //!< The synchronization correlator to use to check synchronization word
                RadioPacketLengthModes HeaderType; //!< If the header is explicit, it will be transmitted in the FLRC packet. If the header is implicit, it will not be transmitted.
                uint8_t PayloadLength; //!< Size of the payload in the FLRC packet
                RadioCrcTypes CrcLength; //!< Size of the CRC block in the FLRC packet
                RadioWhiteningModes Whitening; //!< Usage of whitening on payload and CRC blocks plus header block if header type is variable
            } Flrc;
            /*!
			 * \brief Holds the BLE packet parameters
			 */
            struct
            {
                RadioBleConnectionStates ConnectionState; //!< The BLE state
                RadioBleCrcTypes CrcLength; //!< Size of the CRC block in the BLE packet
                RadioBleTestPayloads BleTestPayload; //!< Special BLE payload for test purpose
                RadioWhiteningModes Whitening; //!< Usage of whitening on PDU and CRC blocks of BLE packet
            } Ble;
        } Params; //!< Holds the packet parameters structure
    } PacketParams;

    typedef struct
    {
        RadioPacketTypes PacketType; //!< Packet to which the modulation parameters are referring to.
        struct
        {
            /*!
			 * \brief Holds the GFSK modulation parameters
			 *
			 * In GFSK modulation, the bit-rate and bandwidth are linked together. In this structure, its values are set using the same token.
			 */
            struct
            {
                RadioGfskBleBitrates BitrateBandwidth; //!< The bandwidth and bit-rate values for BLE and GFSK modulations
                RadioGfskBleModIndexes ModulationIndex; //!< The coding rate for BLE and GFSK modulations
                RadioModShapings ModulationShaping; //!< The modulation shaping for BLE and GFSK modulations
            } Gfsk;
            /*!
			 * \brief Holds the LORA modulation parameters
			 *
			 * LORA modulation is defined by Spreading Factor (SF), Bandwidth and Coding Rate
			 */
            struct
            {
                RadioLoRaSpreadingFactors SpreadingFactor; //!< Spreading Factor for the LORA modulation
                RadioLoRaBandwidths Bandwidth; //!< Bandwidth for the LORA modulation
                RadioLoRaCodingRates CodingRate; //!< Coding rate for the LORA modulation
            } LoRa;
            /*!
			 * \brief Holds the FLRC modulation parameters
			 *
			 * In FLRC modulation, the bit-rate and bandwidth are linked together. In this structure, its values are set using the same token.
			 */
            struct
            {
                RadioFlrcBitrates BitrateBandwidth; //!< The bandwidth and bit-rate values for FLRC modulation
                RadioFlrcCodingRates CodingRate; //!< The coding rate for FLRC modulation
                RadioModShapings ModulationShaping; //!< The modulation shaping for FLRC modulation
            } Flrc;
            /*!
			 * \brief Holds the BLE modulation parameters
			 *
			 * In BLE modulation, the bit-rate and bandwidth are linked together. In this structure, its values are set using the same token.
			 */
            struct
            {
                RadioGfskBleBitrates BitrateBandwidth; //!< The bandwidth and bit-rate values for BLE and GFSK modulations
                RadioGfskBleModIndexes ModulationIndex; //!< The coding rate for BLE and GFSK modulations
                RadioModShapings ModulationShaping; //!< The modulation shaping for BLE and GFSK modulations
            } Ble;
        } Params; //!< Holds the modulation parameters structure
    } ModulationParams;

    typedef struct TickTime_s {
        RadioTickSizes PeriodBase; //!< The base time of ticktime
        /*!
		 * \brief The number of periodBase for ticktime
		 * Special values are:
		 *     - 0x0000 for single mode
		 *     - 0xFFFF for continuous mode
		 */
        uint16_t PeriodBaseCount;
    } TickTime;

    SX1280();

	static SX1280 &instance();

    bool DevicePresent();
    uint16_t GetFirmwareVersion(void);
    RadioStatus GetStatus(void);
    RadioOperatingModes GetOperatingMode(void) { return OperatingMode; }

	// Perform a transfer
    void TxStart(const uint8_t *payload, uint8_t size, TickTime timeout = { RX_TIMEOUT_TICK_SIZE, TX_TIMEOUT_VALUE }, uint8_t offset = 0);

	// Callbacks
	void SetTxDoneCallback(std::function<void (void)> callback) { txDone = callback; };
	void SetRxDoneCallback(std::function<void (const uint8_t *payload, uint8_t size, PacketStatus packetStatus)> callback) { rxDone = callback; };

	void SetRxErrorCallback(std::function<void (IrqErrorCode errCode)> callback) { rxError = callback; };

	void SetTxTimeoutCallback(std::function<void (void)> callback) { txTimeout = callback; };
	void SetRxTimeoutCallback(std::function<void (void)> callback) { rxTimeout = callback; };

	void SetRxSyncWordDoneCallback(std::function<void (void)> callback) { rxSyncWordDone = callback; };
	void SetRxHeaderDoneCallback(std::function<void (void)> callback) { rxHeaderDone = callback; };
	void SetRangingDoneCallback(std::function<void (IrqRangingCode errCode, float value)> callback) { rangingDone = callback; };
	void SetCadDoneCallback(std::function<void (bool cadFlag)> callback) { cadDone = callback; };

	// Set operating modes
    void SetTx(TickTime timeout = { RX_TIMEOUT_TICK_SIZE, TX_TIMEOUT_VALUE });
    void SetRx(TickTime timeout = { RX_TIMEOUT_TICK_SIZE, RX_TIMEOUT_VALUE });
    void SetCad(void);
    void SetSleep(SleepParams sleepConfig);
    void SetStandby(RadioStandbyModes standbyConfig);
    void SetFs(void);

	// Use IRQ mode or poll for status
    void SetPollingMode(void);
    void SetInterruptMode(void);

	// Misc
    void Calibrate(CalibrationParams calibParam);
    void ForcePreambleLength(RadioPreambleLengths preambleLength);
    float GetFrequencyError();

    void SetModulationParams(const ModulationParams &modParams);
    void SetPacketParams(const PacketParams &packetParams);
    void SetRxDutyCycle(RadioTickSizes periodBase, uint16_t periodBaseCountRx, uint16_t periodBaseCountSleep);
    void SetTxContinuousWave(void);
    void SetTxContinuousPreamble(void);
    void SetTxParams(int8_t power, RadioRampTimes rampTime);
    void SetCadParams(RadioLoRaCadSymbols cadSymbolNum);
    void SetRegulatorMode(RadioRegulatorModes mode);
    void SetSaveContext(void);
    void SetAutoTx(uint16_t time);
    void SetAutoFs(bool enableAutoFs);
    void SetLongPreamble(bool enable);
    void SetSyncWordErrorTolerance(uint8_t ErrorBits);
    void SetBleAccessAddress(uint32_t accessAddress);
    void SetBleAdvertizerAccessAddress(void);
    void SetCrcPolynomial(uint16_t polynomial);
    void SetWhiteningSeed(uint8_t seed);
    void SetRangingIdLength(RadioRangingIdCheckLengths length);
    void SetDeviceRangingAddress(uint32_t address);
    void SetRangingRequestAddress(uint32_t address);
    void SetRangingCalibration(uint16_t cal);
    void RangingClearFilterResult(void);
    void RangingSetFilterNumSamples(uint8_t num);
    void SetRangingRole(RadioRangingRoles role);
    
    uint8_t SetSyncWord(uint8_t syncWordIdx, const uint8_t *syncWord);
    uint8_t SetCrcSeed(const uint8_t *seed);

private:

    static constexpr uint8_t LORA_MAX_BUFFER_SIZE = 255;
    uint8_t txBuffer[LORA_MAX_BUFFER_SIZE] = { 0 };
    uint8_t rxBuffer[LORA_MAX_BUFFER_SIZE] = { 0 };

    static constexpr uint32_t RF_FREQUENCY = 2425000000UL; // Overlay between channel 1 and channel 6
    static constexpr uint32_t TX_OUTPUT_POWER = 13;

    static constexpr uint16_t TX_TIMEOUT_VALUE = 1000; // ms
    static constexpr uint16_t RX_TIMEOUT_VALUE = 0xffff; // ms
    static constexpr RadioTickSizes RX_TIMEOUT_TICK_SIZE = RADIO_TICK_SIZE_1000_US;

    static constexpr uint16_t IrqMask = IRQ_TX_DONE | IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT;

    static constexpr uint32_t AUTO_TX_OFFSET = 0x21;
    static constexpr uint32_t MASK_RANGINGMUXSEL = 0xCF;
    static constexpr uint32_t DEFAULT_RANGING_FILTER_SIZE = 0x7F;
    static constexpr uint32_t MASK_FORCE_PREAMBLELENGTH = 0x8F;
    static constexpr uint32_t BLE_ADVERTIZER_ACCESS_ADDRESS = 0x8E89BED6;

	void SetDefaultLoraMode(uint8_t packetSize);

    void Reset();
    void Wakeup();
    void WaitOnBusy();

    void WriteCommand(RadioCommand command, const uint8_t *buffer, uint32_t size);
    void ReadCommand(RadioCommand command, uint8_t *buffer, uint32_t size);
    void WriteRegister(uint32_t address, const uint8_t *buffer, uint32_t size);
    void WriteRegister(uint32_t address, const uint8_t value);
    void ReadRegister(uint32_t address, uint8_t *buffer, uint32_t size);
    uint8_t ReadRegister(uint32_t address);

    void WriteBuffer(uint8_t offset, const uint8_t *buffer, uint8_t size);
    void ReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size);

    RadioPacketTypes GetPacketType(bool returnLocalCopy);
    void SetPacketType(RadioPacketTypes packetType);
    void SetRfFrequency(uint32_t rfFrequency);
    void SetBufferBaseAddresses(uint8_t txBaseAddress, uint8_t rxBaseAddress);
    void SetPayload(const uint8_t *buffer, uint8_t size, uint8_t offset);
    uint16_t GetIrqStatus(void);
    int8_t GetRssiInst(void);
    uint8_t GetPayload(uint8_t *buffer, uint8_t *size, uint8_t maxSize);
    void GetRxBufferStatus(uint8_t *rxPayloadLength, uint8_t *rxStartBufferPointer);
    void GetPacketStatus(PacketStatus* packetStatus);
    void SetDioIrqParams(uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask);
    void ClearIrqStatus(uint16_t irqMask);
    int32_t GetLoRaBandwidth();
    float GetRangingResult(RadioRangingResultTypes resultType);

    void OnDioIrq(void);
    void OnBusyIrq(void);
    void ProcessIrqs(void);

	std::function<void (void)> txDone;
	std::function<void (const uint8_t *payload, uint8_t size, PacketStatus packetStatus)> rxDone;
	std::function<void (void)> rxSyncWordDone;
	std::function<void (void)> rxHeaderDone;
	std::function<void (void)> txTimeout;
	std::function<void (void)> rxTimeout;
	std::function<void (IrqErrorCode errCode)> rxError;
	std::function<void (IrqRangingCode errCode, float value)> rangingDone;
	std::function<void (bool cadFlag)> cadDone;
	
    void init();
    void disableIRQ();
    void enableIRQ();
    void delayMS(uint32_t ms);
    
    void start_uart();
	void uart_handle();

	void do_wait_for_busy_pin();
	void do_pin_reset();
	void pins_init();
	void pin_irq_init();
	void spi_csel_low();
	void spi_csel_high();
    uint8_t spi_write(uint8_t val);

	static void OnDioIrq_C();
	static void OnBusyIrq_C();

    RadioOperatingModes OperatingMode = MODE_SLEEP;
    RadioPacketTypes PacketType = PACKET_TYPE_NONE;
    RadioLoRaBandwidths LoRaBandwidth = LORA_BW_0200;

	uint8_t LoraPacketSize = 51;
    bool IrqState = false;
    bool PollingMode = false;
    bool Initialized = false;
};

#endif // #ifndef SX1280_H_
