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
#include "./leds.h"
#include "./emulator.h"

#include <atmel_start.h>

#include <array>
#include <random>
#include <limits>
#include <stdio.h>

#ifdef EMULATOR
#include <stdio.h>
#include <vector>
#endif  // #ifdef EMULATOR

#include "./model.h"
#include "./timeline.h"

static float signf(float x) {
	return (x > 0.0f) ? 1.0f : ( (x < 0.0f) ? -1.0f : 1.0f);
}

class pseudo_random {
public:
    
    void set_seed(uint32_t seed) {
        uint32_t i;
        a = 0xf1ea5eed, b = c = d = seed;
        for (i=0; i<20; ++i) {
            (void)get();
        }
    }

    #define rot(x,k) (((x)<<(k))|((x)>>(32-(k))))
    uint32_t get() {
        uint32_t e = a - rot(b, 27);
        a = b ^ rot(c, 17);
        b = c + d;
        c = d + e;
        d = e + a;
        return d;
    }

    float get(float lower, float upper) {
        return static_cast<float>(static_cast<double>(get()) * (static_cast<double>(upper-lower)/static_cast<double>(1LL<<32)) ) + lower;
    }

    int32_t get(int32_t lower, int32_t upper) {
        return (static_cast<int32_t>(get()) % (upper-lower)) + lower;
    }

private:
    uint32_t a; 
    uint32_t b; 
    uint32_t c; 
    uint32_t d; 

};

namespace colors {

    class rgb8out {
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

        rgb8out() :
            r(0),
            g(0),
            b(0),
            x(0){
        }
    
        rgb8out(const rgb8out &from) :
            r(from.r),
            g(from.g),
            b(from.b),
            x(0) {
        }

        explicit rgb8out(const rgb &from) {
            r = sat8(from.r * global_limit_factor);
            g = sat8(from.g * global_limit_factor);
            b = sat8(from.b * global_limit_factor);
            x = 0;
        }

        explicit rgb8out(uint8_t _r, uint8_t _g, uint8_t _b) :
            r(_r),
            g(_g),
            b(_b),
            x(0) {
        }
    
    private:
    
        float gamma28(float v) const { // approximate 2^2.8
            float v2 = v*v;
            return 0.2f*v2 + 0.8f*v*v2;
        }

        uint8_t sat8(const float v) const {
            return v < 0.0f ? uint8_t(0) : ( v > 1.0f ? uint8_t(0xFF) : uint8_t( gamma28(v) * 255.f ) );
        }

    };

    class hsp {

    public:
        float h;
        float s;
        float p;

        hsp() :
            h(0.0f),
            s(0.0f),
            p(0.0f) {
        }

        hsp(float _h, float _s, float _p) :
            h(_h),
            s(_s),
            p(_p) {
        }

        hsp(const hsp &from) :
            h(from.h),
            s(from.s),
            p(from.p) {
        }
    
        explicit hsp(const rgb &from) {
            RGBtoHSP(from.r, from.g, from.b, &h, &s, &p);
        }
    
    private:
        friend class rgb;

        const float Pr = 0.299f;
        const float Pg = 0.587f;
        const float Pb = 0.114f;

        void RGBtoHSP(float R, float G, float B, float *H, float *S, float *P) const {
          //  Calculate the Perceived brightness.
          *P = sqrtf(R * R * Pr + G * G * Pg + B * B * Pb);

          //  Calculate the Hue and Saturation.  (This part works
          //  the same way as in the HSV/B and HSL systems???.)
          if (R == G && R == B) {
            *H = 0.0f;
            *S = 0.0f;
            return;
          }
          if (R >= G && R >= B) { //  R is largest
            if (B >= G) {
              *H =        1.0f - 1.0f / 6.0f * (B - G) / (R - G);
              *S = 1.0f - G / R;
            } else {
              *H = 0.0f / 6.0f + 1.0f / 6.0f * (G - B) / (R - B);
              *S = 1.0f - B / R;
            }
          } else if (G >= R && G >= B) { //  G is largest
            if (R >= B) {
              *H = 2.0f / 6.0f - 1.0f / 6.0f * (R - B) / (G - B);
              *S = 1.0f - B / G;
            } else {
              *H = 2.0f / 6.0f + 1.0f / 6.0f * (B - R) / (G - R);
              *S = 1.0f - R / G;
            }
          } else { //  B is largest
            if (G >= R) {
              *H = 4.0f / 6.0f - 1.0f / 6.0f * (G - R) / (B - R);
              *S = 1.0f - R / B;
            } else {
              *H = 4.0f / 6.0f + 1.0f / 6.0f * (R - G) / (B - G);
              *S = 1.0f - G / B;
            }
          }
        }

        void HSPtoRGB(float H, float S, float P, float *R, float *G, float *B) const {

          float minOverMax = 1.0f - S;

          if (minOverMax > 0.0f) {
            if (H < 1.0f / 6.0f) { //  R>G>B
              H = 6.0f * (H - 0.0f / 6.0f);
              float part = 1.0f + H * (1.0f / minOverMax - 1.0f);
              *B = P / sqrtf(Pr / minOverMax / minOverMax + Pg * part * part + Pb);
              *R = (*B) / minOverMax;
              *G = (*B) + H * ((*R) - (*B));
            } else if (H < 2.0f / 6.0f) { //  G>R>B
              H = 6.0f * (-H + 2.0f / 6.0f);
              float part = 1.0f + H * (1.0f / minOverMax - 1.0f);
              *B = P / sqrtf(Pg / minOverMax / minOverMax + Pr * part * part + Pb);
              *G = (*B) / minOverMax;
              *R = (*B) + H * ((*G) - (*B));
            } else if (H < 3.0f / 6.0f) { //  G>B>R
              H = 6.0f * (H - 2.0f / 6.0f);
              float part = 1.0f + H * (1.0f / minOverMax - 1.0f);
              *R = P / sqrtf(Pg / minOverMax / minOverMax + Pb * part * part + Pr);
              *G = (*R) / minOverMax;
              *B = (*R) + H * ((*G) - (*R));
            } else if (H < 4.0f / 6.0f) { //  B>G>R
              H = 6.0f * (-H + 4.0f / 6.0f);
              float part = 1.0f + H * (1.0f / minOverMax - 1.0f);
              *R = P / sqrtf(Pb / minOverMax / minOverMax + Pg * part * part + Pr);
              *B = (*R) / minOverMax;
              *G = (*R) + H * ((*B) - (*R));
            } else if (H < 5.0f / 6.0f) { //  B>R>G
              H = 6.0f * (H - 4.0f / 6.0f);
              float part = 1.0f + H * (1.0f / minOverMax - 1.0f);
              *G = P / sqrtf(Pb / minOverMax / minOverMax + Pr * part * part + Pg);
              *B = (*G) / minOverMax;
              *R = (*G) + H * ((*B) - (*G));
            } else { //  R>B>G
              H = 6.0f * (-H + 1.0f       );
              float part = 1.0f + H * (1.0f / minOverMax - 1.0f);
              *G = P / sqrtf(Pr / minOverMax / minOverMax + Pb * part * part + Pg);
              *R = (*G) / minOverMax;
              *B = (*G) + H * ((*R) - (*G));
            }
          } else {
            if (H < 1.0f / 6.0f) { //  R>G>B
              H = 6.0f * (H - 0.0f / 6.0f);
              *R = sqrtf(P * P / (Pr + Pg * H * H));
              *G = (*R) * H;
              *B = 0.0f;
            } else if (H < 2.0f / 6.0f) { //  G>R>B
              H = 6.0f * (-H + 2.0f / 6.0f);
              *G = sqrtf(P * P / (Pg + Pr * H * H));
              *R = (*G) * H;
              *B = 0.0f;
            } else if (H < 3.0f / 6.0f) { //  G>B>R
              H = 6.0f * (H - 2.0f / 6.0f);
              *G = sqrtf(P * P / (Pg + Pb * H * H));
              *B = (*G) * H;
              *R = 0.0f;
            } else if (H < 4.0f / 6.0f) { //  B>G>R
              H = 6.0f * (-H + 4.0f / 6.0f);
              *B = sqrtf(P * P / (Pb + Pg * H * H));
              *G = (*B) * H;
              *R = 0.0f;
            } else if (H < 5.0f / 6.0f) { //  B>R>G
              H = 6.0f * (H - 4.0f / 6.0f);
              *B = sqrtf(P * P / (Pb + Pr * H * H));
              *R = (*B) * H;
              *G = 0.0f;
            } else { //  R>B>G
              H = 6.0f * (-H + 1.0f       );
              *R = sqrtf(P * P / (Pr + Pb * H * H));
              *B = (*R) * H;
              *G = 0.0f;
            }
          }
        };
    };

    rgb::rgb(const rgb8 &from) {
        r = static_cast<float>(from.r) * (1.0f/255.0f);
        g = static_cast<float>(from.g) * (1.0f/255.0f);
        b = static_cast<float>(from.b) * (1.0f/255.0f);
    }

    rgb::rgb(const rgb8out &from) {
        r = static_cast<float>(from.r) * (1.0f/255.0f);
        g = static_cast<float>(from.g) * (1.0f/255.0f);
        b = static_cast<float>(from.b) * (1.0f/255.0f);
    }

    static float value(  float const &p, float const &q, float const &t ) {
        if( t < 1.0f/6.0f ) return ( p + ( q - p ) * 6.0f * t );
        if( t < 1.0f/2.0f ) return ( q );
        if( t < 2.0f/3.0f ) return ( p + ( q - p ) * ( ( 2.0f / 3.0f ) - t ) * 6.0f );
        return p ;
    }

    rgb::rgb(const uint32_t color) {
        r = static_cast<float>((color>>16)&0xFF) * (1.0f/255.0f);
        g = static_cast<float>((color>> 8)&0xFF) * (1.0f/255.0f);
        b = static_cast<float>((color>> 0)&0xFF) * (1.0f/255.0f);
    }

    rgb::rgb(const hsl &from) {
        if (from.s == 0.0f) {
            r = from.l;
            g = from.l;
            b = from.l;
        } else {
            float q = from.l < 0.5f ? (from.l * (1.0f + from.s)) : ( from.l + from.s - from.l * from.s );
            float p = 2.0f * from.l - q;
            r = value( p, q, from.h + ( 1.5f / 3.5f ) + ( ( 2.5f / 3.5f ) < from.h ? -1.5f : +0.5f ) );
            g = value( p, q, from.h );
            b = value( p, q, from.h - ( 1.5f / 3.5f ) + ( from.h < ( 1.5f / 3.5f ) ? +1.5f : +0.5f ) );
        }
    }

    rgb::rgb(const hsv &from) {
        float v = from.v;
        float h = from.h;
        float s = from.s;

        int32_t rd = static_cast<int32_t>( 6.0f * h );
        float f = h * 6.0f - rd;
        float p = v * (1.0f - s);
        float q = v * (1.0f - f * s);
        float t = v * (1.0f - (1.0f - f) * s);

        switch ( rd  % 6 ) {
            case 0: r = v; g = t; b = p; break;
            case 1: r = q; g = v; b = p; break;
            case 2: r = p; g = v; b = t; break;
            case 3: r = p; g = q; b = v; break;
            case 4: r = t; g = p; b = v; break;
            case 5: r = v; g = p; b = q; break;
        }
    }

    rgb::rgb(const hsp &from) {
        from.HSPtoRGB(from.h, from.s, from.p, &r, &g, &b);
    }

    static rgb8out ip(const rgb8out &a, const rgb8out &b, float i) {
        if ( i <= 0.0f ) {
            return a;
        } else if ( i >= 1.0f) {
            return b;
        } else {
            int32_t ii = static_cast<int32_t>(i * 256.0f);
            return rgb8out(
                static_cast<uint8_t>(( ( a.r * ( 256 - ii ) ) + ( b.r * ii ) ) / 256),
                static_cast<uint8_t>(( ( a.g * ( 256 - ii ) ) + ( b.g * ii ) ) / 256),
                static_cast<uint8_t>(( ( a.b * ( 256 - ii ) ) + ( b.b * ii ) ) / 256)
            );
        }
    }

#if 0
    static rgb ip(const rgb &a, const rgb &b, float i) {
        if ( i <= 0.0f ) {
            return a;
        } else if ( i >= 1.0f) {
            return b;
        } else {
            return rgb(a * (1.0f - i) + b *i);
        }
    }

    static hsl ip(const hsl &a, const hsl &b, float i) {
        if ( i <= 0.0f ) {
            return a;
        } else if ( i >= 1.0f) {
            return b;
        } else {
            return hsl(a.h * (1.0f - i) + b.h * i,
                       a.s * (1.0f - i) + b.s * i,
                       a.l * (1.0f - i) + b.l * i);
        }
    }

    static hsv ip(const hsv &a, const hsv &b, float i) {
        if ( i <= 0.0f ) {
            return a;
        } else if ( i >= 1.0f) {
            return b;
        } else {
            return hsv(a.h * (1.0f - i) + b.h * i,
                       a.s * (1.0f - i) + b.s * i,
                       a.v * (1.0f - i) + b.v * i);
        }
    }
#endif  // #if 0

}

namespace geom {
    class float4 {
    public:
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;
        
        float4() {
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
            w = 0.0f;
        }

        float4(uint32_t c, float a) {
            this->x = ((c>>16)&0xFF)*(1.0f/255.0f);
            this->y = ((c>> 8)&0xFF)*(1.0f/255.0f);
            this->z = ((c>> 0)&0xFF)*(1.0f/255.0f);
            this->w = a;
        }

        float4(float v) {
            this->x = v;
            this->y = v;
            this->z = v;
            this->w = v;
        }

        float4(float _x, float _y, float _z, float _w = 0.0f) {
            this->x = _x;
            this->y = _y;
            this->z = _z;
            this->w = _w;
        }

        float4(const colors::rgb &from) {
            this->x = from.r;
            this->y = from.g;
            this->z = from.b;
            this->w = 1.0f;
        }
        
        operator colors::rgb() {
            return colors::rgb(this->x, this->y, this->z);
        }

        float4 &operator+=(float v) {
            this->x += v;
            this->y += v;
            this->z += v;
            this->w += v;
            return *this;
        }

        float4 &operator-=(float v) {
            this->x -= v;
            this->y -= v;
            this->z -= v;
            this->w -= v;
            return *this;
        }

        float4 &operator*=(float v) {
            this->x *= v;
            this->y *= v;
            this->z *= v;
            this->w *= v;
            return *this;
        }

        float4 &operator/=(float v) {
            this->x /= v;
            this->y /= v;
            this->z /= v;
            this->w /= v;
            return *this;
        }

        float4 &operator%=(float v) {
            this->x = fmodf(this->x, v);
            this->y = fmodf(this->y, v);
            this->z = fmodf(this->z, v);
            this->w = fmodf(this->w, v);
            return *this;
        }
        
        float4 &operator+=(const float4 &b) {
            this->x += b.x;
            this->y += b.y;
            this->z += b.z;
            this->w += b.w;
            return *this;
        }

        float4 &operator-=(const float4 &b) {
            this->x += b.x;
            this->y += b.y;
            this->z += b.z;
            this->w += b.w;
            return *this;
        }

        float4 &operator*=(const float4 &b) {
            this->x += b.x;
            this->y += b.y;
            this->z += b.z;
            this->w += b.w;
            return *this;
        }

        float4 &operator/=(const float4 &b) {
            this->x += b.x;
            this->y += b.y;
            this->z += b.z;
            this->w += b.w;
            return *this;
        }

        float4 &operator%=(const float4 &b) {
            this->x = fmodf(this->x, b.x);
            this->y = fmodf(this->y, b.y);
            this->z = fmodf(this->z, b.z);
            this->w = fmodf(this->w, b.w);
            return *this;
        }

        float4 operator-() {
            return float4(-this->x,
                          -this->y,
                          -this->z,
                          -this->w);
        }

        float4 operator+() {
            return float4(+this->x,
                          +this->y,
                          +this->z,
                          +this->w);
        }

        float4 operator+(float v) {
            return float4(this->x+v,
                          this->y+v,
                          this->z+v,
                          this->w+v);
        }

        float4 operator-(float v) {
            return float4(this->x-v,
                          this->y-v,
                          this->z-v,
                          this->w-v);
        }

        float4 operator*(float v) {
            return float4(this->x*v,
                          this->y*v,
                          this->z*v,
                          this->w*v);
        }

        float4 operator/(float v) {
            return float4(this->x/v,
                          this->y/v,
                          this->z/v,
                          this->w/v);
        }

        float4 operator%(float v) {
            return float4(fmodf(this->x,v),
                          fmodf(this->y,v),
                          fmodf(this->z,v),
                          fmodf(this->w,v));
        }
        
        float4 operator+(const float4 &b) {
            return float4(this->x+b.x,
                          this->y+b.y,
                          this->z+b.z,
                          this->w+b.w);
        }

        float4 operator-(const float4 &b) {
            return float4(this->x-b.x,
                          this->y-b.y,
                          this->z-b.z,
                          this->w-b.w);
        }

        float4 operator*(const float4 &b) {
            return float4(this->x*b.x,
                          this->y*b.y,
                          this->z*b.z,
                          this->w*b.w);
        }

        float4 operator/(const float4 &b) {
            return float4(this->x/b.x,
                          this->y/b.y,
                          this->z/b.z,
                          this->w/b.w);
        }

        float4 operator%(const float4 &b) {
            return float4(fmodf(this->x,b.x),
                          fmodf(this->y,b.y),
                          fmodf(this->z,b.z),
                          fmodf(this->w,b.w));
        }
        
        float len() {
            return sqrtf(x*x + y*y + z*z);
        }

        static float len(const float4 &a) {
            return sqrtf(a.x*a.x + a.y*a.y + a.z*a.z);
        }
        
        float dist(const float4 &b) {
            float xd = fabsf(this->x - b.x);
            float yd = fabsf(this->y - b.y);
            float zd = fabsf(this->z - b.z);
            return sqrtf(xd*xd + yd*yd + zd*zd);
        }

        static float dist(const float4 &a, const float4 &b) {
            float xd = fabsf(a.x - b.x);
            float yd = fabsf(a.y - b.y);
            float zd = fabsf(a.z - b.z);
            return sqrtf(xd*xd + yd*yd + zd*zd);
        }

        float4 min(const float4 &b) {
            return float4(std::min(this->x, b.x),
                          std::min(this->y, b.y),
                          std::min(this->z, b.z),
                          std::min(this->w, b.w));
        }

        static float4 min(const float4 &a, const float4 &b) {
            return float4(std::min(a.x, b.x),
                          std::min(a.y, b.y),
                          std::min(a.z, b.z),
                          std::min(a.w, b.w));
        }

        float4 max(const float4 &b) {
            return float4(std::max(this->x, b.x),
                          std::max(this->y, b.y),
                          std::max(this->z, b.z),
                          std::max(this->w, b.w));
        }

        static float4 max(const float4 &a, const float4 &b) {
            return float4(std::max(a.x, b.x),
                          std::max(a.y, b.y),
                          std::max(a.z, b.z),
                          std::max(a.w, b.w));
        }
        
        float4 abs() {
            return float4(fabsf(this->x),
                          fabsf(this->y),
                          fabsf(this->z),
                          fabsf(this->w));
        }

        static float4 abs(const float4 &a) {
            return float4(fabsf(a.x),
                          fabsf(a.y),
                          fabsf(a.z),
                          fabsf(a.w));
        }

        float4 xx00() {
            return float4(this->x, this->x, 0.0, 0.0);
        }

        float4 yy00() {
            return float4(this->y, this->y, 0.0, 0.0);
        }

        float4 zz00() {
            return float4(this->z, this->z, 0.0, 0.0);
        }
        
        float4 xy00() {
            return float4(this->x, this->y, 0.0, 0.0);
        }

        float4 yx00() {
            return float4(this->y, this->x, 0.0, 0.0);
        }

        float4 xz00() {
            return float4(this->x, this->z, 0.0, 0.0);
        }

        float4 zx00() {
            return float4(this->z, this->x, 0.0, 0.0);
        }

        float4 yz00() {
            return float4(this->y, this->z, 0.0, 0.0);
        }

        float4 zy00() {
            return float4(this->z, this->y, 0.0, 0.0);
        }

        float4 sqrt() {
            return float4(sqrtf(this->x),
                          sqrtf(this->y),
                          sqrtf(this->z),
                          sqrtf(this->w));
        }

        static float4 sqrt(const float4 &a) {
            return float4(sqrtf(a.x),
                          sqrtf(a.y),
                          sqrtf(a.z),
                          sqrtf(a.w));
        }
        
        float4 rotate2d(float angle) {
            return float4(
                this->x * cosf(angle) - this->y * sinf(angle),
                this->y * cosf(angle) + this->x * sinf(angle),
                this->z,
                this->w);
        }
        
        float4 reflect() {
            return float4(
                reflect(this->x),
                reflect(this->y),
                reflect(this->z),
                reflect(this->w));
        }
        
        float4 rsqrt() {
            return float4((this->x != 0.0f) ? (1.0f/sqrtf(this->x)) : 0.0f,
                          (this->y != 0.0f) ? (1.0f/sqrtf(this->y)) : 0.0f,
                          (this->z != 0.0f) ? (1.0f/sqrtf(this->z)) : 0.0f,
                          (this->w != 0.0f) ? (1.0f/sqrtf(this->w)) : 0.0f);
        }

        static float4 rsqrt(const float4 &a) {
            return float4((a.x != 0.0f) ? (1.0f/sqrtf(a.x)) : 0.0f,
                          (a.y != 0.0f) ? (1.0f/sqrtf(a.y)) : 0.0f,
                          (a.z != 0.0f) ? (1.0f/sqrtf(a.z)) : 0.0f,
                          (a.w != 0.0f) ? (1.0f/sqrtf(a.w)) : 0.0f);
        }

        float4 rcp() {
            return float4((this->x != 0.0f) ? (1.0f/this->x) : 0.0f,
                          (this->y != 0.0f) ? (1.0f/this->y) : 0.0f,
                          (this->z != 0.0f) ? (1.0f/this->z) : 0.0f,
                          (this->w != 0.0f) ? (1.0f/this->w) : 0.0f);
        }

        static float4 rcp(const float4 &a) {
            return float4((a.x != 0.0f) ? (1.0f/a.x) : 0.0f,
                          (a.y != 0.0f) ? (1.0f/a.y) : 0.0f,
                          (a.z != 0.0f) ? (1.0f/a.z) : 0.0f,
                          (a.w != 0.0f) ? (1.0f/a.w) : 0.0f);
        }

        float4 lerp(const float4 &b, float v) {
            return float4(
              this->x * (1.0f - v) + b.x * v, 
              this->y * (1.0f - v) + b.y * v, 
              this->z * (1.0f - v) + b.z * v, 
              this->w * (1.0f - v) + b.w * v);
        }

        static float4 lerp(const float4 &a, const float4 &b, float v) {
            return float4(
              a.x * (1.0f - v) + b.x * v, 
              a.y * (1.0f - v) + b.y * v, 
              a.z * (1.0f - v) + b.z * v, 
              a.w * (1.0f - v) + b.w * v);
        }

		float4 clamp() {
			return float4(
				std::min(std::max(this->x, 0.0f), 1.0f),
				std::min(std::max(this->y, 0.0f), 1.0f),
				std::min(std::max(this->z, 0.0f), 1.0f),
				std::min(std::max(this->w, 0.0f), 1.0f)
			);
		}

        static float4 zero() {
            return float4(0.0f, 0.0f, 0.0f, 0.0f);
        }

        static float4 one() {
            return float4(1.0f, 1.0f, 1.0f, 1.0f);
        }

        static float4 half() {
            return float4(0.5f, 0.5f, 0.5f, 0.5f);
        }

    private:
    
        float reflect(float i) {
            i = fabsf(i);
            if ((static_cast<int32_t>(i) & 1) == 0) {
                i = fmodf(i, 1.0f);
            } else {
                i = fmodf(i, 1.0f);
                i = 1.0f - i;
            }
            return i;
        }
    };
}

namespace colors {
    class gradient {

        static constexpr size_t colors_n = 256;
        static constexpr float colors_mul = 255.0;
        static constexpr size_t colors_mask = 0xFF;
        colors::rgb colors[colors_n];
        bool initialized = false;
        
    public:
    
        bool check_init() {
            return !initialized;
        }
        
        void init(const geom::float4 stops[], size_t n) {
            initialized = true;
            for (size_t c = 0; c < colors_n; c++) {
                float f = static_cast<float>(c) / static_cast<float>(colors_n - 1); 
                geom::float4 a = stops[0];
                geom::float4 b = stops[1];
                if (n > 2) {
                  for (int32_t d = static_cast<int32_t>(n-2); d >= 0 ; d--) {
                      if ( f >= (stops[d].w) ) {
                        a = stops[d+0];
                        b = stops[d+1];
                        break;
                      }
                  }
                }
                f -= a.w;
                f /= b.w - a.w;
                colors[c] = a.lerp(b,f);
            }
        }
        
        geom::float4 repeat(float i) {
            i = fmodf(i, 1.0f);
            i *= colors_mul;
            return geom::float4::lerp(colors[(static_cast<size_t>(i))&colors_mask], colors[(static_cast<size_t>(i)+1)&colors_mask], fmodf(i, 1.0f));
        }

        geom::float4 reflect(float i) {
            i = fabsf(i);
            if ((static_cast<int32_t>(i) & 1) == 0) {
                i = fmodf(i, 1.0f);
            } else {
                i = fmodf(i, 1.0f);
                i = 1.0f - i;
            }
            i *= colors_mul;
            return geom::float4::lerp(colors[(static_cast<size_t>(i))&colors_mask], colors[(static_cast<size_t>(i)+1)&colors_mask], fmodf(i, 1.0f));
        }

        geom::float4 clamp(float i) {
            if (i <= 0.0f) {
                return colors[0];
            }
            if (i >= 1.0f) {
                return colors[colors_n-1];
            }
            i *= colors_mul;
            return geom::float4::lerp(colors[(static_cast<size_t>(i))&colors_mask], colors[(static_cast<size_t>(i)+1)&colors_mask], fmodf(i, 1.0f));
        }
    };
};

#ifndef EMULATOR
static void _qspi_memcpy(volatile uint8_t *dst, uint8_t *src, uint32_t count)
{
	(void)dst;
	(void)src;
    if (count < 1) {
        return;
    }
#if 0
    if (count >= 64) {
        __asm volatile (
            "1:\n\t"
            "vldm %[src]!, {s0-s15}\n\t"
            "vstm %[dst]!, {s0-s15}\n\t"
            "subs %[count], #64\n\t"
            "bge 1b\n\t"
            :
            : [dst] "r" (dst), [src] "r" (src), [count] "r" (count)
            : "r1", 
              "s0", "s1", "s2", "s3",
              "s4", "s5", "s6", "s7",
              "s8", "s9", "s10", "s11",
              "s12", "s13", "s14", "s15",
              "cc", "memory"
        );
        count &= ~0x3FUL;
        if (count < 1) {
            return;
        }
    }
#endif  // #if 0
    __asm volatile (
        "1:\n\t"
        "ldrb r1, [%[src]], #1\n\t"
        "strb r1, [%[dst]], #1\n\t"
        "subs %[count], #1\n\t"
        "bge 1b\n\t"
        :
        : [dst] "r" (dst), [src] "r" (src), [count] "r" (count)
        : "r1", "cc", "memory"
    );
}
#endif  // #ifndef EMULATOR

class led_bank {
    static constexpr size_t ws2812_commit_time = 384;
    static constexpr size_t ws2812_rails = 4;

    static constexpr size_t leds_rings_n = 16;

    static constexpr size_t leds_components = 3;

    static constexpr size_t leds_buffer_size = ws2812_commit_time * ws2812_rails * 2 + ( leds_rings_n + 1 ) * ws2812_rails * leds_components * 8;

    colors::rgb8out leds_centr[2];

    colors::rgb8out leds_outer[2][leds_rings_n];
    colors::rgb8out leds_inner[2][leds_rings_n];

    bool initialized = false;

    pseudo_random random;

public:

    static led_bank &instance() {
        static led_bank leds;
        if (!leds.initialized) {
            leds.initialized = true;
            leds.init();
        }
        return leds;
    }
    
    const std::array<geom::float4, 33> &ledpos() {
        static std::array<geom::float4, 33> leds;
        static bool init = false;
        if (!init) {
            init = true;
            leds[ 0] = geom::float4(-0.00000000f,-1.00000000f,+0.00000000f,+0.00000000f);
            leds[ 1] = geom::float4(-0.38268343f,-0.92387953f,+0.00000000f,+0.00000000f);
            leds[ 2] = geom::float4(-0.70710678f,-0.70710678f,+0.00000000f,+0.00000000f);
            leds[ 3] = geom::float4(-0.92387953f,-0.38268343f,+0.00000000f,+0.00000000f);
            leds[ 4] = geom::float4(-1.00000000f,-0.00000000f,+0.00000000f,+0.00000000f);
            leds[ 5] = geom::float4(-0.92387953f,+0.38268343f,+0.00000000f,+0.00000000f);
            leds[ 6] = geom::float4(-0.70710678f,+0.70710678f,+0.00000000f,+0.00000000f);
            leds[ 7] = geom::float4(-0.38268343f,+0.92387953f,+0.00000000f,+0.00000000f);
            leds[ 8] = geom::float4(-0.00000000f,+1.00000000f,+0.00000000f,+0.00000000f);
            leds[ 9] = geom::float4(+0.38268343f,+0.92387953f,+0.00000000f,+0.00000000f);
            leds[10] = geom::float4(+0.70710678f,+0.70710678f,+0.00000000f,+0.00000000f);
            leds[11] = geom::float4(+0.92387953f,+0.38268343f,+0.00000000f,+0.00000000f);
            leds[12] = geom::float4(+1.00000000f,+0.00000000f,+0.00000000f,+0.00000000f);
            leds[13] = geom::float4(+0.92387953f,-0.38268343f,+0.00000000f,+0.00000000f);
            leds[14] = geom::float4(+0.70710678f,-0.70710678f,+0.00000000f,+0.00000000f);
            leds[15] = geom::float4(+0.38268343f,-0.92387953f,+0.00000000f,+0.00000000f);

            leds[16] = geom::float4(-0.00000000f,-0.50000000f,+0.00000000f,+0.00000000f);
            leds[17] = geom::float4(-0.19134172f,-0.46193977f,+0.00000000f,+0.00000000f);
            leds[18] = geom::float4(-0.35355339f,-0.35355339f,+0.00000000f,+0.00000000f);
            leds[19] = geom::float4(-0.46193977f,-0.19134172f,+0.00000000f,+0.00000000f);
            leds[20] = geom::float4(-0.50000000f,-0.00000000f,+0.00000000f,+0.00000000f);
            leds[21] = geom::float4(-0.46193977f,+0.19134172f,+0.00000000f,+0.00000000f);
            leds[22] = geom::float4(-0.35355339f,+0.35355339f,+0.00000000f,+0.00000000f);
            leds[23] = geom::float4(-0.19134172f,+0.46193977f,+0.00000000f,+0.00000000f);
            leds[24] = geom::float4(-0.00000000f,+0.50000000f,+0.00000000f,+0.00000000f);
            leds[25] = geom::float4(+0.19134172f,+0.46193977f,+0.00000000f,+0.00000000f);
            leds[26] = geom::float4(+0.35355339f,+0.35355339f,+0.00000000f,+0.00000000f);
            leds[27] = geom::float4(+0.46193977f,+0.19134172f,+0.00000000f,+0.00000000f);
            leds[28] = geom::float4(+0.50000000f,+0.00000000f,+0.00000000f,+0.00000000f);
            leds[29] = geom::float4(+0.46193977f,-0.19134172f,+0.00000000f,+0.00000000f);
            leds[30] = geom::float4(+0.35355339f,-0.35355339f,+0.00000000f,+0.00000000f);
            leds[31] = geom::float4(+0.19134172f,-0.46193977f,+0.00000000f,+0.00000000f);

            leds[32] = geom::float4(+0.00000000f,+0.00000000f,+0.00000000f,+0.00000000f);
        }
        return leds;
    }

    void init() {
        qspi_sync_enable(&QUAD_SPI_0);

        static Timeline::Span span;

        static uint32_t current_effect = 0;
        static uint32_t previous_effect = 0;
        static double switch_time = 0;
        
        random.set_seed(Model::instance().RandomUInt32());

        span.type = Timeline::Span::Effect;
        span.time = 0;
        span.duration = std::numeric_limits<double>::infinity();

        span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {

            if ( current_effect != Model::instance().Effect() ) {
                previous_effect = current_effect;
                current_effect = Model::instance().Effect();
                switch_time = Model::instance().Time();
            }

            auto calc_effect = [=] (uint32_t effect) mutable {
                switch (effect) {
                    case 0:
                        black();
                    break;
                    case 1:
                        static_color();
                    break;
                    case 2:
                        rgb_band();
                    break;
                    case 3:
                        color_walker();
                    break;
                    case 4:
                        light_walker();
                    break;
                    case 5:
                        rgb_glow();
                    break;
                    case 6:
                        lightning();
                    break;
                    case 7:
                        lightning_crazy();
                    break;
                    case 8:
                        sparkle();
                    break;
                    case 9:
                        rando();
                    break;
                    case 10:
                        red_green();
                    break;
                    case 11:
                        brilliance();
                    break;
                    case 12:
                        highlight();
                    break;
                    case 13:
                        autumn();
                    break;
                    case 14:
                        heartbeat();
                    break;
                    case 15:
                        moving_rainbow();
                    break;
                    case 16:
                        twinkle();
                    break;
                    case 17:
                        twinkly();
                    break;
                    case 18:
                        randomfader();
                    break;
                    case 19:
                        chaser();
                    break;
                    case 20:
                        brightchaser();
                    break;
                    case 21:
                        gradient();
                    break;
                    case 22:
                        overdrive();
                    break;
                    case 23:
                        ironman();
                    break;
                    case 24:
                        sweep();
                    break;
                    case 25:
                        sweephighlight();
                    break;
                    case 26:
                        rainbow_circle();
                    break;
                    case 27:
                        rainbow_grow();
                    break;
                    case 28:
                        rotor();
                    break;
                    case 29:
                        rotor_sparse();
                    break;
                    case 30:
                        fullcolor();
                    break;
                    case 31:
                        flip_colors();
                    break;
                }
            };

            double blend_duration = 0.5;
            double now = Model::instance().Time();
            
            if ((now - switch_time) < blend_duration) {
                calc_effect(previous_effect);

                colors::rgb8out leds_centr_prev[2];
                colors::rgb8out leds_outer_prev[2][leds_rings_n];
                colors::rgb8out leds_inner_prev[2][leds_rings_n];

                memcpy(leds_centr_prev, leds_centr, sizeof(leds_centr));
                memcpy(leds_outer_prev, leds_outer, sizeof(leds_outer));
                memcpy(leds_inner_prev, leds_inner, sizeof(leds_inner));

                calc_effect(current_effect);

                float blend = static_cast<float>(now - switch_time) * (1.0f / static_cast<float>(blend_duration));

                for (size_t c = 0; c < leds_rings_n; c++) {
                    leds_outer[0][c] = colors::ip(leds_outer_prev[0][c], leds_outer[0][c], blend);
                    leds_outer[1][c] = colors::ip(leds_outer_prev[1][c], leds_outer[1][c], blend);
                    leds_inner[0][c] = colors::ip(leds_inner_prev[0][c], leds_inner[0][c], blend);
                    leds_inner[1][c] = colors::ip(leds_inner_prev[1][c], leds_inner[1][c], blend);
                }

                leds_centr[0] = colors::ip(leds_centr_prev[0], leds_centr[0], blend);
                leds_centr[1] = colors::ip(leds_centr_prev[1], leds_centr[1], blend);

            } else {
                calc_effect(current_effect);
            }

        };
        span.commitFunc = [=](Timeline::Span &) {
            led_bank::instance().update_leds();
        };

        Timeline::instance().Add(span);
        current_effect = Model::instance().Effect();
        switch_time = Model::instance().Time();
    }

    void enable_leds() {
        gpio_set_pin_level(ENABLE_E, true);
        gpio_set_pin_level(ENABLE_O, true);
    }

    void disable_leds() {
        gpio_set_pin_level(ENABLE_E, false);
        gpio_set_pin_level(ENABLE_O, false);
    }

    void update_leds(bool hide_non_covered = true) {

        enable_leds();

        static const uint8_t conv_lookup[16][4] = {
            {   0b11111111, 0b00000000, 0b00000000, 0b00000000 }, // 0b0000
            {   0b11111111, 0b00010001, 0b00010000, 0b00000000 }, // 0b0001
            {   0b11111111, 0b00100010, 0b00100000, 0b00000000 }, // 0b0010
            {   0b11111111, 0b00110011, 0b00110000, 0b00000000 }, // 0b0011
            {   0b11111111, 0b01000100, 0b01000000, 0b00000000 }, // 0b0100
            {   0b11111111, 0b01010101, 0b01010000, 0b00000000 }, // 0b0101
            {   0b11111111, 0b01101100, 0b01100000, 0b00000000 }, // 0b0110
            {   0b11111111, 0b01110111, 0b01110000, 0b00000000 }, // 0b0111
            {   0b11111111, 0b10001000, 0b10000000, 0b00000000 }, // 0b1000
            {   0b11111111, 0b10011001, 0b10010000, 0b00000000 }, // 0b1001
            {   0b11111111, 0b10101010, 0b10100000, 0b00000000 }, // 0b1010
            {   0b11111111, 0b10111011, 0b10110000, 0b00000000 }, // 0b1011
            {   0b11111111, 0b11001100, 0b11000000, 0b00000000 }, // 0b1100
            {   0b11111111, 0b11011101, 0b11010000, 0b00000000 }, // 0b1101
            {   0b11111111, 0b11101110, 0b11100000, 0b00000000 }, // 0b1110
            {   0b11111111, 0b11111111, 0b11110000, 0b00000000 }, // 0b1111
        };


        static const uint8_t disabled_inner_leds_top_off[16] = {
            0x1, 0x0, 0x0, 0x1,
            0x0, 0x0, 0x0, 0x0,
            0x1, 0x0, 0x0, 0x0,
            0x0, 0x1, 0x0, 0x0
        };

        static const uint8_t disabled_inner_leds_bottom_off[16] = {
            0x1, 0x0, 0x0, 0x1,
            0x0, 0x0, 0x0, 0x0,
            0x1, 0x0, 0x0, 0x0,
            0x0, 0x1, 0x0, 0x0
        };

        static const uint8_t disabled_inner_leds_top_all_on[16] = {
            0x1, 0x1, 0x1, 0x1,
            0x1, 0x1, 0x1, 0x1,
            0x1, 0x1, 0x1, 0x1,
            0x1, 0x1, 0x1, 0x1,
        };

        static const uint8_t disabled_inner_leds_bottom_all_on[16] = {
            0x1, 0x1, 0x1, 0x1,
            0x1, 0x1, 0x1, 0x1,
            0x1, 0x1, 0x1, 0x1,
            0x1, 0x1, 0x1, 0x1,
        };

        const uint8_t *disabled_inner_leds_top = disabled_inner_leds_top_all_on;
        const uint8_t *disabled_inner_leds_bottom = disabled_inner_leds_bottom_all_on;

        if (hide_non_covered) {
            disabled_inner_leds_top = disabled_inner_leds_top_off;
            disabled_inner_leds_bottom = disabled_inner_leds_bottom_off;
        }

        int32_t brightness = static_cast<int32_t>(Model::instance().Brightness() * 256);

        size_t buf_pos = 0;
        static uint8_t buffer[leds_buffer_size];

        for (size_t c = 0; c < ws2812_commit_time; c++) {
            buffer[buf_pos++] = 0;
            buffer[buf_pos++] = 0;
            buffer[buf_pos++] = 0;
            buffer[buf_pos++] = 0;
        }

        auto transfer4 = [](uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t *buf, size_t &bp, int32_t bright) {
            p0 = static_cast<uint8_t>(( p0 * bright ) / 256);
            p1 = static_cast<uint8_t>(( p1 * bright ) / 256);
            p2 = static_cast<uint8_t>(( p2 * bright ) / 256);
            p3 = static_cast<uint8_t>(( p3 * bright ) / 256);

            for(int32_t d = 7; d >=0; d--) {
                const uint8_t *src = conv_lookup[
                ((p0&(1<<d))?0x8:0x0)|
                ((p1&(1<<d))?0x4:0x0)|
                ((p2&(1<<d))?0x2:0x0)|
                ((p3&(1<<d))?0x1:0x0)];
                buf[bp++] = *src++;
                buf[bp++] = *src++;
                buf[bp++] = *src++;
                buf[bp++] = *src++;
            }
        };

        for (size_t c = 0; c < leds_rings_n; c++) {
            transfer4(leds_inner[0][c].g * disabled_inner_leds_top[c],
                      leds_outer[0][c].g,
                      leds_inner[1][c].g * disabled_inner_leds_bottom[c],
                      leds_outer[1][c].g,
                      buffer, buf_pos, brightness);
            transfer4(leds_inner[0][c].r * disabled_inner_leds_top[c],
                      leds_outer[0][c].r,
                      leds_inner[1][c].r * disabled_inner_leds_bottom[c],
                      leds_outer[1][c].r,
                      buffer, buf_pos, brightness);
            transfer4(leds_inner[0][c].b * disabled_inner_leds_top[c],
                      leds_outer[0][c].b,
                      leds_inner[1][c].b * disabled_inner_leds_bottom[c],
                      leds_outer[1][c].b,
                      buffer, buf_pos, brightness);
        }

        auto transfer2 = [](uint8_t p0, uint8_t p1, uint8_t *buf, size_t &bp, int32_t bright) {
            p0 = static_cast<uint8_t>(( p0 * bright ) / 256);
            p1 = static_cast<uint8_t>(( p1 * bright ) / 256);
            for(int32_t d = 7; d >=0; d--) {
                const uint8_t *src = conv_lookup[
                ((p0&(1<<d))?0x8:0x0)|
                ((p0&(1<<d))?0x4:0x0)|
                ((p1&(1<<d))?0x2:0x0)|
                ((p1&(1<<d))?0x1:0x0)];
                buf[bp++] = *src++;
                buf[bp++] = *src++;
                buf[bp++] = *src++;
                buf[bp++] = *src++;
            }
        };

        transfer2(leds_centr[0].g,
                  leds_centr[1].g,
                  buffer, buf_pos, brightness);
        transfer2(leds_centr[0].r,
                  leds_centr[1].r,
                  buffer, buf_pos, brightness);
        transfer2(leds_centr[0].b,
                  leds_centr[1].b,
                  buffer, buf_pos, brightness);
    
        for (size_t c = 0; c < ws2812_commit_time; c++) {
            buffer[buf_pos++] = 0;
            buffer[buf_pos++] = 0;
            buffer[buf_pos++] = 0;
            buffer[buf_pos++] = 0;
        }

#ifndef EMULATOR
        static bool pending = false;

        struct _qspi_command cmd;
        memset(&cmd, 0, sizeof(cmd));
        cmd.inst_frame.bits.width = QSPI_INST4_ADDR4_DATA4;
        cmd.inst_frame.bits.data_en = 1;
        cmd.inst_frame.bits.tfr_type = QSPI_WRITE_ACCESS;
    

        if (pending) {
            hri_qspi_write_CTRLA_reg(QSPI, QSPI_CTRLA_ENABLE | QSPI_CTRLA_LASTXFER);

            while (!hri_qspi_get_INTFLAG_INSTREND_bit(QSPI)) { };

            hri_qspi_clear_INTFLAG_INSTREND_bit(QSPI);

            pending = false;
        }

        hri_qspi_write_INSTRFRAME_reg(QSPI, cmd.inst_frame.word);
        volatile uint8_t *qspi_mem = reinterpret_cast<volatile uint8_t *>(QSPI_AHB);

        __disable_irq();
        
        _qspi_memcpy(qspi_mem, buffer, buf_pos);

        __DSB();
        __ISB();

        __enable_irq();

        pending = true;
#endif  // #ifndef EMULATOR

#ifdef EMULATOR
        auto print_leds = [](int32_t pos_x, int32_t pos_y, const std::vector<colors::rgb> &leds) {
            std::lock_guard<std::recursive_mutex> lock(g_print_mutex);

            const char *layout = 

    /////////01234567890123456789012345
            "            00            " // 0
            "      01          15      " // 1
            "                          " // 2
            "  02        16       14   " // 3
            "        17      31        " // 4
            " 03   18          30   13 " // 5
            "     19            29     " // 6
            "04   20     32     28   12" // 7
            "     21            27     " // 8
            " 05   22          26   11 " // 9
            "        23      25        " // 0
            "  06        24       10   " // 1
            "                          " // 2
            "      07          09      " // 3
            "            08            ";// 4

            const int w = 26;
            const int h = 15;
        
            for(int y=0; y<h; y++) {
                printf("\x1b[%d;%df",pos_y+y+1,pos_x);
                for (int x=0; x<w; x++) {
                    char ch0 = layout[y*w + x];
                    if (ch0 == ' ') {
                        putchar(' ');
                    } else {
                        char ch1 = layout[y*w + x + 1];
                        char str[3];
                        str[0] = ch0;
                        str[1] = ch1;
                        str[2] = 0;
                        int32_t n = atoi(str);
                        if ( n >= 0 && n < int(leds.size())) {
#if 0 // def __APPLE__
                            printf("\x1b[48;5;%dm  \x1b[0m", 16 + 
                                static_cast<int>(std::min(std::max(leds[static_cast<size_t>(n)].r, 0.0f), 1.0f) *   5.0f)*36+ 
                                static_cast<int>(std::min(std::max(leds[static_cast<size_t>(n)].g, 0.0f), 1.0f) *   5.0f)* 6+ 
                                static_cast<int>(std::min(std::max(leds[static_cast<size_t>(n)].b, 0.0f), 1.0f) *   5.0f)* 1);
#else
                            printf("\x1b[48;2;%d;%d;%dm  \x1b[0m", 
                                static_cast<int>(std::min(std::max(leds[static_cast<size_t>(n)].r, 0.0f), 1.0f) * 255.0f), 
                                static_cast<int>(std::min(std::max(leds[static_cast<size_t>(n)].g, 0.0f), 1.0f) * 255.0f), 
                                static_cast<int>(std::min(std::max(leds[static_cast<size_t>(n)].b, 0.0f), 1.0f) * 255.0f));
#endif
                        }
                        x++;
                    }
                }
            }
        };

        std::vector<colors::rgb> leds_top;
        for (size_t c = 0; c < leds_rings_n; c++) {
            uint8_t r = static_cast<uint8_t>((leds_outer[0][c].r * brightness ) / 256);
            uint8_t g = static_cast<uint8_t>((leds_outer[0][c].g * brightness ) / 256);
            uint8_t b = static_cast<uint8_t>((leds_outer[0][c].b * brightness ) / 256);
            leds_top.push_back(colors::rgb(colors::rgb8out(r,g,b)));
        }
        for (size_t c = 0; c < leds_rings_n; c++) {
            uint8_t r = static_cast<uint8_t>((leds_inner[0][c].r * brightness * disabled_inner_leds_top[c]) / 256);
            uint8_t g = static_cast<uint8_t>((leds_inner[0][c].g * brightness * disabled_inner_leds_top[c]) / 256);
            uint8_t b = static_cast<uint8_t>((leds_inner[0][c].b * brightness * disabled_inner_leds_top[c]) / 256);
            leds_top.push_back(colors::rgb(colors::rgb8out(r,g,b)));
        }
        leds_top.push_back(colors::rgb(colors::rgb8out(
                static_cast<uint8_t>((leds_centr[0].r * brightness) / 256),
                static_cast<uint8_t>((leds_centr[0].g * brightness) / 256),
                static_cast<uint8_t>((leds_centr[0].b * brightness) / 256))));
        print_leds(0, 0, leds_top);
        
        std::vector<colors::rgb> leds_btm;
        for (size_t c = 0; c < leds_rings_n; c++) {
            uint8_t r = static_cast<uint8_t>((leds_outer[1][c].r * brightness ) / 256);
            uint8_t g = static_cast<uint8_t>((leds_outer[1][c].g * brightness ) / 256);
            uint8_t b = static_cast<uint8_t>((leds_outer[1][c].b * brightness ) / 256);
            leds_btm.push_back(colors::rgb(colors::rgb8out(r,g,b)));
        }
        for (size_t c = 0; c < leds_rings_n; c++) {
            uint8_t r = static_cast<uint8_t>((leds_inner[0][c].r * brightness * disabled_inner_leds_bottom[c]) / 256);
            uint8_t g = static_cast<uint8_t>((leds_inner[0][c].g * brightness * disabled_inner_leds_bottom[c]) / 256);
            uint8_t b = static_cast<uint8_t>((leds_inner[0][c].b * brightness * disabled_inner_leds_bottom[c]) / 256);
            leds_btm.push_back(colors::rgb(colors::rgb8out(r,g,b)));
        }
        leds_btm.push_back(colors::rgb(colors::rgb8out(
                static_cast<uint8_t>((leds_centr[1].r * brightness) / 256),
                static_cast<uint8_t>((leds_centr[1].g * brightness) / 256),
                static_cast<uint8_t>((leds_centr[1].b * brightness) / 256))));
        print_leds(30, 0, leds_btm);
#endif  // #ifdef EMULATOR
    }

    void set_bird_color(const colors::rgb &col) {

        const colors::rgb8out col8(col);

        leds_centr[0] = col8;
        leds_centr[1] = col8;

        for (size_t c = 0; c < leds_rings_n; c++) {
            leds_inner[0][c] = col8;
            leds_inner[1][c] = col8;
        }

    }

    void calc_outer(const std::function<geom::float4 (const geom::float4 &pos)> &func) {
        for (size_t c = 0; c < 16; c++) {
            leds_outer[0][c] = colors::rgb8out(colors::rgb(func(ledpos()[c])));
            leds_outer[1][c] = colors::rgb8out(colors::rgb(func(ledpos()[c])));
        }
    }

    void calc_outer(const std::function<geom::float4 (const geom::float4 &pos, const size_t index)> &func) {
        for (size_t c = 0; c < 16; c++) {
            leds_outer[0][c] = colors::rgb8out(colors::rgb(func(ledpos()[c],c)));
            leds_outer[1][c] = colors::rgb8out(colors::rgb(func(ledpos()[c],c)));
        }
    }

    void calc_all(const std::function<geom::float4 (const geom::float4 &pos)> &func) {
        for (size_t c = 0; c < 16; c++) {
            leds_outer[0][c] = colors::rgb8out(colors::rgb(func(ledpos()[c])));
            leds_outer[1][c] = colors::rgb8out(colors::rgb(func(ledpos()[c])));
        }
        for (size_t c = 0; c < 16; c++) {
            leds_inner[0][c] = colors::rgb8out(colors::rgb(func(ledpos()[c+16])));
            leds_inner[1][c] = colors::rgb8out(colors::rgb(func(ledpos()[c+16])));
        }
        leds_centr[0] = colors::rgb8out(colors::rgb(func(ledpos()[32])));
        leds_centr[1] = colors::rgb8out(colors::rgb(func(ledpos()[32])));
    }

    void calc_inner(const std::function<geom::float4 (const geom::float4 &pos)> &func) {
        for (size_t c = 0; c < 16; c++) {
            leds_inner[0][c] = colors::rgb8out(colors::rgb(func(ledpos()[c+16])));
            leds_inner[1][c] = colors::rgb8out(colors::rgb(func(ledpos()[c+16])));
        }
        leds_centr[0] = colors::rgb8out(colors::rgb(func(ledpos()[32])));
        leds_centr[1] = colors::rgb8out(colors::rgb(func(ledpos()[32])));
    }
    
    //
    // STATIC COLOR
    //

    void static_color() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        calc_outer([=](geom::float4) {
        	return geom::float4(colors::rgb(Model::instance().RingColor()));
        });
    }

    //
    // RGB BAND
    //

    float rgb_band_r_walk = 0.0f;
    float rgb_band_g_walk = 0.0f;
    float rgb_band_b_walk = 0.0f;

    float rgb_band_r_walk_step = 2.0f;
    float rgb_band_g_walk_step = 2.0f;
    float rgb_band_b_walk_step = 2.0f;

    template<const std::size_t n> void band_mapper(std::array<float, n> &stops, float stt, float end) {

        for (; stt < 0 ;) {
            stt += 1.0f;
            end += 1.0f;
        }
    
        for (; stt > 1.0f ;) {
            stt -= 1.0f;
            end -= 1.0f;
        }

        stt = fmodf(stt, 2.0f) + 0.5f;
        end = fmodf(end, 2.0f) + 0.5f;
    
        float stop_step = static_cast<float>(stops.size());
        float stop_stepi = 1.0f / static_cast<float>(stops.size());
    
        float stop_stt = -stop_stepi * 0.5f + 0.5f;
        float stop_end = +stop_stepi * 0.5f + 0.5f;

        for (size_t c = 0; c < stops.size(); c++) {
            stops[c] = 0.0f;
        }

        for (size_t c = 0; c < stops.size() * 2; c++) {
            if (stt <= stop_stt && end >= stop_end) {
                if ( ( stop_stt - stt ) < stop_stepi) {
                    stops[c % stops.size()] = ( stop_stt - stt ) * stop_step;
                } else if ( ( end - stop_end ) < stop_stepi) {
                    stops[c % stops.size()] = ( end - stop_end ) * stop_step;
                } else {
                    stops[c % stops.size()] = 1.0f;
                }
            }
            stop_end += stop_stepi;
            stop_stt += stop_stepi;
        }
    }

    void black() {
        memset(leds_centr, 0, sizeof(leds_centr));
        memset(leds_outer, 0, sizeof(leds_outer));
        memset(leds_inner, 0, sizeof(leds_inner));
    }

    void rgb_band() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        static std::mt19937 gen;
        static std::uniform_real_distribution<float> disf(+0.001f, +0.005f);
        static std::uniform_int_distribution<int32_t> disi(0, 1);

        std::array<float, leds_rings_n> band_r;
        std::array<float, leds_rings_n> band_g;
        std::array<float, leds_rings_n> band_b;

        if (fabsf(rgb_band_r_walk) >= 2.0f) {
            while (rgb_band_r_walk >= +1.0f) { rgb_band_r_walk -= 1.0f; }
            while (rgb_band_r_walk <= -1.0f) { rgb_band_r_walk += 1.0f; }
            rgb_band_r_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
        }
    
        if (fabsf(rgb_band_g_walk) >= 2.0f) {
            while (rgb_band_g_walk >= +1.0f) { rgb_band_g_walk -= 1.0f; }
            while (rgb_band_g_walk <= -1.0f) { rgb_band_g_walk += 1.0f; }
            rgb_band_g_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
        }
    
        if (fabsf(rgb_band_b_walk) >= 2.0f) {
            while (rgb_band_b_walk >= +1.0f) { rgb_band_b_walk -= 1.0f; }
            while (rgb_band_b_walk <= -1.0f) { rgb_band_b_walk += 1.0f; }
            rgb_band_b_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
        }

        band_mapper(band_r, rgb_band_r_walk, rgb_band_r_walk + (1.0f / 3.0f));
        band_mapper(band_g, rgb_band_g_walk, rgb_band_g_walk + (1.0f / 3.0f));
        band_mapper(band_b, rgb_band_b_walk, rgb_band_b_walk + (1.0f / 3.0f));

        for (size_t c = 0; c < leds_rings_n; c++) {
            colors::rgb8out out = colors::rgb8out(colors::rgb(band_r[c], band_g[c], band_b[c]));        
            leds_outer[0][c] = out;
            leds_outer[1][leds_rings_n-1-c] = out;
        }
    
        rgb_band_r_walk -= rgb_band_r_walk_step;
        rgb_band_g_walk += rgb_band_g_walk_step;
        rgb_band_b_walk += rgb_band_b_walk_step;
    }

    //
    // COLOR WALKER
    //

    void color_walker() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        double now = Model::instance().Time();

        const double speed = 2.0;

        float rgb_walk = (       static_cast<float>(fmodf(static_cast<float>(now * (1.0 / 5.0) * speed), 1.0)));
        float val_walk = (1.0f - static_cast<float>(fmodf(static_cast<float>(now               * speed), 1.0)));

        colors::hsp col(rgb_walk, 1.0f, 1.0f);
        for (size_t c = 0; c < leds_rings_n; c++) {
            float mod_walk = val_walk + (1.0f - (c * ( 1.0f / static_cast<float>(leds_rings_n) ) ) );
            if (mod_walk > 1.0f) {
                mod_walk -= 1.0f;
            }
            col.p = std::min(1.0f, mod_walk);

            colors::rgb8out out = colors::rgb8out(colors::rgb(col));        

            leds_outer[0][c] = out;
            leds_outer[1][leds_rings_n-1-c] = out;
        }

    }

    //
    // LIGHT WALKER
    //

    void light_walker() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        double now = Model::instance().Time();

        const double speed = 2.0;

        float rgb_walk = (       static_cast<float>(fmodf(static_cast<float>(now * (1.0 / 5.0) * speed), 1.0)));
        float val_walk = (1.0f - static_cast<float>(fmodf(static_cast<float>(now               * speed), 1.0)));
    
        colors::hsv col(rgb_walk, 1.0f, 1.0f);
        for (size_t c = 0; c < leds_rings_n; c++) {
            float mod_walk = val_walk + ( 1.0f - ( c * ( 1.0f / static_cast<float>(leds_rings_n) ) ) );
            if (mod_walk > 1.0f) {
                mod_walk -= 1.0f;
            }
        
            col.s = 1.0f - std::min(1.0f, mod_walk);
            col.v = std::min(1.0f, mod_walk);

            colors::rgb8out out = colors::rgb8out(colors::rgb(col));        

            leds_outer[0][c] = out;
            leds_outer[1][leds_rings_n-1-c] = out;
        }
    }

    //
    // RGB GLOW
    //

    void rgb_glow() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        double now = Model::instance().Time();

        const double speed = 0.5;

        float rgb_walk = (static_cast<float>(fmodf(static_cast<float>(now * (1.0 / 5.0) * speed), 1.0)));
    
        colors::hsv col(rgb_walk, 1.0f, 1.0f);
        colors::rgb8out out = colors::rgb8out(colors::rgb(col));        
        for (size_t c = 0; c < leds_rings_n; c++) {
            leds_outer[0][c] = out;
            leds_outer[1][c] = out;
        }
    }

    //
    // LIGHTNING
    //

    void lightning() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        colors::rgb8out black = colors::rgb8out(colors::rgb(0.0f,0.0f,0.0f));       
        for (size_t c = 0; c < leds_rings_n; c++) {
            leds_outer[0][c] = black;
            leds_outer[1][c] = black;
        }

        size_t index = static_cast<size_t>(random.get(static_cast<int32_t>(0),static_cast<int32_t>(16*2 * 16)));
        colors::rgb8out white = colors::rgb8out(colors::rgb(1.0f,1.0f,1.0f));       
        if (index < 16) {
            leds_outer[0][index] = white;
        } else if (index < 32) {
            leds_outer[1][index-16] = white;
        }
    }

    //
    // LIGHTNING CRAZY
    //

    void lightning_crazy() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        colors::rgb8out black = colors::rgb8out(colors::rgb(0.0f,0.0f,0.0f));       
        for (size_t c = 0; c < leds_rings_n; c++) {
            leds_outer[0][c] = black;
            leds_outer[1][c] = black;
        }

        size_t index = static_cast<size_t>(random.get(static_cast<int32_t>(0),static_cast<int32_t>(leds_rings_n*2)));
        colors::rgb8out white = colors::rgb8out(colors::rgb(1.0f,1.0f,1.0f));       
        if (index < leds_rings_n) {
            leds_outer[0][index] = white;
        } else if (index < leds_rings_n*2) {
            leds_outer[1][index-leds_rings_n] = white;
        }
    }

    //
    // SPARKLE
    //

    void sparkle() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        colors::rgb8out black = colors::rgb8out(colors::rgb(0.0f,0.0f,0.0f));       
        for (size_t c = 0; c < leds_rings_n; c++) {
            leds_outer[0][c] = black;
            leds_outer[1][c] = black;
        }

        size_t index = static_cast<size_t>(random.get(static_cast<int32_t>(0),static_cast<int32_t>(leds_rings_n*2)));
        colors::rgb8out col = colors::rgb8out(colors::rgb(
            random.get(0.0f,1.0f),
            random.get(0.0f,1.0f),
            random.get(0.0f,1.0f)));        
        if (index < leds_rings_n) {
            leds_outer[0][index] = col;
        } else if (index < leds_rings_n*2) {
            leds_outer[1][index-leds_rings_n] = col;
        }
    }

    //
    // RANDOM
    //

    void rando() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        for (size_t c = 0; c < leds_rings_n; c++) {
            colors::rgb8out col = colors::rgb8out(colors::rgb(
                random.get(0.0f,1.0f),
                random.get(0.0f,1.0f),
                random.get(0.0f,1.0f)));        
            leds_outer[0][c] = col;
            leds_outer[1][c] = col;
        }
    }
    
    //
    // RED GREEN
    //

    void red_green() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        calc_outer([=](geom::float4 pos) {
            pos.x *= sinf(now);
            pos.y *= cosf(now);
            return pos;
        });
    }

    //
    // BRILLIANCE
    //

    void brilliance() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static float next = -1.0f;
        static float dir = 0.0f;
        
        if ((next - now) < 0.0f || next < 0.0f) {
            next = now + random.get(2.0f, 20.0f);
            dir = random.get(0.0f, 3.141f * 2.0f);
        }

        static colors::gradient bw;
        static colors::rgb8 col;
        if (bw.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 bwg[] = {
                geom::float4(Model::instance().RingColor().hex(), 0.00f),
                geom::float4(Model::instance().RingColor().hex(), 0.14f),
                geom::float4(0xffffff, 0.21f),
                geom::float4(Model::instance().RingColor().hex(), 0.28f),
                geom::float4(Model::instance().RingColor().hex(), 1.00f)};
            bw.init(bwg,5);
        }

        calc_outer([=](geom::float4 pos) {
            pos = pos.rotate2d(dir);
            pos *= 0.50f;
            pos += (next - now) * 8.0f;
            pos *= 0.05f;
            return bw.clamp(pos.x);
        });
    }

    //
    // HIGHLIGHT
    //

    void highlight() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static float next = -1.0f;
        static float dir = 0.0f;
        
        if ((next - now) < 0.0f || next < 0.0f) {
            next = now + random.get(2.0f, 10.0f);
            dir = random.get(0.0f, 3.141f * 2.0f);
        }

        static colors::gradient bw;
        static colors::rgb8 col;
        if (bw.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 bwg[] = {
                geom::float4(Model::instance().RingColor().hex(), 0.00f),
                geom::float4(Model::instance().RingColor().hex(), 0.40f),
                geom::float4(0xffffff, 0.50f),
                geom::float4(Model::instance().RingColor().hex(), 0.60f),
                geom::float4(Model::instance().RingColor().hex(), 1.00f)};
            bw.init(bwg,5);
        }

        calc_outer([=](geom::float4 pos) {
            pos = pos.rotate2d(dir);
            pos *= 0.50f;
            pos += (next - now);
            pos *= 0.50f;
            return bw.clamp(pos.x);
        });
    }

    //
    // AUTUMN
    //

    void autumn() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static colors::gradient g;
        if (g.check_init()) {
            const geom::float4 gg[] = {
               geom::float4(0x968b3f, 0.00f),
               geom::float4(0x097916, 0.20f),
               geom::float4(0x00d4ff, 0.40f),
               geom::float4(0xffffff, 0.50f),
               geom::float4(0x8a0e45, 0.80f),
               geom::float4(0x968b3f, 1.00f)};
            g.init(gg,6);
        }

        calc_outer([=](geom::float4 pos) {
            pos += 0.5f;
            pos = pos.rotate2d(now);
            pos *= 0.5f;
            pos += 1.0f;
            return g.repeat(pos.x);
        });
    }

    //
    // HEARTBEAT
    //

    void heartbeat() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());
        
        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0x000000, 0.00f),
               geom::float4(Model::instance().RingColor().hex(), 1.00)};
            g.init(gg,2);
        }

        calc_outer([=](geom::float4) {
            return g.reflect(now);
        });
    }

    //
    // MOVING RAINBOW
    //

    void moving_rainbow() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        calc_outer([=](geom::float4 pos) {
            pos = pos.rotate2d(-now * 0.25f);
            pos += now;
            pos *= 0.25f;
            pos = pos.reflect();
            return geom::float4(colors::rgb(colors::hsv(pos.x, 1.0, 1.0f)));
        });
    }

    //
    // TWINKLE
    //

    void twinkle() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static constexpr size_t many = 8;
        static float next[many] = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
        static size_t which[many] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        
        for (size_t c = 0; c < many; c++) {
            if ((next[c] - now) < 0.0f || next[c] < 0.0f) {
                next[c] = now + random.get(0.5f, 4.0f);
                which[c] = static_cast<size_t>(random.get(static_cast<int32_t>(0), leds_rings_n));
            }
        }

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0x000000, 0.00f),
               geom::float4(Model::instance().RingColor().hex(), 0.25f),
               geom::float4(0xFFFFFF, 0.50f),
               geom::float4(Model::instance().RingColor().hex(), 0.75f),
               geom::float4(0x000000, 1.00f)};
            g.init(gg,5);
        }
        
        calc_outer([=](geom::float4, size_t index) {
            for (size_t c = 0; c < many; c++) {
                if (which[c] == index) {
                    return g.clamp(next[c] - now);
                }
            }
            return geom::float4();
        });
        
    }

    //
    // TWINKLY
    //

    void twinkly() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static constexpr size_t many = 8;
        static float next[many] = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
        static size_t which[many] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        
        for (size_t c = 0; c < many; c++) {
            if ((next[c] - now) < 0.0f || next[c] < 0.0f) {
                next[c] = now + random.get(0.5f, 4.0f);
                which[c] = static_cast<size_t>(random.get(static_cast<int32_t>(0), leds_rings_n));
            }
        }

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0x000000, 0.00f),
               geom::float4(0xFFFFFF, 0.50f),
               geom::float4(0x000000, 1.00f)};
            g.init(gg,3);
        }
        
        colors::rgb ring(Model::instance().RingColor());
        
        calc_outer([=](geom::float4, size_t index) {
            for (size_t c = 0; c < many; c++) {
                if (which[c] == index) {
                    return g.clamp(next[c] - now) + geom::float4(ring);
                }
            }
            return geom::float4(ring);
        });
    }

    //
    // RANDOMFADER
    //

    void randomfader() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static float next = -1.0f;
        static size_t which = 0;
        static colors::rgb color;
        static colors::rgb prev_color;
        
        if ((next - now) < 0.0f || next < 0.0f) {
            next = now + 2.0f;
            which = static_cast<size_t>(random.get(static_cast<int32_t>(0), leds_rings_n));
            prev_color = color;
            color = colors::rgb(
                random.get(0.0f,1.0f),
                random.get(0.0f,1.0f),
                random.get(0.0f,1.0f));
        }

        calc_outer([=](geom::float4 pos) {
            float dist = pos.dist(ledpos()[which]) * (next - now);
            if (dist > 1.0f) dist = 1.0f;
            return geom::float4::lerp(color, prev_color, dist);
        });
    }

    //
    // CHASER
    //

    void chaser() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());
        
        colors::rgb ring(Model::instance().RingColor());

        calc_outer([=](geom::float4 pos) {
            pos = pos.rotate2d(now);
            return ring * pos.x;
        });
    }

    //
    // BRIGHT CHASER
    //

    void brightchaser() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0x000000, 0.00f),
               geom::float4(Model::instance().RingColor().hex(), 0.50f),
               geom::float4(0xFFFFFF, 1.00f)};
            g.init(gg,3);
        }

        calc_outer([=](geom::float4 pos) {
            pos = pos.rotate2d(now);
            return g.clamp(pos.x);
        });
    }

    //
    // GRADIENT
    //

    void gradient() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        colors::rgb ring(Model::instance().RingColor());

        calc_outer([=](geom::float4 pos) {
            return ring * (1.0f - ( (pos.y + 1.0f) * 0.33f ) );
        });
    }

    void overdrive() {
        float now = static_cast<float>(Model::instance().Time());

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0x000000, 0.00f),
               geom::float4(Model::instance().RingColor().hex(), 1.0f)
            };
            g.init(gg,2);
        }

        calc_inner([=](geom::float4 pos) {
        	float x = sinf(pos.x + 1.0f + now * 1.77f);
        	float y = cosf(pos.y + 1.0f + now * 2.01f);
            return (g.reflect(x * y) * 8.0f).clamp();
        });

        calc_outer([=](geom::float4 pos) {
        	float x = sinf(pos.x + 1.0f + now * 1.77f);
        	float y = cosf(pos.y + 1.0f + now * 2.01f);
            return (g.reflect(x * y) * 8.0f).clamp();
        });
    }

    void ironman() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0xffffff, 0.00f),
               geom::float4(Model::instance().RingColor().hex(), 0.5f),
               geom::float4(0x000000, 1.00f),
            };
            g.init(gg,3);
        }

        calc_inner([=](geom::float4 pos) {
        	float len = pos.len();
        	return g.clamp(1.0f-((len!=0.0f)?1.0f/len:1000.0f)*(fabsf(sinf(now))));
        });

        calc_outer([=](geom::float4 pos) {
        	float len = pos.len();
        	return g.clamp(1.0f-((len!=0.0f)?1.0f/len:1000.0f)*(fabsf(sinf(now))));
        });
    }

    void sweep() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0x000000, 0.00f),
               geom::float4(Model::instance().RingColor().hex(), 1.0f),
            };
            g.init(gg,2);
        }

        calc_outer([=](geom::float4 pos) {
        	pos = pos.rotate2d(-now * 0.5f);
            return g.reflect(pos.y - now * 8.0f);
        });
    }

    void sweephighlight() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0x000000, 0.00f),
               geom::float4(Model::instance().RingColor().hex(), 0.7f),
               geom::float4(0xffffff, 1.00f),
            };
            g.init(gg,3);
        }

        calc_outer([=](geom::float4 pos) {
        	pos = pos.rotate2d(-now * 0.25f);
            return g.reflect(pos.y - now * 2.0f);
        });
    }

    void rainbow_circle() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        calc_outer([=](geom::float4 pos) {
            return geom::float4(colors::rgb(colors::hsv(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.5f, 1.0f), 1.0f, 1.0f)));
        }); 
    }

    void rainbow_grow() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        calc_outer([=](geom::float4 pos) {
            return geom::float4(colors::rgb(colors::hsv(fmodf(fabsf(pos.x * 0.25f + signf(pos.x) * now * 0.25f), 1.0f), 1.0f, 1.0f)));
        });
    }

    void rotor() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(Model::instance().RingColor().hex(), 0.00f),
               geom::float4(0xffffff, 0.50f),
               geom::float4(Model::instance().RingColor().hex(), 1.00f)
            };
            g.init(gg,3);
        }

        calc_outer([=](geom::float4 pos) {
        	return g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.5f, 1.0f) * 4.0f);
        }); 
    }

    void rotor_sparse() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0x000000, 0.00f),
               geom::float4(Model::instance().RingColor().hex(), 0.40f),
               geom::float4(Model::instance().RingColor().hex(), 0.60f),
               geom::float4(0x000000, 1.00f)
            };
            g.init(gg,5);
        }

        calc_outer([=](geom::float4 pos) {
        	return g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.5f, 1.0f) * 3.0f);
        }); 
    }

    void fullcolor() {
        led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

        float now = static_cast<float>(Model::instance().Time());

        static colors::gradient g;
        static colors::rgb8 col;
        if (g.check_init() || col != Model::instance().RingColor()) {
            col = Model::instance().RingColor();
            const geom::float4 gg[] = {
               geom::float4(0x000000, 0.00f),
               geom::float4(0xFFFFFF, 0.50f),
               geom::float4(0x000000, 1.00f)
            };
            g.init(gg,3);
        }

        calc_outer([=](geom::float4 pos) {
        	return geom::float4(
	        	g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.50f, 1.0f)).x,
	        	g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.75f, 1.0f)).x,
	        	g.repeat(fmodf((atan2f(pos.x, pos.y) + 3.14159f) / (3.14159f * 2.0f) + now * 0.33f, 1.0f)).x
	        );
        }); 
    }

    void flip_colors() {
        float now = static_cast<float>(Model::instance().Time());
        
        geom::float4 bird(colors::rgb(Model::instance().BirdColor()));
        geom::float4 ring(colors::rgb(Model::instance().RingColor()));

        calc_inner([=](geom::float4) {
        	return geom::float4::lerp(bird, ring, (sinf(now) + 1.0f) * 0.5f);
        });

        calc_outer([=](geom::float4) {
        	return geom::float4::lerp(ring, bird, (sinf(now) + 1.0f) * 0.5f);
        });
    }

    //
    // BURN TEST
    //

    float burn_test_flip = 1.0f;

    void burn_test() {

        static std::mt19937 gen;
        static std::uniform_real_distribution<float> disf(+1.0f, -1.0f);
        
        for (size_t c = 0; c < leds_rings_n; c++) {

            burn_test_flip *= -1.0f;

            colors::rgb8out out = colors::rgb8out(colors::rgb(1.0f, 1.0f, 1.0f) * burn_test_flip );

            leds_outer[0][c] = out;
            leds_outer[1][c] = out;

            leds_inner[0][c] = out;
            leds_inner[1][c] = out;
        }
    }

    //
    // BIRD COLOR MODIFIER
    //

    void bird_color(colors::rgb8 color, Timeline::Span &span, Timeline::Span &below) {
        float blend = 0.0f;

        // Continue to run effect below
        below.Calc();

        if (span.InBeginPeriod(blend, 0.25f)) {
        } else if (span.InEndPeriod(blend, 0.25f)) {
            blend = 1.0f - blend;
        }
        
        colors::rgb8out out = colors::rgb8out(colors::rgb(color));
        
        for (size_t c = 0; c < leds_rings_n; c++) {
            if (blend != 0.0f) {
                leds_inner[0][c] = colors::ip(leds_inner[0][c], out, blend);
                leds_inner[1][c] = colors::ip(leds_inner[1][c], out, blend);
            } else {
                leds_inner[0][c] = out;
                leds_inner[1][c] = out;
            }
        }
        if (blend != 0.0f) {
            leds_centr[0] = colors::ip(leds_centr[0], out, blend);
            leds_centr[1] = colors::ip(leds_centr[1], out, blend);
        } else {
            leds_centr[0] = out;
            leds_centr[1] = out;
        }
    }

    //
    // RING COLOR MODIFIER
    //

    void ring_color(colors::rgb8 color, Timeline::Span &span, Timeline::Span &below) {
        float blend = 0.0f;

        // Continue to run effect below
        below.Calc();

        if (span.InBeginPeriod(blend, 0.25f)) {
        } else if (span.InEndPeriod(blend, 0.25f)) {
            blend = 1.0f - blend;
        }
        
        colors::rgb8out out = colors::rgb8out(colors::rgb(color));
        
        for (size_t c = 0; c < leds_rings_n; c++) {
            if (blend != 0.0f) {
                leds_outer[0][c] = colors::ip(leds_outer[0][c], out, blend);
                leds_outer[1][c] = colors::ip(leds_outer[1][c], out, blend);
            } else {
                leds_outer[0][c] = out;
                leds_outer[1][c] = out;
            }
        }
    }
    
    //
    // MESSAGE COLOR MODIFIER
    // 

    void message_color(colors::rgb8 color, Timeline::Span &span, Timeline::Span &below) {

        float blend = 0.0f;
        if (span.InBeginPeriod(blend, 0.25f)) {
            below.Calc();
        } else if (span.InEndPeriod(blend, 0.25f)) {
            below.Calc();
            blend = 1.0f - blend;
        }

        colors::rgb8out out = colors::rgb8out(colors::rgb(color));

        for (size_t c = 0; c < leds_rings_n; c++) {

            if (blend != 0.0f) {
                leds_outer[0][c] = colors::ip(leds_outer[0][c], out, blend);
                leds_outer[1][c] = colors::ip(leds_outer[1][c], out, blend);

                leds_inner[0][c] = colors::ip(leds_inner[0][c], out, blend);
                leds_inner[1][c] = colors::ip(leds_inner[1][c], out, blend);

            } else {
                leds_outer[0][c] = out;
                leds_outer[1][c] = out;

                leds_inner[0][c] = out;
                leds_inner[1][c] = out;
            }
        }

        if (blend != 0.0f) {
            leds_centr[0] = colors::ip(leds_centr[0], out, blend);
            leds_centr[1] = colors::ip(leds_centr[1], out, blend);
        } else {
            leds_centr[0] = out;
            leds_centr[1] = out;
        }
    }

    //
    // V2 MESSAGE
    //
    
    void message_v2(uint32_t color, Timeline::Span &span, Timeline::Span &below) {

        float blend = 0.0f;
        if (span.InBeginPeriod(blend, 0.25f)) {
            below.Calc();
        } else if (span.InEndPeriod(blend, 0.25f)) {
            below.Calc();
            blend = 1.0f - blend;
        }

        const double speed = 3.0;
        double now = Model::instance().Time();
        int32_t direction = static_cast<int32_t>((now - span.time)  * speed) & 1;
        float color_walk = fmodf(static_cast<float>((now - span.time) * speed), 1.0f);

        colors::rgb8out out = colors::rgb8out(colors::rgb(color) * (direction ? (1.0f - color_walk) : color_walk) * 1.6f );

        for (size_t c = 0; c < leds_rings_n; c++) {

            if (blend != 0.0f) {
                leds_outer[0][c] = colors::ip(leds_outer[0][c], out, blend);
                leds_outer[1][c] = colors::ip(leds_outer[1][c], out, blend);

                leds_inner[0][c] = colors::ip(leds_inner[0][c], out, blend);
                leds_inner[1][c] = colors::ip(leds_inner[1][c], out, blend);

            } else {
                leds_outer[0][c] = out;
                leds_outer[1][c] = out;

                leds_inner[0][c] = out;
                leds_inner[1][c] = out;
            }
        }

        if (blend != 0.0f) {
            leds_centr[0] = colors::ip(leds_centr[0], out, blend);
            leds_centr[1] = colors::ip(leds_centr[1], out, blend);
        } else {
            leds_centr[0] = out;
            leds_centr[1] = out;
        }
    
    }
};

void led_control::init () {
    led_bank::instance();
}

void led_control::PerformV2MessageEffect(uint32_t color, bool remove) {
    static Timeline::Span s;

    if (Timeline::instance().Scheduled(s)) {
        return;
    }

    if (remove) {
        s.time = Model::instance().Time();
        s.duration = 0.25;
        return;
    }

    s.type = Timeline::Span::Effect;
    s.time = Model::instance().Time();
    s.duration = 8.0;
    s.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
        led_bank::instance().message_v2(color, span, below);
    };
    s.commitFunc = [=](Timeline::Span &) {
        led_bank::instance().update_leds();
    };

    Timeline::instance().Add(s);
}

void led_control::PerformColorBirdDisplay(colors::rgb8 color, bool remove) {
    static Timeline::Span s;

    static colors::rgb8 passThroughColor;

    passThroughColor = color;

    if (remove) {
        s.time = Model::instance().Time();
        s.duration = 0.25;
        return;
    }

    if (Timeline::instance().Scheduled(s)) {
        return;
    }

    s.type = Timeline::Span::Effect;
    s.time = Model::instance().Time();
    s.duration = std::numeric_limits<double>::infinity();
    s.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
        led_bank::instance().bird_color(passThroughColor, span, below);
    };
    s.commitFunc = [=](Timeline::Span &) {
        led_bank::instance().update_leds();
    };

    Timeline::instance().Add(s);
}

void led_control::PerformColorRingDisplay(colors::rgb8 color, bool remove) {
    static Timeline::Span s;

    static colors::rgb8 passThroughColor;

    passThroughColor = color;

    if (remove) {
        s.time = Model::instance().Time();
        s.duration = 0.25;
        return;
    }

    if (Timeline::instance().Scheduled(s)) {
        return;
    }

    s.type = Timeline::Span::Effect;
    s.time = Model::instance().Time();
    s.duration = std::numeric_limits<double>::infinity();
    s.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
        led_bank::instance().ring_color(passThroughColor, span, below);
    };
    s.commitFunc = [=](Timeline::Span &) {
        led_bank::instance().update_leds();
    };

    Timeline::instance().Add(s);
}

void led_control::PerformMessageColorDisplay(colors::rgb8 color, bool remove) {
    static Timeline::Span s;

    static colors::rgb8 passThroughColor;
    
    passThroughColor = color;

    if (remove) {
        s.time = Model::instance().Time();
        s.duration = 0.25;
        return;
    }

    if (Timeline::instance().Scheduled(s)) {
        return;
    }

    s.type = Timeline::Span::Effect;
    s.time = Model::instance().Time();
    s.duration = std::numeric_limits<double>::infinity();
    s.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
        led_bank::instance().message_color(passThroughColor, span, below);
    };
    s.commitFunc = [=](Timeline::Span &) {
        led_bank::instance().update_leds();
    };

    Timeline::instance().Add(s);
}
