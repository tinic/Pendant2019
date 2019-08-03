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
		return (double(get()) * (double(upper-lower)/double(1LL<<32)) ) + lower;
	}

	int32_t get(int32_t lower, int32_t upper) {
		return (get() % (upper-lower)) + lower;
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
		r = float(from.r) * (1.0f/255.0f);
		g = float(from.g) * (1.0f/255.0f);
		b = float(from.b) * (1.0f/255.0f);
	}

	rgb::rgb(const rgb8out &from) {
		r = float(from.r) * (1.0f/255.0f);
		g = float(from.g) * (1.0f/255.0f);
		b = float(from.b) * (1.0f/255.0f);
	}

	static float value(  float const &p, float const &q, float const &t ) {
		if( t < 1.0f/6.0f ) return ( p + ( q - p ) * 6.0f * t );
		if( t < 1.0f/2.0f ) return ( q );
		if( t < 2.0f/3.0f ) return ( p + ( q - p ) * ( ( 2.0f / 3.0f ) - t ) * 6.0f );
		return p ;
	}

	rgb::rgb(const uint32_t color) {
		r = float((color>>16)&0xFF) * (1.0f/255.0f);
		g = float((color>> 8)&0xFF) * (1.0f/255.0f);
		b = float((color>> 0)&0xFF) * (1.0f/255.0f);
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

		int32_t rd = int32_t( 6.0f * h );
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
			int32_t ii = int32_t(i * 256.0f);
			return rgb8out(
				( ( a.r * ( 256 - ii ) ) + ( b.r * ii ) ) / 256,
				( ( a.g * ( 256 - ii ) ) + ( b.g * ii ) ) / 256,
				( ( a.b * ( 256 - ii ) ) + ( b.b * ii ) ) / 256
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

		float4(float x, float y, float z, float w = 0.0f) {
			this->x = x;
			this->y = y;
			this->z = z;
			this->w = w;
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
			float xd = fabs(this->x - b.x);
			float yd = fabs(this->y - b.y);
			float zd = fabs(this->z - b.z);
			return sqrtf(xd*xd + yd*yd + zd*zd);
		}

		static float dist(const float4 &a, const float4 &b) {
			float xd = fabs(a.x - b.x);
			float yd = fabs(a.y - b.y);
			float zd = fabs(a.z - b.z);
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
			return float4(fabs(this->x),
					  	  fabs(this->y),
						  fabs(this->z),
						  fabs(this->w));
		}

		static float4 abs(const float4 &a) {
			return float4(fabs(a.x),
					  	  fabs(a.y),
						  fabs(a.z),
						  fabs(a.w));
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
			return  float4(
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
			i = fabs(i);
			if (((int)i & 1) == 0) {
				i = fmod(i, 1.0f);
			} else {
				i = fmod(i, 1.0f);
				i = 1.0 - i;
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
				float f = (float)c / ((float)colors_n - 1); 
				geom::float4 a = stops[0];
				geom::float4 b = stops[1];
				if (n > 2) {
				  for (int32_t d = (n-2); d >= 0 ; d--) {
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
			return geom::float4::lerp(colors[((int32_t)(i))&colors_mask], colors[((int32_t)(i)+1)&colors_mask], fmodf(i, 1.0f));
		}

		geom::float4 reflect(float i) {
			i = fabs(i);
			if (((int)i & 1) == 0) {
				i = fmodf(i, 1.0f);
			} else {
				i = fmodf(i, 1.0f);
				i = 1.0f - i;
			}
			i *= colors_mul;
			return geom::float4::lerp(colors[((int32_t)(i))&colors_mask], colors[((int32_t)(i)+1)&colors_mask], fmodf(i, 1.0f));
		}

		geom::float4 clamp(float i) {
			if (i <= 0.0f) {
				return colors[0];
			}
			if (i >= 1.0f) {
				return colors[colors_n-1];
			}
			i *= colors_mul;
			return geom::float4::lerp(colors[((int32_t)(i))&colors_mask], colors[((int32_t)(i)+1)&colors_mask], fmodf(i, 1.0f));
		}
	};
};

static void _qspi_memcpy(uint8_t *dst, uint8_t *src, uint32_t count)
{
	if (count < 1) {
		return;
	}
#ifndef EMULATOR
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
#endif  // #ifndef EMULATOR
}

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
			leds[ 0] = geom::float4(-0.00000000,-1.00000000,+0.00000000,+0.00000000);
			leds[ 1] = geom::float4(-0.38268343,-0.92387953,+0.00000000,+0.00000000);
			leds[ 2] = geom::float4(-0.70710678,-0.70710678,+0.00000000,+0.00000000);
			leds[ 3] = geom::float4(-0.92387953,-0.38268343,+0.00000000,+0.00000000);
			leds[ 4] = geom::float4(-1.00000000,-0.00000000,+0.00000000,+0.00000000);
			leds[ 5] = geom::float4(-0.92387953,+0.38268343,+0.00000000,+0.00000000);
			leds[ 6] = geom::float4(-0.70710678,+0.70710678,+0.00000000,+0.00000000);
			leds[ 7] = geom::float4(-0.38268343,+0.92387953,+0.00000000,+0.00000000);
			leds[ 8] = geom::float4(-0.00000000,+1.00000000,+0.00000000,+0.00000000);
			leds[ 9] = geom::float4(+0.38268343,+0.92387953,+0.00000000,+0.00000000);
			leds[10] = geom::float4(+0.70710678,+0.70710678,+0.00000000,+0.00000000);
			leds[11] = geom::float4(+0.92387953,+0.38268343,+0.00000000,+0.00000000);
			leds[12] = geom::float4(+1.00000000,+0.00000000,+0.00000000,+0.00000000);
			leds[13] = geom::float4(+0.92387953,-0.38268343,+0.00000000,+0.00000000);
			leds[14] = geom::float4(+0.70710678,-0.70710678,+0.00000000,+0.00000000);
			leds[15] = geom::float4(+0.38268343,-0.92387953,+0.00000000,+0.00000000);

			leds[16] = geom::float4(-0.00000000,-0.50000000,+0.00000000,+0.00000000);
			leds[17] = geom::float4(-0.19134172,-0.46193977,+0.00000000,+0.00000000);
			leds[18] = geom::float4(-0.35355339,-0.35355339,+0.00000000,+0.00000000);
			leds[19] = geom::float4(-0.46193977,-0.19134172,+0.00000000,+0.00000000);
			leds[20] = geom::float4(-0.50000000,-0.00000000,+0.00000000,+0.00000000);
			leds[21] = geom::float4(-0.46193977,+0.19134172,+0.00000000,+0.00000000);
			leds[22] = geom::float4(-0.35355339,+0.35355339,+0.00000000,+0.00000000);
			leds[23] = geom::float4(-0.19134172,+0.46193977,+0.00000000,+0.00000000);
			leds[24] = geom::float4(-0.00000000,+0.50000000,+0.00000000,+0.00000000);
			leds[25] = geom::float4(+0.19134172,+0.46193977,+0.00000000,+0.00000000);
			leds[26] = geom::float4(+0.35355339,+0.35355339,+0.00000000,+0.00000000);
			leds[27] = geom::float4(+0.46193977,+0.19134172,+0.00000000,+0.00000000);
			leds[28] = geom::float4(+0.50000000,+0.00000000,+0.00000000,+0.00000000);
			leds[29] = geom::float4(+0.46193977,-0.19134172,+0.00000000,+0.00000000);
			leds[30] = geom::float4(+0.35355339,-0.35355339,+0.00000000,+0.00000000);
			leds[31] = geom::float4(+0.19134172,-0.46193977,+0.00000000,+0.00000000);

			leds[32] = geom::float4(+0.00000000,+0.00000000,+0.00000000,+0.00000000);
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
						rgb_band();
					break;
					case 2:
						color_walker();
					break;
					case 3:
						light_walker();
					break;
					case 4:
						rgb_glow();
					break;
					case 5:
						lightning();
					break;
					case 6:
						lightning_crazy();
					break;
					case 7:
						sparkle();
					break;
					case 8:
						rando();
					break;
					case 9:
						red_green();
					break;
					case 10:
						brilliance();
					break;
					case 11:
						effect_11();
					break;
					case 12:
						effect_12();
					break;
					case 13:
						effect_13();
					break;
					case 14:
						effect_14();
					break;
					case 15:
						effect_15();
					break;
					case 16:
						effect_16();
					break;
					case 17:
						effect_17();
					break;
					case 18:
						effect_18();
					break;
					case 19:
						effect_19();
					break;
					case 20:
						effect_20();
					break;
					case 21:
						effect_21();
					break;
					case 22:
						effect_22();
					break;
					case 23:
						effect_23();
					break;
					case 24:
						effect_24();
					break;
					case 25:
						effect_25();
					break;
					case 26:
						effect_26();
					break;
					case 27:
						effect_27();
					break;
					case 28:
						effect_28();
					break;
					case 29:
						effect_29();
					break;
					case 30:
						effect_30();
					break;
					case 31:
						effect_31();
					break;
					case 32:
						effect_32();
					break;
					case 33:
						burn_test();
					break;
				}
			};

			double blend_duration = 0.5f;
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

				float blend = float(now - switch_time) * (1.0f / float(blend_duration));

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
			{   0b11111111,	0b00000000, 0b00000000, 0b00000000 }, // 0b0000
			{   0b11111111,	0b00010001, 0b00010000, 0b00000000 }, // 0b0001
			{   0b11111111,	0b00100010, 0b00100000, 0b00000000 }, // 0b0010
			{   0b11111111,	0b00110011, 0b00110000, 0b00000000 }, // 0b0011
			{   0b11111111,	0b01000100, 0b01000000, 0b00000000 }, // 0b0100
			{   0b11111111,	0b01010101, 0b01010000, 0b00000000 }, // 0b0101
			{   0b11111111,	0b01101100, 0b01100000, 0b00000000 }, // 0b0110
			{   0b11111111,	0b01110111, 0b01110000, 0b00000000 }, // 0b0111
			{   0b11111111,	0b10001000, 0b10000000, 0b00000000 }, // 0b1000
			{   0b11111111,	0b10011001, 0b10010000, 0b00000000 }, // 0b1001
			{   0b11111111,	0b10101010, 0b10100000, 0b00000000 }, // 0b1010
			{   0b11111111,	0b10111011, 0b10110000, 0b00000000 }, // 0b1011
			{   0b11111111,	0b11001100, 0b11000000, 0b00000000 }, // 0b1100
			{   0b11111111,	0b11011101, 0b11010000, 0b00000000 }, // 0b1101
			{   0b11111111,	0b11101110, 0b11100000, 0b00000000 }, // 0b1110
			{   0b11111111,	0b11111111, 0b11110000, 0b00000000 }, // 0b1111
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

		int32_t brightness = int32_t(Model::instance().Brightness() * 256);

		size_t buf_pos = 0;
		static uint8_t buffer[leds_buffer_size];

		for (size_t c = 0; c < ws2812_commit_time; c++) {
			buffer[buf_pos++] = 0;
			buffer[buf_pos++] = 0;
			buffer[buf_pos++] = 0;
			buffer[buf_pos++] = 0;
		}

		auto transfer4 = [](uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3, uint8_t *buffer, size_t &buf_pos, int32_t brightness) {
			p0 = ( p0 * brightness ) / 256;
			p1 = ( p1 * brightness ) / 256;
			p2 = ( p2 * brightness ) / 256;
			p3 = ( p3 * brightness ) / 256;

			for(int32_t d = 7; d >=0; d--) {
				const uint8_t *src = conv_lookup[
				((p0&(1<<d))?0x8:0x0)|
				((p1&(1<<d))?0x4:0x0)|
				((p2&(1<<d))?0x2:0x0)|
				((p3&(1<<d))?0x1:0x0)];
				buffer[buf_pos++] = *src++;
				buffer[buf_pos++] = *src++;
				buffer[buf_pos++] = *src++;
				buffer[buf_pos++] = *src++;
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

		auto transfer2 = [](uint8_t p0, uint8_t p1, uint8_t *buffer, size_t &buf_pos, int32_t brightness) {
			p0 = ( p0 * brightness ) / 256;
			p1 = ( p1 * brightness ) / 256;
			for(int32_t d = 7; d >=0; d--) {
				const uint8_t *src = conv_lookup[
				((p0&(1<<d))?0x8:0x0)|
				((p0&(1<<d))?0x4:0x0)|
				((p1&(1<<d))?0x2:0x0)|
				((p1&(1<<d))?0x1:0x0)];
				buffer[buf_pos++] = *src++;
				buffer[buf_pos++] = *src++;
				buffer[buf_pos++] = *src++;
				buffer[buf_pos++] = *src++;
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
		volatile uint8_t *qspi_mem = (volatile uint8_t *)QSPI_AHB;

		__disable_irq();
		
		_qspi_memcpy((uint8_t *)qspi_mem, buffer, buf_pos);

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
#ifdef __APPLE__
							printf("\x1b[48;5;%dm  \x1b[0m", 16 + 
								int(std::min(std::max(leds[n].r, 0.0f), 1.0f) *   5.0f)*36+ 
								int(std::min(std::max(leds[n].g, 0.0f), 1.0f) *   5.0f)* 6+ 
								int(std::min(std::max(leds[n].b, 0.0f), 1.0f) *   5.0f)* 1);
#else
							printf("\x1b[48;2;%d;%d;%dm  \x1b[0m", 
								int(std::min(std::max(leds[n].r, 0.0f), 1.0f) * 255.0f), 
								int(std::min(std::max(leds[n].g, 0.0f), 1.0f) * 255.0f), 
								int(std::min(std::max(leds[n].b, 0.0f), 1.0f) * 255.0f));
#endif
						}
						x++;
					}
				}
			}
		};

		std::vector<colors::rgb> leds_top;
		for (size_t c = 0; c < leds_rings_n; c++) {
			uint32_t r = (leds_outer[0][c].r * brightness ) / 256;
			uint32_t g = (leds_outer[0][c].g * brightness ) / 256;
			uint32_t b = (leds_outer[0][c].b * brightness ) / 256;
			leds_top.push_back(colors::rgb(colors::rgb8out(r,g,b)));
		}
		for (size_t c = 0; c < leds_rings_n; c++) {
			uint32_t r = (leds_inner[0][c].r * brightness * disabled_inner_leds_top[c]) / 256;
			uint32_t g = (leds_inner[0][c].g * brightness * disabled_inner_leds_top[c]) / 256;
			uint32_t b = (leds_inner[0][c].b * brightness * disabled_inner_leds_top[c]) / 256;
			leds_top.push_back(colors::rgb(colors::rgb8out(r,g,b)));
		}
		leds_top.push_back(colors::rgb(colors::rgb8out(
				(leds_centr[0].r * brightness) / 256,
				(leds_centr[0].g * brightness) / 256,
				(leds_centr[0].b * brightness) / 256)));
		print_leds(0, 0, leds_top);
		
		std::vector<colors::rgb> leds_btm;
		for (size_t c = 0; c < leds_rings_n; c++) {
			uint32_t r = (leds_outer[1][c].r * brightness ) / 256;
			uint32_t g = (leds_outer[1][c].g * brightness ) / 256;
			uint32_t b = (leds_outer[1][c].b * brightness ) / 256;
			leds_btm.push_back(colors::rgb(colors::rgb8out(r,g,b)));
		}
		for (size_t c = 0; c < leds_rings_n; c++) {
			uint32_t r = (leds_inner[0][c].r * brightness * disabled_inner_leds_bottom[c]) / 256;
			uint32_t g = (leds_inner[0][c].g * brightness * disabled_inner_leds_bottom[c]) / 256;
			uint32_t b = (leds_inner[0][c].b * brightness * disabled_inner_leds_bottom[c]) / 256;
			leds_btm.push_back(colors::rgb(colors::rgb8out(r,g,b)));
		}
		leds_btm.push_back(colors::rgb(colors::rgb8out(
				(leds_centr[1].r * brightness) / 256,
				(leds_centr[1].g * brightness) / 256,
				(leds_centr[1].b * brightness) / 256)));
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
	
		float stop_step = float(stops.size());
		float stop_stepi = 1.0f / float(stops.size());
	
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

		if (fabs(rgb_band_r_walk) >= 2.0f) {
			while (rgb_band_r_walk >= +1.0f) { rgb_band_r_walk -= 1.0f; }
			while (rgb_band_r_walk <= -1.0f) { rgb_band_r_walk += 1.0f; }
			rgb_band_r_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
		}
	
		if (fabs(rgb_band_g_walk) >= 2.0f) {
			while (rgb_band_g_walk >= +1.0f) { rgb_band_g_walk -= 1.0f; }
			while (rgb_band_g_walk <= -1.0f) { rgb_band_g_walk += 1.0f; }
			rgb_band_g_walk_step = disf(gen) * (disi(gen) ? 1.0f : -1.0f);
		}
	
		if (fabs(rgb_band_b_walk) >= 2.0f) {
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

		float rgb_walk = (       float(fmodf(now * (1.0 / 5.0) * speed, 1.0)));
		float val_walk = (1.0f - float(fmodf(now               * speed, 1.0)));

		colors::hsp col(rgb_walk, 1.0f, 1.0f);
		for (size_t c = 0; c < leds_rings_n; c++) {
			float mod_walk = val_walk + (1.0f - (c * ( 1.0f / float(leds_rings_n) ) ) );
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

		float rgb_walk = (       float(fmodf(now * (1.0 / 5.0) * speed, 1.0)));
		float val_walk = (1.0f - float(fmodf(now               * speed, 1.0)));
	
		colors::hsv col(rgb_walk, 1.0f, 1.0f);
		for (size_t c = 0; c < leds_rings_n; c++) {
			float mod_walk = val_walk + ( 1.0f - ( c * ( 1.0f / float(leds_rings_n) ) ) );
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

		float rgb_walk = (float(fmodf(now * (1.0 / 5.0) * speed, 1.0)));
	
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

		size_t index = random.get(int32_t(0),int32_t(16*2 * 16));
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

		size_t index = random.get(int32_t(0),int32_t(leds_rings_n*2));
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

		size_t index = random.get(int32_t(0),int32_t(leds_rings_n*2));
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

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void brilliance() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		static colors::gradient bw;
		if (bw.check_init()) {
			const geom::float4 bwg[] = {
				geom::float4(0x206090, 0.00f),
				geom::float4(0x206090, 0.14f),
				geom::float4(0xffffff, 0.21f),
				geom::float4(0x206090, 0.28f),
				geom::float4(0x206090, 1.00f)};
			bw.init(bwg,5);
		}

		calc_outer([=](geom::float4 pos) {
			pos *= 0.50f;
			pos += now * 8.0f;
			pos %= 64.0f;
			pos *= 0.05;
			return bw.clamp(pos.x);
		});
	}

	void effect_11() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_12() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_13() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_14() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_15() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_16() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_17() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_18() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_19() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_20() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_21() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_22() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_23() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_24() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_25() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_26() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_27() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_28() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_29() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_30() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_31() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
		});
	}

	void effect_32() {
		led_bank::set_bird_color(colors::rgb(Model::instance().BirdColor()));

		float now = (float)Model::instance().Time();

		calc_outer([=](geom::float4 pos) {
			pos.x *= sinf(now);
			pos.y *= cosf(now);
			return pos;
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
			if (blend) {
				leds_inner[0][c] = colors::ip(leds_inner[0][c], out, blend);
				leds_inner[1][c] = colors::ip(leds_inner[1][c], out, blend);
			} else {
				leds_inner[0][c] = out;
				leds_inner[1][c] = out;
			}
		}
		if (blend) {
			leds_centr[0] = colors::ip(leds_centr[0], out, blend);
			leds_centr[1] = colors::ip(leds_centr[1], out, blend);
		} else {
			leds_centr[0] = out;
			leds_centr[1] = out;
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

			if (blend) {
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

		if (blend) {
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
		int32_t direction = int32_t((now - span.time)  * speed) & 1;
		float color_walk = float(fmodf((now - span.time) * speed, 1.0));

		colors::rgb8out out = colors::rgb8out(colors::rgb(color) * (direction ? (1.0f - color_walk) : color_walk) * 1.6f );

		for (size_t c = 0; c < leds_rings_n; c++) {

			if (blend) {
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

		if (blend) {
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
	static Timeline::Span span;

	if (Timeline::instance().Scheduled(span)) {
		return;
	}

	if (remove) {
		span.time = Model::instance().Time();
		span.duration = 0.25f;
		return;
	}

	span.type = Timeline::Span::Effect;
	span.time = Model::instance().Time();
	span.duration = 8.0f;
	span.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
		led_bank::instance().message_v2(color, span, below);
	};
	span.commitFunc = [=](Timeline::Span &) {
		led_bank::instance().update_leds();
	};

	Timeline::instance().Add(span);
}

void led_control::PerformColorBirdDisplay(colors::rgb8 color, bool remove) {
	static Timeline::Span span;

	static colors::rgb8 passThroughColor;

	passThroughColor = color;

	if (remove) {
		span.time = Model::instance().Time();
		span.duration = 0.25f;
		return;
	}

	if (Timeline::instance().Scheduled(span)) {
		return;
	}

	span.type = Timeline::Span::Effect;
	span.time = Model::instance().Time();
	span.duration = std::numeric_limits<double>::infinity();
	span.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
		led_bank::instance().bird_color(passThroughColor, span, below);
	};
	span.commitFunc = [=](Timeline::Span &) {
		led_bank::instance().update_leds();
	};

	Timeline::instance().Add(span);
}

void led_control::PerformMessageColorDisplay(colors::rgb8 color, bool remove) {
	static Timeline::Span span;

	static colors::rgb8 passThroughColor;
	
	passThroughColor = color;

	if (remove) {
		span.time = Model::instance().Time();
		span.duration = 0.25f;
		return;
	}

	if (Timeline::instance().Scheduled(span)) {
		return;
	}

	span.type = Timeline::Span::Effect;
	span.time = Model::instance().Time();
	span.duration = std::numeric_limits<double>::infinity();
	span.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
		led_bank::instance().message_color(passThroughColor, span, below);
	};
	span.commitFunc = [=](Timeline::Span &) {
		led_bank::instance().update_leds();
	};

	Timeline::instance().Add(span);
}
