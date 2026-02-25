#include <windows.h>
#include "ncbind.hpp"

struct   LongExposure {
	/**/~LongExposure() { term(); }
	/**/ LongExposure(iTJSDispatch2 *obj) : self(obj), buffer(0), width(0), height(0) {}

	void init() {
		term();
		if (!GetLayerSize(self, width, height))
			TVPThrowExceptionMessage(TJS_W("LongExposure.init: invalid layer image"));
		size_t len = width * height * 4;
		buffer = new tjs_uint32[len];
		memset(buffer, 0, sizeof(tjs_uint32)*len);
	}

	void snap() {
		if (!buffer)
			TVPThrowExceptionMessage(TJS_W("LongExposure.snap: not initialized"));
		size_t curw = 0, curh = 0;
		if (!GetLayerSize(self, curw, curh))
			TVPThrowExceptionMessage(TJS_W("LongExposure.snap: invalid layer image"));
		if (curw != width || curh != height)
			TVPThrowExceptionMessage(TJS_W("LongExposure.snap: invalid layer size"));

		const tjs_uint8 *ptr = 0;
		long pitch = 0;
		GetLayerImage(self, ptr, pitch);

		for (size_t y = 0; y < height; y++) {
			tjs_uint32* w = buffer + width*4 * y;
			const tjs_uint8* r = ptr + pitch * y;
			for (size_t x = 0; x < width; x++) {
				*w++ += (tjs_uint32)(*r++);
				*w++ += (tjs_uint32)(*r++);
				*w++ += (tjs_uint32)(*r++);
				*w++ += (tjs_uint32)(*r++);
			}
		}
	}

	struct MinMaxRGBA {
		struct UnitRGBA {
			UnitRGBA(tjs_uint32 n) : r(n), g(n), b(n), a(n) {}
			tjs_uint32 r, g, b, a;
		} _min, _max;
		MinMaxRGBA() : _min(0xFFFFFFFF), _max(0) {}
		inline void setMinMaxR(tjs_uint32 n) { setMinMax(n, _min.r, _max.r); }
		inline void setMinMaxG(tjs_uint32 n) { setMinMax(n, _min.g, _max.g); }
		inline void setMinMaxB(tjs_uint32 n) { setMinMax(n, _min.b, _max.b); }
		inline void setMinMaxA(tjs_uint32 n) { setMinMax(n, _min.a, _max.a); }
		inline tjs_uint8 getNormalizeR(tjs_uint32 n) { return getNormalize(n, _min.r, _max.r); }
		inline tjs_uint8 getNormalizeG(tjs_uint32 n) { return getNormalize(n, _min.g, _max.g); }
		inline tjs_uint8 getNormalizeB(tjs_uint32 n) { return getNormalize(n, _min.b, _max.b); }
		inline tjs_uint8 getNormalizeA(tjs_uint32 n) { return getNormalize(n, _min.a, _max.a); }
	private:
		inline void setMinMax(tjs_uint32 n, tjs_uint32 &min, tjs_uint32 &max) {
			if (n < min) min = n;
			if (n > max) max = n;
		}
		inline tjs_uint8 getNormalize(tjs_uint32 n, tjs_uint32 min, tjs_uint32 max) {
			if (min >= max) return 0xFF;
			if (n < min) n = min;
			if (n > max) n = max;
			return (tjs_uint8)((n - min) * 255 / (max - min));
		}
	};
	bool _stat(MinMaxRGBA &m) {
		for (size_t y = 0; y < height; y++) {
			const tjs_uint32* r = buffer + width*4 * y;
			for (size_t x = 0; x < width; x++) {
				m.setMinMaxB(*r++);
				m.setMinMaxG(*r++);
				m.setMinMaxR(*r++);
				m.setMinMaxA(*r++);
			}
		}
		return true;
	}
	tTJSVariant stat() {
		if (!buffer)
			TVPThrowExceptionMessage(TJS_W("LongExposure.stat: not initialized"));

		MinMaxRGBA m;
		if (!_stat(m))
			TVPThrowExceptionMessage(TJS_W("LongExposure.stat: invalid layer image"));

		ncbDictionaryAccessor rdic;
		tTJSVariant r;
		if (rdic.IsValid()) {
			rdic.SetValue(TJS_W("min_r"), (tTVInteger)m._min.r);
			rdic.SetValue(TJS_W("min_g"), (tTVInteger)m._min.g);
			rdic.SetValue(TJS_W("min_b"), (tTVInteger)m._min.b);
			rdic.SetValue(TJS_W("min_a"), (tTVInteger)m._min.a);
			rdic.SetValue(TJS_W("max_r"), (tTVInteger)m._max.r);
			rdic.SetValue(TJS_W("max_g"), (tTVInteger)m._max.g);
			rdic.SetValue(TJS_W("max_b"), (tTVInteger)m._max.b);
			rdic.SetValue(TJS_W("max_a"), (tTVInteger)m._max.a);
			r = tTJSVariant(rdic, rdic);
		}
		return r;
	}

	void copy(tTJSVariant v) {
		if (!buffer)
			TVPThrowExceptionMessage(TJS_W("LongExposure.copy: not initialized"));
		size_t curw = 0, curh = 0;
		if (!GetLayerSize(self, curw, curh))
			TVPThrowExceptionMessage(TJS_W("LongExposure.copy: invalid layer image"));
		if (curw != width || curh != height)
			TVPThrowExceptionMessage(TJS_W("LongExposure.copy: invalid layer size"));

		tjs_uint8 *ptr = 0;
		long pitch = 0;
		if (!GetLayerImageForWrite(self, ptr, pitch))
			TVPThrowExceptionMessage(TJS_W("LongExposure.copy: invalid layer image"));

		MinMaxRGBA m;
		switch (v.Type()) {
		case tvtVoid:
			if (!_stat(m))
				TVPThrowExceptionMessage(TJS_W("LongExposure.copy: stat failed"));
			break;
		case tvtObject:
			{
				ncbPropAccessor rdic(v);
				if (rdic.IsValid()) {
					m._min.r = rdic.getIntValue(TJS_W("min_r"));
					m._min.g = rdic.getIntValue(TJS_W("min_g"));
					m._min.b = rdic.getIntValue(TJS_W("min_b"));
					m._min.a = rdic.getIntValue(TJS_W("min_a"));
					m._max.r = rdic.getIntValue(TJS_W("max_r"));
					m._max.g = rdic.getIntValue(TJS_W("max_g"));
					m._max.b = rdic.getIntValue(TJS_W("max_b"));
					m._max.a = rdic.getIntValue(TJS_W("max_a"));
				}
			}
			break;
		}
		for (size_t y = 0; y < height; y++) {
			const tjs_uint32* r = buffer + width*4 * y;
			tjs_uint8* w = ptr + pitch * y;
			for (size_t x = 0; x < width; x++) {
				*w++ = m.getNormalizeB(*r++);
				*w++ = m.getNormalizeG(*r++);
				*w++ = m.getNormalizeR(*r++);
				*w++ = m.getNormalizeA(*r++);
			}
		}
	}

	void term() {
		if (buffer) delete[] buffer;
		buffer = 0;
		width = height = 0;
	}

private:
	iTJSDispatch2 *self;
	tjs_uint32 *buffer;
	size_t width, height;

	static iTJSDispatch2 *LayerClass;
	static bool GetLayerSize(iTJSDispatch2 *lay, size_t &w, size_t &h) {
		static ttstr hasImage   (TJS_W("hasImage"));
		static ttstr imageWidth (TJS_W("imageWidth"));
		static ttstr imageHeight(TJS_W("imageHeight"));

		tTVInteger lw, lh;
		if (!LayerPropGet(lay, hasImage) ||
			(lw = LayerPropGet(lay, imageWidth )) <= 0 ||
			(lh = LayerPropGet(lay, imageHeight)) <= 0) return false;
		w = (size_t)lw;
		h = (size_t)lh;
		return true;
	}
	static bool GetLayerImage(iTJSDispatch2 *lay, const tjs_uint8* &ptr, long &pitch) {
		static ttstr mainImageBufferPitch(TJS_W("mainImageBufferPitch"));
		static ttstr mainImageBuffer(TJS_W("mainImageBuffer"));

		tTVInteger lpitch, lptr;
		if ((lpitch = LayerPropGet(lay, mainImageBufferPitch)) == 0 ||
			(lptr   = LayerPropGet(lay, mainImageBuffer)) == 0) return false;
		pitch = (long)lpitch;
		ptr = reinterpret_cast<const tjs_uint8*>(lptr);
		return true;
	}
	static bool GetLayerImageForWrite(iTJSDispatch2 *lay, tjs_uint8* &ptr, long &pitch) {
		static ttstr mainImageBufferPitch(TJS_W("mainImageBufferPitch"));
		static ttstr mainImageBufferForWrite(TJS_W("mainImageBufferForWrite"));

		tTVInteger lpitch, lptr;
		if ((lpitch = LayerPropGet(lay, mainImageBufferPitch)) == 0 ||
			(lptr   = LayerPropGet(lay, mainImageBufferForWrite)) == 0) return false;
		pitch = (long)lpitch;
		ptr = reinterpret_cast<tjs_uint8*>(lptr);
		return true;
	}
	static tTVInteger LayerPropGet(iTJSDispatch2 *lay, ttstr &prop, tTVInteger defval = 0) {
		if (!LayerClass) {
			tTJSVariant var;
			TVPExecuteExpression(TJS_W("Layer"), &var);
			LayerClass = var.AsObjectNoAddRef();
		}
		tTJSVariant val;
		return (TJS_FAILED(LayerClass->PropGet(0, prop.c_str(), prop.GetHint(), &val, lay))) ? defval : val.AsInteger();
	}
};
iTJSDispatch2* LongExposure::LayerClass = 0;


NCB_GET_INSTANCE_HOOK(LongExposure)
{
	/**/  NCB_GET_INSTANCE_HOOK_CLASS () {}
	/**/ ~NCB_GET_INSTANCE_HOOK_CLASS () {}
	NCB_INSTANCE_GETTER(objthis) {
		ClassT* obj = GetNativeInstance(objthis);
		if (!obj) SetNativeInstance(objthis, (obj = new ClassT(objthis)));
		return obj;
	}
};
NCB_ATTACH_CLASS_WITH_HOOK(LongExposure, Layer)
{
	Method(TJS_W("initExposure"), &Class::init);
	Method(TJS_W("snapExposure"), &Class::snap);
	Method(TJS_W("statExposure"), &Class::stat);
	Method(TJS_W("copyExposure"), &Class::copy);
	Method(TJS_W("termExposure"), &Class::term);
}
