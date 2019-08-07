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
#ifndef SDD1306_H_
#define SDD1306_H_

#include <cstdint>
#include <cstddef>

class SDD1306 {
public:
    
    SDD1306();

    static SDD1306 &instance();
    
    void Clear();
    void ClearAttr();
    void DisplayBootScreen();
    void SetCenterFlip(int8_t progression);

    void PlaceUTF8String(uint32_t x, uint32_t y, const char *str);
    void Invert();
    void SetAttr(uint32_t x, uint32_t y, uint8_t attr);
    void SetAsciiScrollMessage(const char *str, int32_t offset);
    void Display();
    void SetVerticalShift(int8_t val);

    void DisplayOn();
    void DisplayOff();
    void DisplayUID();

    bool DevicePresent() const { return devicePresent; }

private:
    void Init();

    static constexpr uint32_t i2caddr = 0x3C;

    void DisplayCenterFlip();
    void DisplayChar(uint32_t x, uint32_t y, uint16_t ch, uint8_t attr);
    void WriteCommand(uint8_t v) const;
    void BulkTransfer(const uint8_t *buf, size_t size) const;

    bool devicePresent = false;

    int8_t center_flip_screen = 0;
    int8_t center_flip_cache = 0;
    uint16_t text_buffer_cache[12*2];
    uint16_t text_buffer_screen[12*2];
    uint8_t text_attr_cache[12*2];
    uint8_t text_attr_screen[12*2];
    
    bool display_scroll_message = false;
    uint8_t scroll_message[64];
    int32_t scroll_message_offset = 0;
    int32_t scroll_message_len = 0;
    bool initialized = false;

    struct io_descriptor *I2C_0_io = 0;

};  // class SDD1306


#endif /* SDD1306_H_ */