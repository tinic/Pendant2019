#ifndef BQ25895_H_
#define BQ25895_H_

#include <cstdint>

class BQ25895 {
	 
public:
	 BQ25895() {
		 devicePresent = false;
		 deviceChecked = false;
		 fault_state = 0;
#ifndef EMULATOR
		 I2C_0_io = 0;
#endif  // #ifndef EMULATOR
	 }
	 
	 static BQ25895 &instance();

	 void SetBoostVoltage(uint32_t voltageMV);
	 uint32_t GetBoostVoltage();

	 void DisableWatchdog();
	 void DisableOTG();

	 void SetInputCurrent(uint32_t currentMA);
	 uint32_t GetInputCurrent();

	 void StartContinousADC();

	 bool ADCActive();
	 void OneShotADC();

	 float BatteryVoltage();
	 float SystemVoltage();
	 float VBUSVoltage();
	 float ChargeCurrent();

	 uint8_t GetStatus();
	 uint8_t FaultState();
	 bool IsInFaultState();
	 bool DevicePresent() const { return devicePresent; }

 private:

	 void PinInterrupt();
	 static void PinInterrupt_C();

	 static constexpr uint32_t i2caddr = 0x6A;

	 uint8_t getRegister(uint8_t address);
	 void setRegister(uint8_t address, uint8_t value);
	 void setRegisterBits(uint8_t address, uint8_t mask);
	 void clearRegisterBits(uint8_t address, uint8_t mask);

	 bool devicePresent;
	 bool deviceChecked;
	 uint8_t fault_state;
	 
#ifndef EMULATOR
	 struct io_descriptor *I2C_0_io;
#endif  // #ifndef EMULATOR
};

#endif /* BQ25895_H_ */