#include "./leds.h"

#include <atmel_start.h>

#include <algorithm>
#include <array>
#include <random>
#include <limits>
#include <stdio.h>
#include <cmath>
#include <cfloat>

#include "./model.h"
#include "./timeline.h"

namespace colors {

	static constexpr float global_limit_factor = 0.606f;

	class rgb8;
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

		friend rgb operator+(rgb a, const rgb &b) {
			a += b;
			return a;
		}

		rgb &operator-=(const rgb &v) {
			r -= v.r;
			g -= v.g;
			b -= v.b;
			return *this;
		}

		friend rgb operator-(rgb a, const rgb &b) {
			a -= b;
			return a;
		}

		rgb &operator*=(const rgb &v) {
			r *= v.r;
			g *= v.g;
			b *= v.b;
			return *this;
		}

		friend rgb operator*(rgb a, const rgb &b) {
			a *= b;
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

		friend rgb operator/(rgb a, const rgb &b) {
			a /= b;
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
			r = sat8(from.r * global_limit_factor);
			g = sat8(from.g * global_limit_factor);
			b = sat8(from.b * global_limit_factor);
			x = 0;
		}

		explicit rgb8(uint8_t _r, uint8_t _g, uint8_t _b) :
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

	rgb::rgb(const rgb8 &from) {
		r = float(from.r) * (1.0f/255.0f);
		g = float(from.g) * (1.0f/255.0f);
		b = float(from.b) * (1.0f/255.0f);
	}

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
			float d = fabs(hi - lo);
		
			h = 0;
			s = 0;
			l = (hi + lo) * 0.5f;
		
			if (d > 0.00001f) {
				s = d / ( 1.0f - fabs( 2.0f * l - 1.0f ) );
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
		  *P = sqrt(R * R * Pr + G * G * Pg + B * B * Pb);

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

	static rgb8 ip(const rgb8 &a, const rgb8 &b, float i) {
		if ( i <= 0.0f ) {
			return a;
		} else if ( i >= 1.0f) {
			return b;
		} else {
			int32_t ii = int32_t(i * 256.0f);
			return rgb8(
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

static void _qspi_memcpy(uint8_t *dst, uint8_t *src, uint32_t count)
{
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
	}
#endif  // #if 0
	__asm volatile (
		"sub %[count], #1\n\t"
		"1:\n\t"
		"ldrb r1, [%[src]], #1\n\t"
		"strb r1, [%[dst]], #1\n\t"
		"subs %[count], #1\n\t"
		"bhs 1b\n\t"
		:
		: [dst] "r" (dst), [src] "r" (src), [count] "r" (count)
		: "r1", "cc", "memory"
	);
}

class led_bank {
	static constexpr size_t ws2812_commit_time = 384;
	static constexpr size_t ws2812_rails = 4;

	static constexpr size_t leds_rings_n = 16;

	static constexpr size_t leds_components = 3;

	static constexpr size_t leds_buffer_size = ws2812_commit_time * ws2812_rails * 2 + ( leds_rings_n + 1 ) * ws2812_rails * leds_components * 8;

	colors::rgb8 leds_centr[2];

	colors::rgb8 leds_outer[2][leds_rings_n];
	colors::rgb8 leds_inner[2][leds_rings_n];

	bool initialized = false;

public:

	static led_bank &instance() {
		static led_bank leds;
		if (!leds.initialized) {
			leds.initialized = true;
			leds.init();
		}
		return leds;
	}

	void init() {
		qspi_sync_enable(&QUAD_SPI_0);

		static Timeline::Span span;

		static uint32_t current_effect = 0;
		static uint32_t previous_effect = 0;
		static double switch_time = 0;

		span.type = Timeline::Span::Effect;
		span.time = 0;
		span.duration = std::numeric_limits<double>::infinity();

		span.calcFunc = [=](Timeline::Span &, Timeline::Span &) {

			if ( current_effect != Model::instance().CurrentEffect() ) {
				previous_effect = current_effect;
				current_effect = Model::instance().CurrentEffect();
				switch_time = Model::instance().CurrentTime();
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
						burn_test();
					break;
				}
			};

			double blend_duration = 0.5f;
			double now = Model::instance().CurrentTime();

			if ((now - switch_time) < blend_duration) {
				calc_effect(previous_effect);

				colors::rgb8 leds_centr_prev[2];
				colors::rgb8 leds_outer_prev[2][leds_rings_n];
				colors::rgb8 leds_inner_prev[2][leds_rings_n];

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
		current_effect = Model::instance().CurrentEffect();
		switch_time = Model::instance().CurrentTime();
	}

	void enable_leds() {
		gpio_set_pin_level(ENABLE_E, true);
		gpio_set_pin_level(ENABLE_O, true);
	}

	void disable_leds() {
		gpio_set_pin_level(ENABLE_E, false);
		gpio_set_pin_level(ENABLE_O, false);
	}

	void update_leds() {

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

#if 1
		static const uint8_t disabled_inner_leds_top[16] = {
			0x1, 0x0, 0x0, 0x1,
			0x0, 0x0, 0x0, 0x0,
			0x1, 0x0, 0x0, 0x0,
			0x0, 0x1, 0x0, 0x0
		};

		static const uint8_t disabled_inner_leds_bottom[16] = {
			0x1, 0x0, 0x0, 0x1,
			0x0, 0x0, 0x0, 0x0,
			0x1, 0x0, 0x0, 0x0,
			0x0, 0x1, 0x0, 0x0
		};
#else  // #if 0
		static const uint8_t disabled_inner_leds_top[16] = {
			0x1, 0x1, 0x1, 0x1,
			0x1, 0x1, 0x1, 0x1,
			0x1, 0x1, 0x1, 0x1,
			0x1, 0x1, 0x1, 0x1,
		};

		static const uint8_t disabled_inner_leds_bottom[16] = {
			0x1, 0x1, 0x1, 0x1,
			0x1, 0x1, 0x1, 0x1,
			0x1, 0x1, 0x1, 0x1,
			0x1, 0x1, 0x1, 0x1,
		};
#endif  // #if 0

		struct _qspi_command cmd;
		memset(&cmd, 0, sizeof(cmd));
		cmd.inst_frame.bits.width = QSPI_INST4_ADDR4_DATA4;
		cmd.inst_frame.bits.data_en = 1;
		cmd.inst_frame.bits.tfr_type = QSPI_WRITE_ACCESS;
	
		int32_t brightness = int32_t(Model::instance().CurrentBrightness() * 256);

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

		static bool pending = false;

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
	}

	void set_bird_color(const colors::rgb &col) {

		const colors::rgb8 col8(col);

		leds_centr[0] = col8;
		leds_centr[1] = col8;

		for (size_t c = 0; c < leds_rings_n; c++) {
			leds_inner[0][c] = col8;
			leds_inner[1][c] = col8;
		}

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
		led_bank::set_bird_color(colors::rgb(Model::instance().CurrentBirdColor()));

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
			colors::rgb8 out = colors::rgb8(colors::rgb(band_r[c], band_g[c], band_b[c])); 		
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
		led_bank::set_bird_color(colors::rgb(Model::instance().CurrentBirdColor()));

		double now = Model::instance().CurrentTime();

		const double speed = 2.0;

		float rgb_walk = (       float(fmod(now * (1.0 / 5.0) * speed, 1.0)));
		float val_walk = (1.0f - float(fmod(now               * speed, 1.0)));

		colors::hsp col(rgb_walk, 1.0f, 1.0f);
		for (size_t c = 0; c < leds_rings_n; c++) {
			float mod_walk = val_walk + (1.0f - (c * ( 1.0f / float(leds_rings_n) ) ) );
			if (mod_walk > 1.0f) {
				mod_walk -= 1.0f;
			}
			col.p = std::min(1.0f, mod_walk);

			colors::rgb8 out = colors::rgb8(colors::rgb(col)); 		

			leds_outer[0][c] = out;
			leds_outer[1][leds_rings_n-1-c] = out;
		}

	}

	//
	// LIGHT WALKER
	//

	void light_walker() {
		led_bank::set_bird_color(colors::rgb(Model::instance().CurrentBirdColor()));

		double now = Model::instance().CurrentTime();

		const double speed = 2.0;

		float rgb_walk = (       float(fmod(now * (1.0 / 5.0) * speed, 1.0)));
		float val_walk = (1.0f - float(fmod(now               * speed, 1.0)));
	
		colors::hsv col(rgb_walk, 1.0f, 1.0f);
		for (size_t c = 0; c < leds_rings_n; c++) {
			float mod_walk = val_walk + ( 1.0f - ( c * ( 1.0f / float(leds_rings_n) ) ) );
			if (mod_walk > 1.0f) {
				mod_walk -= 1.0f;
			}
		
			col.s = 1.0f - std::min(1.0f, mod_walk);
			col.v = std::min(1.0f, mod_walk);

			colors::rgb8 out = colors::rgb8(colors::rgb(col)); 		

			leds_outer[0][c] = out;
			leds_outer[1][leds_rings_n-1-c] = out;
		}
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
//			burn_test_flip = disf(gen);

			colors::rgb8 out = colors::rgb8(colors::rgb(1.0f, 1.0f, 1.0f) * burn_test_flip );

			leds_outer[0][c] = out;
			leds_outer[1][c] = out;

			leds_inner[0][c] = out;
			leds_inner[1][c] = out;
		}
	}

	//
	// MESSAGE
	//

	void voltage(Timeline::Span &, Timeline::Span &) {
		led_bank::set_bird_color(colors::rgb(Model::instance().CurrentBirdColor()));


		for (size_t c = 0; c < leds_rings_n; c++) {
			leds_outer[0][c] = colors::rgb8();
			leds_outer[1][c] = colors::rgb8();
		}

		int32_t nl = int32_t(Model::instance().CurrentVbusVoltage());
		for (int32_t c = 0; c < nl; c++) {
			leds_outer[0][c+1] = colors::rgb8(colors::rgb(1.0f, 1.0f, 1.0f));
			leds_outer[1][c+1] = colors::rgb8(colors::rgb(1.0f, 1.0f, 1.0f));
		}

		int32_t nr = int32_t(Model::instance().CurrentSystemVoltage());
		for (int32_t c = 0; c < nr; c++) {
			leds_outer[0][15-c] = colors::rgb8(colors::rgb(1.0f, 1.0f, 1.0f));
			leds_outer[1][15-c] = colors::rgb8(colors::rgb(1.0f, 1.0f, 1.0f));
		}
	}


	void message(uint32_t color, Timeline::Span &span, Timeline::Span &below) {

		float blend = 0.0f;
		if (span.InBeginPeriod(blend, 0.25f)) {
			below.Calc();
		} else if (span.InEndPeriod(blend, 0.25f)) {
			below.Calc();
			blend = 1.0f - blend;
		}

		const double speed = 3.0;
		double now = Model::instance().CurrentTime();
		int32_t direction = int32_t((now - span.time)  * speed) & 1;
		float color_walk = float(fmod((now - span.time) * speed, 1.0));

		colors::rgb8 out = colors::rgb8(colors::rgb(color) * (direction ? (1.0f - color_walk) : color_walk) * 1.6f );

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

void led_control::PerformVoltageDisplay() {
	static Timeline::Span voltage;

	if (Timeline::instance().Scheduled(voltage)) {
		return;
	}

	voltage.type = Timeline::Span::Effect;
	voltage.time = Model::instance().CurrentTime();
	voltage.duration = 0.5f;
	voltage.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
		led_bank::instance().voltage(span, below);
	};
	voltage.commitFunc = [=](Timeline::Span &) {
		led_bank::instance().update_leds();
	};

	Timeline::instance().Add(voltage);
}

void led_control::PerformV2MessageEffect(uint32_t color) {
	static Timeline::Span message;

	if (Timeline::instance().Scheduled(message)) {
		return;
	}

	message.type = Timeline::Span::Effect;
	message.time = Model::instance().CurrentTime();
	message.duration = 8.0f;
	message.calcFunc = [=](Timeline::Span &span, Timeline::Span &below) {
		led_bank::instance().message(color, span, below);
	};
	message.commitFunc = [=](Timeline::Span &) {
		led_bank::instance().update_leds();
	};

	Timeline::instance().Add(message);
}
