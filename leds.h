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
#ifndef LEDS_H_
#define LEDS_H_

#include <cstdint>
#include <algorithm>
#include <cmath>
#include <cfloat>

namespace colors {

#ifndef EMULATOR
    static constexpr float global_limit_factor = 0.606f;
#else  // #ifndef EMULATOR
    static constexpr float global_limit_factor = 1.000f;
#endif  // #ifndef EMULATOR

    class rgb8;
    class rgb8out;
    class hsl;
    class hsv;
    class hsp;

    class rgb {
    public:
        float r;
        float g;
        float b;

        rgb() :
            r(0.0f),
            g(0.0f),
            b(0.0f){
        }
    
        rgb(const rgb &from) :
            r(from.r),
            g(from.g),
            b(from.b) {
        }

        explicit rgb(const uint32_t color);
        explicit rgb(const rgb8 &from);
        explicit rgb(const rgb8out &from);
        explicit rgb(const hsl &from);
        explicit rgb(const hsv &from);
        explicit rgb(const hsp &from);

        rgb(float _r, float _g, float _b) :
            r(_r),
            g(_g),
            b(_b) {
        }

        void set(float _r, float _g, float _b) {
            r = _r;
            g = _g;
            b = _b;
        }

        rgb &operator+=(const rgb &v) {
            r += v.r;
            g += v.g;
            b += v.b;
            return *this;
        }

        friend rgb operator+(rgb a, const rgb &_b) {
            a += _b;
            return a;
        }

        rgb &operator-=(const rgb &v) {
            r -= v.r;
            g -= v.g;
            b -= v.b;
            return *this;
        }

        friend rgb operator-(rgb a, const rgb &_b) {
            a -= _b;
            return a;
        }

        rgb &operator*=(const rgb &v) {
            r *= v.r;
            g *= v.g;
            b *= v.b;
            return *this;
        }

        friend rgb operator*(rgb a, const rgb &_b) {
            a *= _b;
            return a;
        }

        rgb &operator*=(float v) {
            r *= v;
            g *= v;
            b *= v;
            return *this;
        }

        friend rgb operator*(rgb a, float v) {
            a *= v;
            return a;
        }

        rgb &operator/=(const rgb &v) {
            r /= v.r;
            g /= v.g;
            b /= v.b;
            return *this;
        }

        friend rgb operator/(rgb a, const rgb &_b) {
            a /= _b;
            return a;
        }

        rgb &operator/=(float v) {
            r /= v;
            g /= v;
            b /= v;
            return *this;
        }

        friend rgb operator/(rgb a, float v) {
            a /= v;
            return a;
        }
    
    private:
    };

    class rgb8 {
    public:

        union {
            struct {
                uint8_t r;
                uint8_t g;
                uint8_t b;
                uint8_t x;
            };
            uint32_t rgbx;
        };

        rgb8() :
            r(0),
            g(0),
            b(0),
            x(0){
        }
    
        rgb8(const rgb8 &from) :
            r(from.r),
            g(from.g),
            b(from.b),
            x(0) {
        }

        explicit rgb8(const rgb &from) {
            r = sat8(from.r);
            g = sat8(from.g);
            b = sat8(from.b);
            x = 0;
        }

        explicit rgb8(uint8_t _r, uint8_t _g, uint8_t _b) :
            r(_r),
            g(_g),
            b(_b),
            x(0) {
        }
        
        uint32_t hex() const {
            return static_cast<uint32_t>((r<<16) | (g<<8) | b);
        }
        
        bool operator==(const rgb8 &c) const {
            return rgbx == c.rgbx;
        }

        bool operator!=(const rgb8 &c) const {
            return rgbx != c.rgbx;
        }
        
    private:
    
        uint8_t sat8(const float v) const {
            return v < 0.0f ? uint8_t(0) : ( v > 1.0f ? uint8_t(0xFF) : uint8_t( v * 255.f ) );
        }

    };

    class hsl {
    public:
        float h;
        float s;
        float l;
    
        hsl() :
            h(0.0f),
            s(0.0f),
            l(0.0f) {
        }

        hsl(float _h, float _s, float _l) :
            h(_h),
            s(_s),
            l(_l) {
        }

        hsl(const hsl &from) :
            h(from.h),
            s(from.s),
            l(from.l) {
        }
    
        explicit hsl(const rgb &from) {
            float hi = std::max(std::max(from.r, from.g), from.b);
            float lo = std::min(std::max(from.r, from.g), from.b);
            float d = fabsf(hi - lo);
        
            h = 0;
            s = 0;
            l = (hi + lo) * 0.5f;
        
            if (d > 0.00001f) {
                s = d / ( 1.0f - fabsf( 2.0f * l - 1.0f ) );
                if( hi == from.r ) {
                    h = (60.0f/360.0f) * (from.g - from.b) / d + (from.g < from.b ? 1.0f : 0.0f);
                }
                if( hi == from.g ) {
                    h = (60.0f/360.0f) * (from.b - from.r) / d + (120.0f/360.0f);
                }
                if( hi == from.b ) {
                    h = (60.0f/360.0f) * (from.r - from.g) / d + (240.0f/360.0f);
                }
            }
        }
    };

    class hsv {
    public:
        float h;
        float s;
        float v;

        hsv() :
            h(0.0f),
            s(0.0f),
            v(0.0f) {
        }

        hsv(float _h, float _s, float _v) :
            h(_h),
            s(_s),
            v(_v) {
        }

        hsv(const hsv &from) :
            h(from.h),
            s(from.s),
            v(from.v) {
        }
    
        explicit hsv(const rgb &from) {
            float hi = std::max(std::max(from.r, from.g), from.b);
            float lo = std::min(std::max(from.r, from.g), from.b);
            float d = hi - lo;

            h = 0.0f;
            s = 0.0f;
            v = hi;

            if ( ( v > 0.00001f ) &&
                 ( d > 0.00001f ) ) {
                s = d / v;
                if( hi == from.r ) {
                    h = (60.0f/360.0f) * (from.g - from.b) / d + (from.g < from.b ? 1.0f : 0.0f);
                }
                if( hi == from.g ) {
                    h = (60.0f/360.0f) * (from.b - from.r) / d + (120.0f/360.0f);
                }
                if( hi == from.b ) {
                    h = (60.0f/360.0f) * (from.r - from.g) / d + (240.0f/360.0f);
                }
            }
        }
    };
};

class led_control {
public:
    static void init();
    static void PerformV2MessageEffect(uint32_t color, bool remove = false);
    static void PerformV3MessageEffect(colors::rgb8 color, bool remove = false);

    static void PerformMessageColorDisplay(colors::rgb8 color, bool remove = false);
    static void PerformColorBirdDisplay(colors::rgb8 color, bool remove = false);
    static void PerformColorRingDisplay(colors::rgb8 color, bool remove = false);
    static void PerformFlashlight(colors::rgb8 color, bool remove = false);
};

#endif /* LEDS_H_ */
