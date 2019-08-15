/*
Copyright 2019 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "./sx1280.h"
#include "./emulator.h"
#include "./model.h"

#include <atmel_start.h>

#include <algorithm>
#include <memory.h>
#include <vector>
            
#ifdef MCP
static const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

static inline bool is_base64(uint8_t c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static std::string base64_encode(uint8_t const* buf, unsigned int bufLen) {
    std::string ret;
    int i = 0;
    int j = 0;
    if (!buf || !bufLen) { 
        return ret;
    }
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];
    while (bufLen--) {
        char_array_3[i++] = *(buf++);
        if (i == 3) {
            char_array_4[0] = static_cast<uint8_t>((char_array_3[0] & 0xfc) >> 2);
            char_array_4[1] = static_cast<uint8_t>(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
            char_array_4[2] = static_cast<uint8_t>(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
            char_array_4[3] = static_cast<uint8_t>(char_array_3[2] & 0x3f);

            for(i = 0; (i <4) ; i++)
            ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
        char_array_3[j] = '\0';

        char_array_4[0] = static_cast<uint8_t>((char_array_3[0] & 0xfc) >> 2);
        char_array_4[1] = static_cast<uint8_t>(((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4));
        char_array_4[2] = static_cast<uint8_t>(((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6));
        char_array_4[3] = static_cast<uint8_t>(char_array_3[2] & 0x3f);

        for (j = 0; (j < i + 1); j++)
        ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
        ret += '=';
    }
    return ret;
}

static std::vector<uint8_t> base64_decode(std::string const& encoded_string) {
    int32_t in_len = encoded_string.size();
    int32_t i = 0;
    int32_t j = 0;
    int32_t in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    std::vector<uint8_t> ret;
    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++) {
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            }
            char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];
            for (i = 0; (i < 3); i++) {
                ret.push_back(char_array_3[i]);
                i = 0;
            }
        }
    }
    if (i) {
        for (j = 0; j < i; j++) {
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; (j < i - 1); j++) {
            ret.push_back(char_array_3[j]);
        }
    }
    return ret;
}
#endif  // make#ifdef MCP
            
SX1280::SX1280() {
}

SX1280 &SX1280::instance() {
    static SX1280 sx1280;
    if (!sx1280.Initialized) {
        sx1280.Initialized = true;
        sx1280.init();
    }
    return sx1280;
}

void SX1280::init() {

#ifdef MCP
    StartMCP();
#endif  // #ifdef MCP

    pins_init();

    Reset();

    pin_irq_init();

    SetInterruptMode();

    Wakeup();

    SetStandby(STDBY_RC);

    SetLoraRX();
}

void SX1280::Reset() {
    disableIRQ();
    
    do_pin_reset();

    enableIRQ();

    WaitOnBusy();
    
    delayMS( 10 );
}

void SX1280::Wakeup() {
    disableIRQ();

    spi_csel_low();
    spi_write(RADIO_GET_STATUS);
    spi_write(0);
    spi_csel_high();

    WaitOnBusy( );

    enableIRQ( );
}

bool SX1280::DevicePresent()  {
    return GetFirmwareVersion() == 0x0000a9b5;
}

uint16_t SX1280::GetFirmwareVersion( void )
{
    return static_cast<uint16_t>( ( ( ReadRegister( REG_LR_FIRMWARE_VERSION_MSB ) ) << 8 ) | ( ReadRegister( REG_LR_FIRMWARE_VERSION_MSB + 1 ) ) );
}

SX1280::RadioStatus SX1280::GetStatus( void )
{
    uint8_t stat = 0;
    RadioStatus status;

    ReadCommand( RADIO_GET_STATUS, reinterpret_cast<uint8_t *>(&stat), 1 );
    status.Value = stat;
    return( status );
}

void SX1280::WaitOnBusy() {
    do_wait_for_busy_pin();
}

void SX1280::WriteCommand(RadioCommand command, const uint8_t *buffer, uint32_t size) {
    WaitOnBusy();

    spi_csel_low();
    spi_write( static_cast<uint8_t>(command) );
    for( uint16_t i = 0; i < size; i++ ) {
        spi_write( buffer[i] );
    }
    spi_csel_high();

    if( command != RADIO_SET_SLEEP ) {
        WaitOnBusy( );
    }
}

void SX1280::ReadCommand(RadioCommand command, uint8_t *buffer, uint32_t size ) {
    WaitOnBusy();

    spi_csel_low();
    if( command == RADIO_GET_STATUS ) {
        buffer[0] = spi_write( RADIO_GET_STATUS );
        spi_write( 0 );
        spi_write( 0 );
    } else {
        spi_write( static_cast<uint8_t>(command) );
        spi_write( 0 );
        for( uint16_t i = 0; i < size; i++ )
        {
             buffer[i] = spi_write( 0 );
        }
    }
    spi_csel_high();

    WaitOnBusy( );
}

void SX1280::WriteRegister( uint32_t address, const uint8_t *buffer, uint32_t size ) {
    WaitOnBusy( );

    spi_csel_low();
    spi_write( RADIO_WRITE_REGISTER );
    spi_write( ( address & 0xFF00 ) >> 8 );
    spi_write( address & 0x00FF );
    for( uint16_t i = 0; i < size; i++ ) {
        spi_write( buffer[i] );
    }
    spi_csel_high();

    WaitOnBusy( );
}

void SX1280::WriteRegister( uint32_t address, uint8_t value ) {
    WriteRegister( address, &value, 1 );
}

void SX1280::ReadRegister( uint32_t address, uint8_t *buffer, uint32_t size ) {
    WaitOnBusy( );

    spi_csel_low();
    spi_write( RADIO_READ_REGISTER );
    spi_write( ( address & 0xFF00 ) >> 8 );
    spi_write( address & 0x00FF );
    spi_write( 0 );
    for( uint16_t i = 0; i < size; i++ ) {
        buffer[i] = spi_write( 0 );
    }
    spi_csel_high();

    WaitOnBusy( );
}

uint8_t SX1280::ReadRegister( uint32_t address ) {
    uint8_t data;
    ReadRegister( address, &data, 1 );
    return data;
}

void SX1280::WriteBuffer( uint8_t offset, const uint8_t *buffer, uint8_t size ) {
    WaitOnBusy( );

    spi_csel_low();
    spi_write( RADIO_WRITE_BUFFER );
    spi_write( offset );
    for( uint16_t i = 0; i < size; i++ ) {
        spi_write( buffer[i] );
    }
    spi_csel_high();

    WaitOnBusy( );
}

void SX1280::ReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size ) {
    WaitOnBusy( );

    spi_csel_low();
    spi_write( RADIO_READ_BUFFER );
    spi_write( offset );
    spi_write( 0 );
    for( uint16_t i = 0; i < size; i++ ) {
        buffer[i] = spi_write( 0 );
    }
    spi_csel_high();

    WaitOnBusy( );
}


void SX1280::SetSleep( SleepParams sleepConfig ) {
    uint8_t sleep = static_cast<uint8_t>(( sleepConfig.WakeUpRTC << 3 ) |
                                         ( sleepConfig.InstructionRamRetention << 2 ) |
                                         ( sleepConfig.DataBufferRetention << 1 ) |
                                         ( sleepConfig.DataRamRetention ));
    OperatingMode = MODE_SLEEP;
    WriteCommand( RADIO_SET_SLEEP, &sleep, 1 );
}

void SX1280::SetStandby( RadioStandbyModes standbyConfig ) {
    WriteCommand( RADIO_SET_STANDBY, reinterpret_cast<uint8_t *>(&standbyConfig), 1 );
    if( standbyConfig == STDBY_RC ) {
        OperatingMode = MODE_STDBY_RC;
    } else {
        OperatingMode = MODE_STDBY_XOSC;
    }
}

void SX1280::SetFs( void ) {
    WriteCommand( RADIO_SET_FS, 0, 0 );
    OperatingMode = MODE_FS;
}

void SX1280::SetTx( TickTime timeout ) {
    uint8_t buf[3];
    buf[0] = timeout.PeriodBase;
    buf[1] = static_cast<uint8_t>( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
    buf[2] = static_cast<uint8_t>( timeout.PeriodBaseCount & 0x00FF );

    ClearIrqStatus( IRQ_RADIO_ALL );

    // If the radio is doing ranging operations, then apply the specific calls
    // prior to SetTx
    if( GetPacketType( true ) == PACKET_TYPE_RANGING )
    {
        SetRangingRole( RADIO_RANGING_ROLE_MASTER );
    }
    WriteCommand( RADIO_SET_TX, buf, 3 );
    OperatingMode = MODE_TX;
}

void SX1280::SetRx( TickTime timeout ) {
    uint8_t buf[3];
    buf[0] = timeout.PeriodBase;
    buf[1] = static_cast<uint8_t>( ( timeout.PeriodBaseCount >> 8 ) & 0x00FF );
    buf[2] = static_cast<uint8_t>( timeout.PeriodBaseCount & 0x00FF );

    ClearIrqStatus( IRQ_RADIO_ALL );

    // If the radio is doing ranging operations, then apply the specific calls
    // prior to SetRx
    if( GetPacketType( true ) == PACKET_TYPE_RANGING )
    {
        SetRangingRole( RADIO_RANGING_ROLE_SLAVE );
    }
    WriteCommand( RADIO_SET_RX, buf, 3 );
    OperatingMode = MODE_RX;
}

void SX1280::SetRxDutyCycle( RadioTickSizes periodBase, uint16_t periodBaseCountRx, uint16_t periodBaseCountSleep ) {
    uint8_t buf[5];

    buf[0] = periodBase;
    buf[1] = static_cast<uint8_t>( ( periodBaseCountRx >> 8 ) & 0x00FF );
    buf[2] = static_cast<uint8_t>( periodBaseCountRx & 0x00FF );
    buf[3] = static_cast<uint8_t>( ( periodBaseCountSleep >> 8 ) & 0x00FF );
    buf[4] = static_cast<uint8_t>( periodBaseCountSleep & 0x00FF );
    WriteCommand( RADIO_SET_RXDUTYCYCLE, buf, 5 );
    OperatingMode = MODE_RX;
}

void SX1280::SetCad( void ) {
    WriteCommand( RADIO_SET_CAD, 0, 0 );
    OperatingMode = MODE_CAD;
}

void SX1280::SetTxContinuousWave( void ) {
    WriteCommand( RADIO_SET_TXCONTINUOUSWAVE, 0, 0 );
}

void SX1280::SetTxContinuousPreamble( void ) {
    WriteCommand( RADIO_SET_TXCONTINUOUSPREAMBLE, 0, 0 );
}

void SX1280::SetPacketType( RadioPacketTypes packetType ) {
    // Save packet type internally to avoid questioning the radio
    PacketType = packetType;

    WriteCommand( RADIO_SET_PACKETTYPE, reinterpret_cast<uint8_t *>(&packetType), 1 );
}

SX1280::RadioPacketTypes SX1280::GetPacketType( bool returnLocalCopy ) {
    RadioPacketTypes packetType = PACKET_TYPE_NONE;
    if( returnLocalCopy == false )
    {
        ReadCommand( RADIO_GET_PACKETTYPE, reinterpret_cast<uint8_t *>(&packetType), 1 );
        if( PacketType != packetType )
        {
            PacketType = packetType;
        }
    }
    else
    {
        packetType = PacketType;
    }
    return packetType;
}

void SX1280::SetRfFrequency( uint32_t rfFrequency ) {
    uint8_t buf[3];
    uint32_t freq = 0;

    const uint64_t XTAL_FREQ = 52000000;
    freq = static_cast<uint32_t>( (static_cast<uint64_t>(rfFrequency) * static_cast<uint64_t>(262144)) / static_cast<uint64_t>(XTAL_FREQ) );

    buf[0] = static_cast<uint8_t>( ( freq >> 16 ) & 0xFF );
    buf[1] = static_cast<uint8_t>( ( freq >> 8  ) & 0xFF );
    buf[2] = static_cast<uint8_t>( ( freq       ) & 0xFF );
    WriteCommand( RADIO_SET_RFFREQUENCY, buf, 3 );
}

void SX1280::SetTxParams( int8_t power, RadioRampTimes rampTime ) {
    uint8_t buf[2];

    // The power value to send on SPI/UART is in the range [0..31] and the
    // physical output power is in the range [-18..13]dBm
    buf[0] = static_cast<uint8_t>(power + 18);
    buf[1] = static_cast<uint8_t>(rampTime);
    WriteCommand( RADIO_SET_TXPARAMS, buf, 2 );
}

void SX1280::SetCadParams( RadioLoRaCadSymbols cadSymbolNum ) {
    WriteCommand( RADIO_SET_CADPARAMS, reinterpret_cast<uint8_t *>(&cadSymbolNum), 1 );
    OperatingMode = MODE_CAD;
}

void SX1280::SetBufferBaseAddresses( uint8_t txBaseAddress, uint8_t rxBaseAddress ) {
    uint8_t buf[2];

    buf[0] = txBaseAddress;
    buf[1] = rxBaseAddress;
    WriteCommand( RADIO_SET_BUFFERBASEADDRESS, buf, 2 );
}

void SX1280::SetModulationParams(const ModulationParams &modParams) {
    uint8_t buf[3];

    // Check if required configuration corresponds to the stored packet type
    // If not, silently update radio packet type
    if( PacketType != modParams.PacketType )
    {
        SetPacketType( modParams.PacketType );
    }

    switch( modParams.PacketType )
    {
        case PACKET_TYPE_GFSK:
            buf[0] = modParams.Params.Gfsk.BitrateBandwidth;
            buf[1] = modParams.Params.Gfsk.ModulationIndex;
            buf[2] = modParams.Params.Gfsk.ModulationShaping;
            break;
        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            buf[0] = modParams.Params.LoRa.SpreadingFactor;
            buf[1] = modParams.Params.LoRa.Bandwidth;
            buf[2] = modParams.Params.LoRa.CodingRate;
            LoRaBandwidth = modParams.Params.LoRa.Bandwidth;
            break;
        case PACKET_TYPE_FLRC:
            buf[0] = modParams.Params.Flrc.BitrateBandwidth;
            buf[1] = modParams.Params.Flrc.CodingRate;
            buf[2] = modParams.Params.Flrc.ModulationShaping;
            break;
        case PACKET_TYPE_BLE:
            buf[0] = modParams.Params.Ble.BitrateBandwidth;
            buf[1] = modParams.Params.Ble.ModulationIndex;
            buf[2] = modParams.Params.Ble.ModulationShaping;
            break;
        default:
        case PACKET_TYPE_NONE:
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = 0;
            break;
    }
    WriteCommand( RADIO_SET_MODULATIONPARAMS, buf, 3 );
}

void SX1280::SetPacketParams(const PacketParams &packetParams) {
    uint8_t buf[7];
    
    // Check if required configuration corresponds to the stored packet type
    // If not, silently update radio packet type
    if( PacketType != packetParams.PacketType )
    {
        SetPacketType( packetParams.PacketType );
    }

    switch( packetParams.PacketType )
    {
        case PACKET_TYPE_GFSK:
            buf[0] = packetParams.Params.Gfsk.PreambleLength;
            buf[1] = packetParams.Params.Gfsk.SyncWordLength;
            buf[2] = packetParams.Params.Gfsk.SyncWordMatch;
            buf[3] = packetParams.Params.Gfsk.HeaderType;
            buf[4] = packetParams.Params.Gfsk.PayloadLength;
            buf[5] = packetParams.Params.Gfsk.CrcLength;
            buf[6] = packetParams.Params.Gfsk.Whitening;
            break;
        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            buf[0] = packetParams.Params.LoRa.PreambleLength;
            buf[1] = packetParams.Params.LoRa.HeaderType;
            buf[2] = packetParams.Params.LoRa.PayloadLength;
            buf[3] = packetParams.Params.LoRa.Crc;
            buf[4] = packetParams.Params.LoRa.InvertIQ;
            buf[5] = 0;
            buf[6] = 0;
            break;
        case PACKET_TYPE_FLRC:
            buf[0] = packetParams.Params.Flrc.PreambleLength;
            buf[1] = packetParams.Params.Flrc.SyncWordLength;
            buf[2] = packetParams.Params.Flrc.SyncWordMatch;
            buf[3] = packetParams.Params.Flrc.HeaderType;
            buf[4] = packetParams.Params.Flrc.PayloadLength;
            buf[5] = packetParams.Params.Flrc.CrcLength;
            buf[6] = packetParams.Params.Flrc.Whitening;
            break;
        case PACKET_TYPE_BLE:
            buf[0] = packetParams.Params.Ble.ConnectionState;
            buf[1] = packetParams.Params.Ble.CrcLength;
            buf[2] = packetParams.Params.Ble.BleTestPayload;
            buf[3] = packetParams.Params.Ble.Whitening;
            buf[4] = 0;
            buf[5] = 0;
            buf[6] = 0;
            break;
        default:
        case PACKET_TYPE_NONE:
            buf[0] = 0;
            buf[1] = 0;
            buf[2] = 0;
            buf[3] = 0;
            buf[4] = 0;
            buf[5] = 0;
            buf[6] = 0;
            break;
    }
    WriteCommand( RADIO_SET_PACKETPARAMS, buf, 7 );
}

void SX1280::ForcePreambleLength( RadioPreambleLengths preambleLength ) {
    WriteRegister( REG_LR_PREAMBLELENGTH, ( ReadRegister( REG_LR_PREAMBLELENGTH ) & MASK_FORCE_PREAMBLELENGTH ) | preambleLength );
}

void SX1280::EnableManualGain() {
    WriteRegister( REG_ENABLE_MANUAL_GAIN_CONTROL, ReadRegister( REG_ENABLE_MANUAL_GAIN_CONTROL ) | 0x80 );
    WriteRegister( REG_DEMOD_DETECTION, static_cast<uint8_t>(ReadRegister( REG_DEMOD_DETECTION ) & 0xFE) );
}

void SX1280::DisableManualGain() {
    WriteRegister( REG_ENABLE_MANUAL_GAIN_CONTROL, ReadRegister( REG_ENABLE_MANUAL_GAIN_CONTROL ) & (~0x80) );
    WriteRegister( REG_DEMOD_DETECTION, static_cast<uint8_t>(ReadRegister( REG_DEMOD_DETECTION ) | (~0xFE)) );
}

void SX1280::SetManualGainValue(uint8_t gain) {
    WriteRegister( REG_MANUAL_GAIN_VALUE, static_cast<uint8_t>(( ReadRegister( REG_MANUAL_GAIN_VALUE ) & 0xF0 ) | gain) );
}

void SX1280::GetRxBufferStatus( uint8_t *rxPayloadLength, uint8_t *rxStartBufferPointer ) {
    uint8_t status[2];

    ReadCommand( RADIO_GET_RXBUFFERSTATUS, status, 2 );

    // In case of LORA fixed header, the rxPayloadLength is obtained by reading
    // the register REG_LR_PAYLOADLENGTH
    if( ( this -> GetPacketType( true ) == PACKET_TYPE_LORA ) && ( ReadRegister( REG_LR_PACKETPARAMS ) >> 7 == 1 ) )
    {
        *rxPayloadLength = ReadRegister( REG_LR_PAYLOADLENGTH );
    }
    else if( this -> GetPacketType( true ) == PACKET_TYPE_BLE )
    {
        // In the case of BLE, the size returned in status[0] do not include the 2-byte length PDU header
        // so it is added there
        *rxPayloadLength = static_cast<uint8_t>(std::min(255, (status[0]) + 2));
    }
    else
    {
        *rxPayloadLength = status[0];
    }

    *rxStartBufferPointer = status[1];
}

void SX1280::GetPacketStatus( PacketStatus *packetStatus ) {
    uint8_t status[5];

    ReadCommand( RADIO_GET_PACKETSTATUS, status, 5 );

    packetStatus->packetType = this -> GetPacketType( true );
    switch( packetStatus->packetType )
    {
        case PACKET_TYPE_GFSK:
            packetStatus->Gfsk.RssiSync = -( status[1] / 2 );

            packetStatus->Gfsk.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
            packetStatus->Gfsk.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

            packetStatus->Gfsk.TxRxStatus.RxNoAck = ( status[3] >> 5 ) & 0x01;
            packetStatus->Gfsk.TxRxStatus.PacketSent = status[3] & 0x01;

            packetStatus->Gfsk.SyncAddrStatus = status[4] & 0x07;
            break;
        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            packetStatus->LoRa.RssiPkt = -( status[0] / 2 );
            ( status[1] < 128 ) ? ( packetStatus->LoRa.SnrPkt = status[1] / 4 ) : ( packetStatus->LoRa.SnrPkt = ( ( status[1] - 256 ) /4 ) );
            break;
        case PACKET_TYPE_FLRC:
            packetStatus->Flrc.RssiSync = -( status[1] / 2 );

            packetStatus->Flrc.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
            packetStatus->Flrc.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

            packetStatus->Flrc.TxRxStatus.RxPid = ( status[3] >> 6 ) & 0x03;
            packetStatus->Flrc.TxRxStatus.RxNoAck = ( status[3] >> 5 ) & 0x01;
            packetStatus->Flrc.TxRxStatus.RxPidErr = ( status[3] >> 4 ) & 0x01;
            packetStatus->Flrc.TxRxStatus.PacketSent = status[3] & 0x01;

            packetStatus->Flrc.SyncAddrStatus = status[4] & 0x07;
            break;
        case PACKET_TYPE_BLE:
            packetStatus->Ble.RssiSync =  -( status[1] / 2 );

            packetStatus->Ble.ErrorStatus.SyncError = ( status[2] >> 6 ) & 0x01;
            packetStatus->Ble.ErrorStatus.LengthError = ( status[2] >> 5 ) & 0x01;
            packetStatus->Ble.ErrorStatus.CrcError = ( status[2] >> 4 ) & 0x01;
            packetStatus->Ble.ErrorStatus.AbortError = ( status[2] >> 3 ) & 0x01;
            packetStatus->Ble.ErrorStatus.HeaderReceived = ( status[2] >> 2 ) & 0x01;
            packetStatus->Ble.ErrorStatus.PacketReceived = ( status[2] >> 1 ) & 0x01;
            packetStatus->Ble.ErrorStatus.PacketControlerBusy = status[2] & 0x01;

            packetStatus->Ble.TxRxStatus.PacketSent = status[3] & 0x01;

            packetStatus->Ble.SyncAddrStatus = status[4] & 0x07;
            break;
        default:
        case PACKET_TYPE_NONE:
            // In that specific case, we set everything in the packetStatus to zeros
            // and reset the packet type accordingly
            for (uint32_t c=0; c<sizeof(PacketStatus); c++) { reinterpret_cast<uint8_t*>(packetStatus)[c] = 0; } 
            packetStatus->packetType = PACKET_TYPE_NONE;
            break;
    }
}

int8_t SX1280::GetRssiInst( void ) {
    uint8_t raw = 0;

    ReadCommand( RADIO_GET_RSSIINST, &raw, 1 );

    return static_cast<int8_t>( -raw / 2 );
}

void SX1280::SetDioIrqParams( uint16_t irqMask, uint16_t dio1Mask, uint16_t dio2Mask, uint16_t dio3Mask ) {
    uint8_t buf[8];

    buf[0] = static_cast<uint8_t>( ( irqMask >> 8 ) & 0x00FF );
    buf[1] = static_cast<uint8_t>( irqMask & 0x00FF );
    buf[2] = static_cast<uint8_t>( ( dio1Mask >> 8 ) & 0x00FF );
    buf[3] = static_cast<uint8_t>( dio1Mask & 0x00FF );
    buf[4] = static_cast<uint8_t>( ( dio2Mask >> 8 ) & 0x00FF );
    buf[5] = static_cast<uint8_t>( dio2Mask & 0x00FF );
    buf[6] = static_cast<uint8_t>( ( dio3Mask >> 8 ) & 0x00FF );
    buf[7] = static_cast<uint8_t>( dio3Mask & 0x00FF );
    WriteCommand( RADIO_SET_DIOIRQPARAMS, buf, 8 );
}

uint16_t SX1280::GetIrqStatus( void ) {
    uint8_t irqStatus[2];
    ReadCommand( RADIO_GET_IRQSTATUS, irqStatus, 2 );
    return static_cast<uint16_t>(( irqStatus[0] << 8 ) | irqStatus[1]);
}

void SX1280::ClearIrqStatus( uint16_t irqMask ) {
    uint8_t buf[2];

    buf[0] = static_cast<uint8_t>( ( static_cast<uint16_t>(irqMask) >> 8 ) & 0x00FF );
    buf[1] = static_cast<uint8_t>( static_cast<uint16_t>(irqMask) & 0x00FF );
    WriteCommand( RADIO_CLR_IRQSTATUS, buf, 2 );
}

void SX1280::Calibrate( CalibrationParams calibParam ) {
    uint8_t cal = static_cast<uint8_t>(( calibParam.ADCBulkPEnable << 5 ) |
                                       ( calibParam.ADCBulkNEnable << 4 ) |
                                       ( calibParam.ADCPulseEnable << 3 ) |
                                       ( calibParam.PLLEnable << 2 ) |
                                       ( calibParam.RC13MEnable << 1 ) |
                                       ( calibParam.RC64KEnable ));
    WriteCommand( RADIO_CALIBRATE, &cal, 1 );
}

void SX1280::SetRegulatorMode( RadioRegulatorModes mode ) {
    WriteCommand( RADIO_SET_REGULATORMODE, reinterpret_cast<uint8_t *>(&mode), 1 );
}

void SX1280::SetSaveContext( void ) {
    WriteCommand( RADIO_SET_SAVECONTEXT, 0, 0 );
}

void SX1280::SetAutoTx( uint16_t time ) {
    uint16_t compensatedTime = time - static_cast<uint16_t>(AUTO_TX_OFFSET);
    uint8_t buf[2];

    buf[0] = static_cast<uint8_t>( ( compensatedTime >> 8 ) & 0x00FF );
    buf[1] = static_cast<uint8_t>( compensatedTime & 0x00FF );
    WriteCommand( RADIO_SET_AUTOTX, buf, 2 );
}

void SX1280::SetAutoFs( bool enableAutoFs ) {
    WriteCommand( RADIO_SET_AUTOFS, reinterpret_cast<uint8_t *>(&enableAutoFs), 1 );
}

void SX1280::SetLongPreamble( bool enable ) {
    WriteCommand( RADIO_SET_LONGPREAMBLE, reinterpret_cast<uint8_t *>(&enable), 1 );
}

void SX1280::SetPayload(const uint8_t *buffer, uint8_t size, uint8_t offset) {
    WriteBuffer( offset, buffer, size );
}

uint8_t SX1280::GetPayload(uint8_t *buffer, uint8_t *size , uint8_t maxSize) {
    uint8_t offset;

    GetRxBufferStatus( size, &offset );
    if( *size > maxSize )
    {
        return 1;
    }
    ReadBuffer( offset, buffer, *size );
    return 0;
}

void SX1280::LoraTxStart(const uint8_t *payload, uint8_t size, TickTime timeout, uint8_t offset) {
    SetPayload(payload, size, offset);
    SetLoraTX(timeout);
}

uint8_t SX1280::SetSyncWord( uint8_t syncWordIdx, const uint8_t *syncWord ) {
    uint16_t addr;
    uint8_t syncwordSize = 0;

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_GFSK:
            syncwordSize = 5;
            switch( syncWordIdx )
            {
                case 1:
                    addr = REG_LR_SYNCWORDBASEADDRESS1;
                    break;
                case 2:
                    addr = REG_LR_SYNCWORDBASEADDRESS2;
                    break;
                case 3:
                    addr = REG_LR_SYNCWORDBASEADDRESS3;
                    break;
                default:
                    return 1;
            }
            break;
        case PACKET_TYPE_FLRC:
            // For FLRC packet type, the SyncWord is one byte shorter and
            // the base address is shifted by one byte
            syncwordSize = 4;
            switch( syncWordIdx )
            {
                case 1:
                    addr = REG_LR_SYNCWORDBASEADDRESS1 + 1;
                    break;
                case 2:
                    addr = REG_LR_SYNCWORDBASEADDRESS2 + 1;
                    break;
                case 3:
                    addr = REG_LR_SYNCWORDBASEADDRESS3 + 1;
                    break;
                default:
                    return 1;
            }
            break;
        case PACKET_TYPE_BLE:
            // For Ble packet type, only the first SyncWord is used and its
            // address is shifted by one byte
            syncwordSize = 4;
            switch( syncWordIdx )
            {
                case 1:
                    addr = REG_LR_SYNCWORDBASEADDRESS1 + 1;
                    break;
                default:
                    return 1;
            }
            break;
        default:
            return 1;
    }
    WriteRegister( addr, syncWord, syncwordSize );
    return 0;
}

void SX1280::SetSyncWordErrorTolerance( uint8_t ErrorBits ) {
    ErrorBits = ( ReadRegister( REG_LR_SYNCWORDTOLERANCE ) & 0xF0 ) | ( ErrorBits & 0x0F );
    WriteRegister( REG_LR_SYNCWORDTOLERANCE, ErrorBits );
}

uint8_t SX1280::SetCrcSeed(const uint8_t *seed) {
    uint8_t updated = 0;
    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_GFSK:
        case PACKET_TYPE_FLRC:
            WriteRegister( REG_LR_CRCSEEDBASEADDR, seed, 2 );
            updated = 1;
            break;
        case PACKET_TYPE_BLE:
            WriteRegister(0x9c7, seed[2] );
            WriteRegister(0x9c8, seed[1] );
            WriteRegister(0x9c9, seed[0] );
            updated = 1;
            break;
        default:
            break;
    }
    return updated;
}

void SX1280::SetBleAccessAddress( uint32_t accessAddress ) {
    WriteRegister( REG_LR_BLE_ACCESS_ADDRESS, ( accessAddress >> 24 ) & 0x000000FF );
    WriteRegister( REG_LR_BLE_ACCESS_ADDRESS + 1, ( accessAddress >> 16 ) & 0x000000FF );
    WriteRegister( REG_LR_BLE_ACCESS_ADDRESS + 2, ( accessAddress >> 8 ) & 0x000000FF );
    WriteRegister( REG_LR_BLE_ACCESS_ADDRESS + 3, accessAddress & 0x000000FF );
}

void SX1280::SetBleAdvertizerAccessAddress( void ) {
    SetBleAccessAddress( BLE_ADVERTIZER_ACCESS_ADDRESS );
}

void SX1280::SetCrcPolynomial( uint16_t polynomial ) {
    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_GFSK:
        case PACKET_TYPE_FLRC:
            uint8_t val[2];
            val[0] = static_cast<uint8_t>( (polynomial >> 8 ) & 0xFF);
            val[1] = static_cast<uint8_t>( polynomial  & 0xFF );
            WriteRegister( REG_LR_CRCPOLYBASEADDR, val, 2 );
            break;
        default:
            break;
    }
}

void SX1280::SetWhiteningSeed( uint8_t seed ) {
    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_GFSK:
        case PACKET_TYPE_FLRC:
        case PACKET_TYPE_BLE:
            WriteRegister( REG_LR_WHITSEEDBASEADDR, seed );
            break;
        default:
            break;
    }
}

void SX1280::SetRangingIdLength( RadioRangingIdCheckLengths length ) {
    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            WriteRegister( REG_LR_RANGINGIDCHECKLENGTH, static_cast<uint8_t>( ( ( ( static_cast<uint8_t>(length) ) & 0x03 ) << 6 ) | ( ReadRegister( REG_LR_RANGINGIDCHECKLENGTH ) & 0x3F ) ) );
            break;
        default:
            break;
    }
}

void SX1280::SetDeviceRangingAddress( uint32_t address ) {
    uint8_t addrArray[] = { static_cast<uint8_t>(address >> 24), static_cast<uint8_t>(address >> 16), static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address) };

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            WriteRegister( REG_LR_DEVICERANGINGADDR, addrArray, 4 );
            break;
        default:
            break;
    }
}

void SX1280::SetRangingRequestAddress( uint32_t address ) {
    uint8_t addrArray[] = { static_cast<uint8_t>(address >> 24), static_cast<uint8_t>(address >> 16), static_cast<uint8_t>(address >> 8), static_cast<uint8_t>(address) };

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            WriteRegister( REG_LR_REQUESTRANGINGADDR, addrArray, 4 );
            break;
        default:
            break;
    }
}

            
static int32_t complement2( const uint32_t num, const uint8_t bitCnt ) {
    int32_t retVal = int32_t(num);
    if( int32_t(num) >= 2<<( bitCnt - 2 ) ) {
        retVal -= 2<<( bitCnt - 1 );
    }
    return retVal;
}

float SX1280::GetRangingResult( RadioRangingResultTypes resultType ) {
    uint32_t valLsb = 0;
    float val = 0.0f;

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            SetStandby( STDBY_XOSC );
            WriteRegister( 0x97F, ReadRegister( 0x97F ) | ( 1 << 1 ) ); // enable LORA modem clock
            WriteRegister( REG_LR_RANGINGRESULTCONFIG, static_cast<uint8_t>( ( ReadRegister( REG_LR_RANGINGRESULTCONFIG ) & MASK_RANGINGMUXSEL ) | static_cast<uint32_t>( ( ( resultType ) & 0x03 ) << 4 ) ) );
            valLsb = static_cast<uint32_t>( ( ( ReadRegister( REG_LR_RANGINGRESULTBASEADDR ) << 16 ) | ( ReadRegister( REG_LR_RANGINGRESULTBASEADDR + 1 ) << 8 ) | ( ReadRegister( REG_LR_RANGINGRESULTBASEADDR + 2 ) ) ) );
            SetStandby( STDBY_RC );

            // Convertion from LSB to distance. For explanation on the formula, refer to Datasheet of SX1280
            switch( resultType )
            {
                case RANGING_RESULT_RAW:
                    // Convert the ranging LSB to distance in meter
                    // The theoretical conversion from register value to distance [m] is given by:
                    // distance [m] = ( complement2( register ) * 150 ) / ( 2^12 * bandwidth[MHz] ) )
                    // The API provide BW in [Hz] so the implemented formula is complement2( register ) / bandwidth[Hz] * A,
                    // where A = 150 / (2^12 / 1e6) = 36621.09
                    val = static_cast<float>(complement2( valLsb, 24 )) / static_cast<float>(GetLoRaBandwidth()) * 36621.09375f;
                    break;

                case RANGING_RESULT_AVERAGED:
                case RANGING_RESULT_DEBIASED:
                case RANGING_RESULT_FILTERED:
                    val = static_cast<float>(valLsb) * 20.0f / 100.0f;
                    break;
                default:
                    val = 0.0f;
            }
            break;
        default:
            break;
    }
    return val;
}

void SX1280::SetRangingCalibration( uint16_t cal ) {
    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_RANGING:
            WriteRegister( REG_LR_RANGINGRERXTXDELAYCAL, static_cast<uint8_t>( ( cal >> 8 ) & 0xFF ) );
            WriteRegister( REG_LR_RANGINGRERXTXDELAYCAL + 1, static_cast<uint8_t>( ( cal ) & 0xFF ) );
            break;
        default:
            break;
    }
}

void SX1280::RangingClearFilterResult( void ) {
    uint8_t regVal = ReadRegister( REG_LR_RANGINGRESULTCLEARREG );

    // To clear result, set bit 5 to 1 then to 0
    WriteRegister( REG_LR_RANGINGRESULTCLEARREG, regVal | ( 1 << 5 ) );
    WriteRegister( REG_LR_RANGINGRESULTCLEARREG, regVal & ( ~( 1 << 5 ) ) );
}

void SX1280::RangingSetFilterNumSamples( uint8_t num ) {
    // Silently set 8 as minimum value
    WriteRegister( REG_LR_RANGINGFILTERWINDOWSIZE, static_cast<uint8_t>( ( num < DEFAULT_RANGING_FILTER_SIZE ) ? DEFAULT_RANGING_FILTER_SIZE : num ) );
}

void SX1280::SetRangingRole( RadioRangingRoles role ) {
    uint8_t buf[1];

    buf[0] = role;
    WriteCommand( RADIO_SET_RANGING_ROLE, &buf[0], 1 );
}

float SX1280::GetFrequencyError( ) {
    uint8_t efeRaw[3] = {0};
    uint32_t efe = 0;
    float efeHz = 0.0f;

    switch( GetPacketType( true ) )
    {
        case PACKET_TYPE_LORA:
        case PACKET_TYPE_RANGING:
            efeRaw[0] = ReadRegister( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB );
            efeRaw[1] = ReadRegister( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB + 1 );
            efeRaw[2] = ReadRegister( REG_LR_ESTIMATED_FREQUENCY_ERROR_MSB + 2 );
            efe = static_cast<uint32_t>( ( efeRaw[0]<<16 ) | ( efeRaw[1]<<8 ) | efeRaw[2] );
            efe &= REG_LR_ESTIMATED_FREQUENCY_ERROR_MASK;

            efeHz = 1.55f * static_cast<float>(complement2( efe, 20 )) / ( 1600.0f / static_cast<float>(GetLoRaBandwidth()) * 1000.0f );
            break;
        case PACKET_TYPE_NONE:
        case PACKET_TYPE_BLE:
        case PACKET_TYPE_FLRC:
        case PACKET_TYPE_GFSK:
            break;
    }

    return efeHz;
}

void SX1280::SetHighSensitivity() {
    WriteRegister(REG_LNA_REGIME, (ReadRegister(REG_LNA_REGIME) | 0xC0));
}

void SX1280::SetLowPowerMode() {
    WriteRegister(REG_LNA_REGIME, (ReadRegister(REG_LNA_REGIME) & (~0xC0)));
}

uint8_t SX1280::GetRangingPowerDeltaThresholdIndicator()
{
    SetStandby( STDBY_XOSC );
    WriteRegister( 0x97F, ReadRegister( 0x97F ) | ( 1 << 1 ) ); // enable LoRa modem clock
    WriteRegister( REG_LR_RANGINGRESULTCONFIG, static_cast<uint8_t>( ( ReadRegister( REG_LR_RANGINGRESULTCONFIG ) & MASK_RANGINGMUXSEL ) | ( ( ( static_cast<uint8_t>(RANGING_RESULT_RAW) ) & 0x03 ) << 4 ) ) ); // Select raw results
    return ReadRegister( REG_RANGING_RSSI );
}

void SX1280::SetPollingMode( void ) {
    PollingMode = true;
}

int32_t SX1280::GetLoRaBandwidth( ) {
    int32_t bwValue = 0;

    switch( LoRaBandwidth ) {
        case LORA_BW_0200:
            bwValue = 203125;
            break;
        case LORA_BW_0400:
            bwValue = 406250;
            break;
        case LORA_BW_0800:
            bwValue = 812500;
            break;
        case LORA_BW_1600:
            bwValue = 1625000;
            break;
        default:
            bwValue = 0;
    }
    return bwValue;
}

void SX1280::SetInterruptMode( void ) {
    PollingMode = false;
}

void SX1280::OnDioIrq( void ) {
    if( PollingMode == true ) {
        IrqState = true;
    } else {
        ProcessIrqs( );
    }
}

void SX1280::SetLoraRX(TickTime timeout) {
    SetHighSensitivity();

    SetStandby(STDBY_RC);

    SetRfFrequency( RF_FREQUENCY );
    SetBufferBaseAddresses( 0x00, 0x00 );
    SetTxParams( TX_OUTPUT_POWER, RADIO_RAMP_20_US );
    SetDioIrqParams( SX1280::IrqMask, SX1280::IrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );

    ModulationParams modulationParams;
    memset(&modulationParams, 0, sizeof(modulationParams));
    modulationParams.PacketType                  = PACKET_TYPE_LORA;
    modulationParams.Params.LoRa.SpreadingFactor = LORA_SF11;
    modulationParams.Params.LoRa.Bandwidth       = LORA_BW_0200;
    modulationParams.Params.LoRa.CodingRate      = LORA_CR_LI_4_7;
    SetModulationParams( modulationParams );

    PacketParams packetParams;
    memset(&packetParams, 0, sizeof(packetParams));
    packetParams.PacketType                      = PACKET_TYPE_LORA;
    packetParams.Params.LoRa.PreambleLength      = 0x0C;
    packetParams.Params.LoRa.HeaderType          = LORA_PACKET_VARIABLE_LENGTH;
    packetParams.Params.LoRa.PayloadLength       = LORA_PACKET_SIZE;
    packetParams.Params.LoRa.Crc                 = LORA_CRC_ON;
    packetParams.Params.LoRa.InvertIQ            = LORA_IQ_NORMAL;
    SetPacketParams( packetParams );

    SetRx(timeout);
}

void SX1280::SetLoraTX(TickTime timeout) {
    SetHighSensitivity();

    SetStandby(STDBY_RC);
    
    SetRfFrequency( RF_FREQUENCY );
    SetBufferBaseAddresses( 0x00, 0x00 );
    SetTxParams( TX_OUTPUT_POWER, RADIO_RAMP_20_US );
    SetDioIrqParams( SX1280::IrqMask, SX1280::IrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );

    ModulationParams modulationParams;
    memset(&modulationParams, 0, sizeof(modulationParams));
    modulationParams.PacketType                  = PACKET_TYPE_LORA;
    modulationParams.Params.LoRa.SpreadingFactor = LORA_SF11;
    modulationParams.Params.LoRa.Bandwidth       = LORA_BW_0200;
    modulationParams.Params.LoRa.CodingRate      = LORA_CR_LI_4_7;
    SetModulationParams( modulationParams );

    PacketParams packetParams;
    memset(&packetParams, 0, sizeof(packetParams));
    packetParams.PacketType                      = PACKET_TYPE_LORA;
    packetParams.Params.LoRa.PreambleLength      = 0x0C;
    packetParams.Params.LoRa.HeaderType          = LORA_PACKET_VARIABLE_LENGTH;
    packetParams.Params.LoRa.PayloadLength       = LORA_PACKET_SIZE;
    packetParams.Params.LoRa.Crc                 = LORA_CRC_ON;
    packetParams.Params.LoRa.InvertIQ            = LORA_IQ_NORMAL;
    SetPacketParams( packetParams );

    SetTx(timeout);
}

void SX1280::SetRangingRX(TickTime timeout) {
    SetHighSensitivity();

    SetStandby(STDBY_RC);
    SetPacketType(PACKET_TYPE_RANGING);

    ModulationParams modulationParams;
    memset(&modulationParams, 0, sizeof(modulationParams));
    modulationParams.PacketType                  = PACKET_TYPE_RANGING;
    modulationParams.Params.LoRa.SpreadingFactor = LORA_SF10;
    modulationParams.Params.LoRa.Bandwidth       = LORA_BW_0400;
    modulationParams.Params.LoRa.CodingRate      = LORA_CR_4_8;
    SetModulationParams( modulationParams );

    PacketParams packetParams;
    memset(&packetParams, 0, sizeof(packetParams));
    packetParams.PacketType                      = PACKET_TYPE_RANGING;
    packetParams.Params.LoRa.PreambleLength      = 0x0C;
    packetParams.Params.LoRa.HeaderType          = LORA_PACKET_VARIABLE_LENGTH;
    packetParams.Params.LoRa.PayloadLength       = 0;
    packetParams.Params.LoRa.Crc                 = LORA_CRC_ON;
    packetParams.Params.LoRa.InvertIQ            = LORA_IQ_NORMAL;
    SetPacketParams( packetParams );

    SetRfFrequency( RF_FREQUENCY );
    SetTxParams(TX_OUTPUT_POWER, RADIO_RAMP_02_US);
    SetDeviceRangingAddress(Model::instance().UID());
    SetRangingIdLength(RANGING_IDCHECK_LENGTH_32_BITS);

    SetDioIrqParams((IRQ_RANGING_SLAVE_RESPONSE_DONE | IRQ_RANGING_SLAVE_REQUEST_DISCARDED | IRQ_RANGING_SLAVE_REQUEST_VALID), 
                    (IRQ_RANGING_SLAVE_RESPONSE_DONE | IRQ_RANGING_SLAVE_REQUEST_DISCARDED | IRQ_RANGING_SLAVE_REQUEST_VALID), 0, 0);
                    
    SetRangingCalibration(10020);
    SetRangingRole(RADIO_RANGING_ROLE_SLAVE);
    
    SetRx(timeout);
}

void SX1280::SetRangingTX(uint32_t targetAddress, TickTime timeout) {
    SetHighSensitivity();

    SetStandby(STDBY_RC);
    SetPacketType(PACKET_TYPE_RANGING);

    ModulationParams modulationParams;
    memset(&modulationParams, 0, sizeof(modulationParams));
    modulationParams.PacketType                  = PACKET_TYPE_RANGING;
    modulationParams.Params.LoRa.SpreadingFactor = LORA_SF10;
    modulationParams.Params.LoRa.Bandwidth       = LORA_BW_0400;
    modulationParams.Params.LoRa.CodingRate      = LORA_CR_4_8;
    SetModulationParams( modulationParams );

    PacketParams packetParams;
    memset(&packetParams, 0, sizeof(packetParams));
    packetParams.PacketType                      = PACKET_TYPE_RANGING;
    packetParams.Params.LoRa.PreambleLength      = 0x0C;
    packetParams.Params.LoRa.HeaderType          = LORA_PACKET_VARIABLE_LENGTH;
    packetParams.Params.LoRa.PayloadLength       = 0;
    packetParams.Params.LoRa.Crc                 = LORA_CRC_ON;
    packetParams.Params.LoRa.InvertIQ            = LORA_IQ_NORMAL;
    SetPacketParams( packetParams );

    SetRfFrequency( RF_FREQUENCY );
    SetTxParams(TX_OUTPUT_POWER, RADIO_RAMP_02_US);
    SetRangingRequestAddress(targetAddress);
    SetRangingIdLength(RANGING_IDCHECK_LENGTH_32_BITS);

    SetDioIrqParams((IRQ_RANGING_SLAVE_RESPONSE_DONE | IRQ_RANGING_SLAVE_REQUEST_DISCARDED | IRQ_RANGING_SLAVE_REQUEST_VALID), 
                    (IRQ_RANGING_SLAVE_RESPONSE_DONE | IRQ_RANGING_SLAVE_REQUEST_DISCARDED | IRQ_RANGING_SLAVE_REQUEST_VALID), 0, 0);
                    
    SetRangingCalibration(10020);
    SetRangingRole(RADIO_RANGING_ROLE_MASTER);
    
    SetTx(timeout);
}

void SX1280::OnBusyIrq( void ) {
}

#ifdef MCP
void SX1280::ReturnRange(float range) {
    struct io_descriptor *io = 0;
    usart_sync_get_io_descriptor(&USART_0, &io);
    char str[32];
    snprintf(str, 32, "G%d\n", int(range * 1000000));
    io_write(io, reinterpret_cast<const uint8_t *>(str), strlen(str));
}
#endif  // #ifdef MCP

void SX1280::ProcessIrqs( void ) {

    if( PollingMode == true ) {
        if( IrqState == true ) {
            disableIRQ( );
            IrqState = false;
            enableIRQ( );
        } else {
            return;
        }
    }

    RadioPacketTypes packetType = GetPacketType( true );
    uint16_t irqRegs = GetIrqStatus( );
    ClearIrqStatus( IRQ_RADIO_ALL );

    switch( packetType )
    {
        case PACKET_TYPE_GFSK:
        case PACKET_TYPE_FLRC:
        case PACKET_TYPE_BLE:
            switch( OperatingMode )
            {
                case MODE_RX:
                    if( ( irqRegs & IRQ_RX_DONE ) == IRQ_RX_DONE )
                    {
                        if( ( irqRegs & IRQ_CRC_ERROR ) == IRQ_CRC_ERROR )
                        {
                            if (rxError) {
                                rxError( IRQ_CRC_ERROR_CODE );
                            }
                        }
                        else if( ( irqRegs & IRQ_SYNCWORD_ERROR ) == IRQ_SYNCWORD_ERROR )
                        {
                            if (rxError) {
                                rxError( IRQ_SYNCWORD_ERROR_CODE );
                            }
                        }
                        else
                        {
                            if (rxDone) {
                                PacketStatus packetStatus;
                                GetPacketStatus(&packetStatus);
                                uint8_t rxBufferSize = 0;
                                GetPayload( rxBuffer, &rxBufferSize, LORA_MAX_BUFFER_SIZE );
                                rxDone(rxBuffer, rxBufferSize, packetStatus);
#ifdef MCP
                                struct io_descriptor *io = 0;
                                usart_sync_get_io_descriptor(&USART_0, &io);
                                
                                std::string str("P");
                                str += base64_encode(rxBuffer, rxBufferSize);
                                str += "\n";
                                io_write(io, reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
#endif  // #ifdef MCP
                            }
                        }
                    }
                    if( ( irqRegs & IRQ_SYNCWORD_VALID ) == IRQ_SYNCWORD_VALID )
                    {
                        if (rxSyncWordDone) {
                            rxSyncWordDone( );
                        }
                    }
                    if( ( irqRegs & IRQ_SYNCWORD_ERROR ) == IRQ_SYNCWORD_ERROR )
                    {
                        if (rxError) {
                            rxError( IRQ_SYNCWORD_ERROR_CODE );
                        }
                    }
                    if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if (rxTimeout) {
                            rxTimeout( );
                        }
                    }
                    break;
                case MODE_TX:
                    if( ( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
                    {
                        if (txDone) {
                            txDone( );
                        }
                        SetLoraRX();
                    } else if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT ) {
                        if (txTimeout) {
                            txTimeout( );
                        }
                        SetLoraRX();
                    }
                    break;
                default:
                    // Unexpected IRQ: silently returns
                    break;
            }
            break;
        case PACKET_TYPE_LORA:
            switch( OperatingMode )
            {
                case MODE_RX:
                    if( ( irqRegs & IRQ_RX_DONE ) == IRQ_RX_DONE )
                    {
                        if( ( irqRegs & IRQ_CRC_ERROR ) == IRQ_CRC_ERROR )
                        {
                            if (rxError) {
                                rxError( IRQ_CRC_ERROR_CODE );
                            }
                        }
                        else
                        {
                            if (rxDone) {
                                PacketStatus packetStatus;
                                GetPacketStatus(&packetStatus);
                                uint8_t rxBufferSize = 0;
                                GetPayload( rxBuffer, &rxBufferSize, LORA_MAX_BUFFER_SIZE);
                                rxDone(rxBuffer, rxBufferSize, packetStatus);
#ifdef MCP
                                struct io_descriptor *io = 0;
                                usart_sync_get_io_descriptor(&USART_0, &io);

                                std::string str("P");
                                str += base64_encode(rxBuffer, rxBufferSize);
                                str += "\n";
                                io_write(io, reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
#endif  // #ifdef MCP
                            }
                        }
                    }
                    if( ( irqRegs & IRQ_HEADER_VALID ) == IRQ_HEADER_VALID )
                    {
                        if (rxHeaderDone) {
                            rxHeaderDone( );
                        }
                    }
                    if( ( irqRegs & IRQ_HEADER_ERROR ) == IRQ_HEADER_ERROR )
                    {
                        if (rxError) {
                            rxError( IRQ_HEADER_ERROR_CODE );
                        }
                    }
                    if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if (rxTimeout) {
                            rxTimeout( );
                        }
                    }
                    if( ( irqRegs & IRQ_RANGING_SLAVE_REQUEST_DISCARDED ) == IRQ_RANGING_SLAVE_REQUEST_DISCARDED )
                    {
                        if (rxError) {
                            rxError( IRQ_RANGING_ON_LORA_ERROR_CODE );
                        }
                    }
                    break;
                case MODE_TX:
                    if( ( irqRegs & IRQ_TX_DONE ) == IRQ_TX_DONE )
                    {
                        if (txDone) {
                            txDone( );
                        }
                        SetLoraRX();
                    } else if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT ) {
                        if (txTimeout) {
                            txTimeout( );
                        }
                        SetLoraRX();
                    }
                    break;
                case MODE_CAD:
                    if( ( irqRegs & IRQ_CAD_DONE ) == IRQ_CAD_DONE )
                    {
                        if( ( irqRegs & IRQ_CAD_DETECTED ) == IRQ_CAD_DETECTED )
                        {
                            if (cadDone) {
                                cadDone( true );
                            }
                        }
                        else
                        {
                            if (cadDone) {
                                cadDone( false );
                            }
                        }
                    }
                    else if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if (rxTimeout) {
                            rxTimeout( );
                        }
                    }
                    break;
                default:
                    // Unexpected IRQ: silently returns
                    break;
            }
            break;
        case PACKET_TYPE_RANGING:
            switch( OperatingMode )
            {
                // MODE_RX indicates an IRQ on the Slave side
                case MODE_RX:
                    if( ( irqRegs & IRQ_RANGING_SLAVE_REQUEST_DISCARDED ) == IRQ_RANGING_SLAVE_REQUEST_DISCARDED )
                    {
                        if (rangingDone) {
                            rangingDone( IRQ_RANGING_SLAVE_ERROR_CODE, 0.0f );
                        }
                        SetLoraRX();
                    }
                    if( ( irqRegs & IRQ_RANGING_SLAVE_REQUEST_VALID ) == IRQ_RANGING_SLAVE_REQUEST_VALID )
                    {
                        float range = GetRangingResult(RANGING_RESULT_RAW);
                        if (rangingDone) {
                            rangingDone( IRQ_RANGING_SLAVE_VALID_CODE, range);
                        }
#ifdef MCP
                        ReturnRange(range);
#endif  // #ifdef MCP
                        SetLoraRX();
                    }
                    if( ( irqRegs & IRQ_RANGING_SLAVE_RESPONSE_DONE ) == IRQ_RANGING_SLAVE_RESPONSE_DONE )
                    {
                        float range = GetRangingResult(RANGING_RESULT_RAW);
                        if (rangingDone) {
                            rangingDone( IRQ_RANGING_SLAVE_VALID_CODE, range);
                        }
#ifdef MCP
                        ReturnRange(range);
#endif  // #ifdef MCP
                        SetLoraRX();
                    }
                    if( ( irqRegs & IRQ_RX_TX_TIMEOUT ) == IRQ_RX_TX_TIMEOUT )
                    {
                        if (rangingDone) {
                            rangingDone( IRQ_RANGING_SLAVE_ERROR_CODE, 0.0f );
                        }
                        SetLoraRX();
                    }
                    if( ( irqRegs & IRQ_HEADER_VALID ) == IRQ_HEADER_VALID )
                    {
                        if (rxHeaderDone) {
                            rxHeaderDone( );
                        }
                    }
                    if( ( irqRegs & IRQ_HEADER_ERROR ) == IRQ_HEADER_ERROR )
                    {
                        if (rxError) {
                            rxError( IRQ_HEADER_ERROR_CODE );
                        }
                    }
                    break;
                // MODE_TX indicates an IRQ on the Master side
                case MODE_TX:
                    if( ( irqRegs & IRQ_RANGING_MASTER_TIMEOUT ) == IRQ_RANGING_MASTER_TIMEOUT )
                    {
                        if (rangingDone) {
                            rangingDone( IRQ_RANGING_MASTER_ERROR_CODE, 0.0f );
                        }
                        SetLoraRX();
                    } else if( ( irqRegs & IRQ_RANGING_MASTER_RESULT_VALID ) == IRQ_RANGING_MASTER_RESULT_VALID ) {
                        if (rangingDone) {
                            rangingDone( IRQ_RANGING_MASTER_VALID_CODE, GetRangingResult(RANGING_RESULT_RAW) );
                        }
                        SetLoraRX();
                    }
                    break;
                default:
                    // Unexpected IRQ: silently returns
                    break;
            }
            break;
        case PACKET_TYPE_NONE:
            break;
        default:
            // Unexpected IRQ: silently returns
            break;
    }
}

void SX1280::OnDioIrq_C() {
    instance().OnDioIrq();
}

void SX1280::OnBusyIrq_C() {
    instance().OnBusyIrq();
}

void SX1280::disableIRQ() {
    __disable_irq();
}

void SX1280::enableIRQ() {
    __enable_irq();
}

void SX1280::delayMS(uint32_t ms) {
    delay_ms(static_cast<int32_t>(ms));
}

void SX1280::do_wait_for_busy_pin() {
    while (gpio_get_pin_level(SX1280_BUSY)) { }
}

void SX1280::do_pin_reset()
{
    delay_ms(50);
    gpio_set_pin_level(SX1280_RESET, false);
    delay_ms(50);
    gpio_set_pin_level(SX1280_RESET, true);
    delay_ms(50);
}

#ifdef MCP
void SX1280::StartMCP() 
{
    struct io_descriptor *io;
    usart_sync_get_io_descriptor(&USART_0, &io);
    usart_sync_enable(&USART_0);

	for (int32_t c=0; c<1024;c++) {
		const char *str = "AAAAAAAAAAAAAAAAAAAA";
		io_write(io, reinterpret_cast<const uint8_t *>(str), strlen(str));
	}
}

void SX1280::OnMCPTimer()
{
    static int32_t s = 0;
    static uint8_t cmd[64];

    struct io_descriptor *io;
    usart_sync_get_io_descriptor(&USART_0, &io);

    while (usart_sync_is_rx_not_empty(&USART_0)) {
        uint8_t c;
        io_read(io, &c,1);
        if (c == 'C') {
            s = 0;
        }
        else if (s >= 64) {
            s = 0;
        }
        else if (c == '\n') {
            if (s > 1) {
                switch(cmd[1]) {
                    case    'G': {
                                std::vector<uint8_t> data = base64_decode(std::string(&cmd[2], &cmd[s-1]));
                                if (data.size() >= 4) {
                                    SetRangingTX((data[0] << 24)|
                                                 (data[1] << 16)|
                                                 (data[2] <<  8)|
                                                 (data[3] <<  0));
                                }
                            } break;
                    case    'W': {
                                if (s > 2) {
                                    std::vector<uint8_t> data = base64_decode(std::string(&cmd[3], &cmd[s-1]));
                                    switch(cmd[2]) {
                                        case    'C': {
                                            WriteCommand((RadioCommand)data.data()[0], data.data()+1, data.size()-1);
                                        } break;
                                        case    'R': {
                                            WriteRegister(data.data()[0], data.data()+1, data.size()-1);
                                        } break;
                                        case    'M': {
                                            LoraTxStart(data.data(), data.size());
                                        } break;
                                    }
                                }
                            } break;
                    case    'R': {
                                if (s > 2) {
                                    std::vector<uint8_t> data = base64_decode(std::string(&cmd[3], &cmd[s-1]));
                                    switch(cmd[2]) {
                                        case    'C': {
                                            uint8_t buf[2];
                                            ReadCommand((RadioCommand)data[0], &buf[0], data[1]);
                                            std::string str("R"); 
                                            str += base64_encode(buf, data[1]);
                                            str += "\n";
                                            io_write(io, reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
                                        } break;
                                        case    'R': {
                                            uint8_t buf[2];
                                            ReadRegister((data[0]<<8)|data[1], &buf[0], data[2]);
                                            std::string str("R");
                                            str += base64_encode(buf, data[1]);
                                            str += "\n";
                                            io_write(io, reinterpret_cast<const uint8_t *>(str.c_str()), str.length());
                                        } break;
                                    }
                                }
                            } break;
                }
            }
            s = 0;
        }
        cmd[s] = c;
    }
}
#endif  // #ifdef MCP

#ifdef EMULATOR
void SX1280::RxDone(const uint8_t *payload, uint8_t size, PacketStatus packetStatus) {
	rxDone(payload, size, packetStatus);
}
#endif  // #ifdef EMULATOR

void SX1280::pins_init()
{
    spi_m_sync_enable(&SPI_0);
}

void SX1280::pin_irq_init()
{
    ext_irq_register(PIN_PB08, OnBusyIrq_C);
    ext_irq_register(PIN_PB09, OnDioIrq_C);
}

void SX1280::spi_csel_low() {
    gpio_set_pin_level(SX1280_SSEL, false);
}

void SX1280::spi_csel_high() {
    gpio_set_pin_level(SX1280_SSEL, true);
}

uint8_t SX1280::spi_write(uint8_t write_value) {
    spi_xfer xfer;
    uint8_t read_value = 0;
    xfer.rxbuf = &read_value;
    xfer.txbuf = &write_value;
    xfer.size = 1;
    spi_m_sync_transfer(&SPI_0, &xfer);
    return read_value;
}
