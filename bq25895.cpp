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
#include "./bq25895.h"
#include "./emulator.h"
#include "./system_time.h"

#include <atmel_start.h>
#include <math.h>

BQ25895 &BQ25895::instance() {
    static BQ25895 bq25895;
    i2c_m_sync_get_io_descriptor(&I2C_0, &bq25895.I2C_0_io);
    if (!bq25895.deviceChecked) {
        bq25895.deviceChecked = true;
        i2c_m_sync_set_slaveaddr(&I2C_0, i2caddr, I2C_M_SEVEN);
        uint8_t status = 0x8;
        uint8_t value = 0x0;
        if ((io_write(bq25895.I2C_0_io, &status, 1)) == 1 &&
            (io_read(bq25895.I2C_0_io, &value, 1)) == 1) {
            bq25895.devicePresent = true;
        }
        ext_irq_register(PIN_PA16, PinInterrupt_C);
    }
    return bq25895;
}

void BQ25895::PinInterrupt_C() {
    instance().PinInterrupt();
}

void BQ25895::PinInterrupt() {
}

void BQ25895::SetBoostVoltage (uint32_t voltageMV) {
    uint8_t reg = getRegister(0x06);
    if ((voltageMV >= 4550) && (voltageMV <= 5510)) {
        uint32_t codedValue = voltageMV;
        codedValue = (codedValue - 4550) / 64;
        if ((voltageMV - 4550) % 64 != 0) {
            codedValue++;
        }
        reg &= ~(0x0f << 4);
        reg |= (uint8_t) ((codedValue & 0x0f) << 4);
        setRegister (0x0A, reg);
    }
}
     
uint32_t BQ25895::GetBoostVoltage () {
    uint8_t reg = getRegister(0x0A);
    reg = (reg >> 4) & 0x0f;
    return 4550 + ((uint32_t) reg) * 64;
}

void BQ25895::SetInputCurrent(uint32_t currentMA) {
    if (currentMA >= 50 && currentMA <= 3250) {
        uint32_t codedValue = currentMA;
        codedValue = ((codedValue) / 50) - 1;
        codedValue |= (1 << 6);
        setRegister (0x00, codedValue);
    }
    if (currentMA == 0) {
        uint32_t codedValue = 0;
        codedValue |= (1 << 6);
        setRegister (0x00, codedValue);
    }
}
     
uint32_t BQ25895::GetInputCurrent () {
    return ((getRegister(0x00) & (0x3F)) * 50);
}

void BQ25895::DisableWatchdog() {
    clearRegisterBits(0x07, (1 << 4));
    clearRegisterBits(0x07, (1 << 5));
}
     
void BQ25895::DisableOTG() {
    clearRegisterBits(0x03, (1 << 5));
}

void BQ25895::StartContinousADC() {
    setRegisterBits(0x02, (1 << 6));
}

bool BQ25895::ADCActive() {
    return ( getRegister(0x02) & (1 << 7) ) ? true : false;
}

void BQ25895::OneShotADC() {
    clearRegisterBits(0x02, (1 << 6));
    setRegisterBits(0x02, (1 << 7));
}

float BQ25895::BatteryVoltage() {
    uint8_t reg = getRegister(0x0E) & 0x7F;
    return 2.304f + ( float(reg) * 2.540f ) * ( 1.0f / 127.0f);
}

float BQ25895::SystemVoltage() {
    uint8_t reg = getRegister(0x0F) & 0x7F;
    return 2.304f + ( float(reg) * 2.540f ) * ( 1.0f / 127.0f);
}

float BQ25895::VBUSVoltage() {
    uint8_t reg = getRegister(0x11) & 0x7F;
    return 2.6f + ( float(reg) * 12.7f ) * ( 1.0f / 127.0f);
}

float BQ25895::ChargeCurrent() {
    uint8_t reg = getRegister(0x12) & 0x7F;
    return ( float(reg) * 6350.0f ) * ( 1.0f / 127.0f);
}

uint8_t BQ25895::GetStatus() {
    return getRegister(0x0B);
}
     
uint8_t BQ25895::FaultState() {
    return fault_state;
}
     
bool BQ25895::IsInFaultState() {
    uint8_t reg = getRegister(0x0C);
    fault_state = reg;
    return reg != 0;
}
     
uint8_t BQ25895::getRegister(uint8_t address) {
    i2c_m_sync_set_slaveaddr(&I2C_0, i2caddr, I2C_M_SEVEN);
    uint8_t value = 0x0;
    if ((io_write(I2C_0_io, &address, 1)) == 1 &&
        (io_read(I2C_0_io, &value, 1)) == 1) {
        return value;
    }
    return 0;
}

void BQ25895::setRegister(uint8_t address, uint8_t value) {
    i2c_m_sync_set_slaveaddr(&I2C_0, i2caddr, I2C_M_SEVEN);
    uint8_t set[2];
    set[0] = address;
    set[1] = value;
    io_write(I2C_0_io, set, 2);
}

void BQ25895::setRegisterBits(uint8_t address, uint8_t mask) {
    uint8_t value = getRegister(address);
    value |= mask;
    setRegister(address, value);
}

void BQ25895::clearRegisterBits(uint8_t address, uint8_t mask) {
    uint8_t value = getRegister(address);
    value &= ~mask;
    setRegister(address, value);
}
