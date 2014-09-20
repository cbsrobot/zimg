#ifndef ZIMG_UNRESIZE_IMPL_H_
#define ZING_UNRESIZE_IMPL_H_

#include "Common/osdep.h"
#include "bilinear.h"

namespace zimg {;
namespace unresize {;

FORCE_INLINE inline void filter_scanline_h_forward(const BilinearContext &ctx, const float * RESTRICT src, float * RESTRICT tmp, int src_stride,
                                                   int i, int j_begin, int j_end)
{
	const float *c = ctx.lu_c.data();
	const float *l = ctx.lu_l.data();

	float z = j_begin ? src[i * src_stride + j_begin - 1] : 0;

	// Matrix-vector product, and forward substitution loop.
	for (int j = j_begin; j < j_end; ++j) {
		const float *row = ctx.matrix_coefficients.data() + j * ctx.matrix_row_stride;
		int left = ctx.matrix_row_offsets[j];

		float accum = 0;
		for (int k = 0; k < ctx.matrix_row_size; ++k) {
			accum += row[k] * src[i * src_stride + left + k];
		}

		z = (accum - c[j] * z) * l[j];
		tmp[j] = z;
	}
}

FORCE_INLINE inline void filter_scanline_h_back(const BilinearContext &ctx, const float * RESTRICT tmp, float * RESTRICT dst, int dst_stride,
                                                int i, int j_begin, int j_end)
{
	const float *u = ctx.lu_u.data();
	float w = j_begin < ctx.dst_width ? dst[i * dst_stride + j_begin] : 0;

	// Backward substitution.
	for (int j = j_begin; j > j_end; --j) {
		w = tmp[j - 1] - u[j - 1] * w;
		dst[i * dst_stride + j - 1] = w;
	}
}

FORCE_INLINE inline void filter_scanline_v_forward(const BilinearContext &ctx, const float * RESTRICT src, float * RESTRICT dst, int src_stride, int dst_stride,
                                                   int i, int j_begin, int j_end)
{
	const float *c = ctx.lu_c.data();
	const float *l = ctx.lu_l.data();

	const float *row = ctx.matrix_coefficients.data() + i * ctx.matrix_row_stride;
	int top = ctx.matrix_row_offsets[i];

	for (int j = j_begin; j < j_end; ++j) {
		float z = i ? dst[(i - 1) * dst_stride + j] : 0;

		float accum = 0;
		for (int k = 0; k < ctx.matrix_row_size; ++k) {
			accum += row[k] * src[(top + k) * src_stride + j];
		}

		z = (accum - c[i] * z) * l[i];
		dst[i * dst_stride + j] = z;
	}
}

FORCE_INLINE inline void filter_scanline_v_back(const BilinearContext &ctx, float * RESTRICT dst, int dst_stride, int i, int j_begin, int j_end)
{
	const float *u = ctx.lu_u.data();

	for (int j = j_begin; j < j_end; ++j) {
		float w = i < ctx.dst_width ? dst[i * dst_stride + j] : 0;

		w = dst[(i - 1) * dst_stride + j] - u[i - 1] * w;
		dst[(i - 1) * dst_stride + j] = w;
	}
}


/**
 * Base class for implementations of unresizing filter.
 */
class UnresizeImpl {
protected:
	/**
	 * Coefficients for the horizontal pass.
	 */
	BilinearContext m_hcontext;

	/**
	 * Coefficients for the vertical pass.
	 */
	BilinearContext m_vcontext;

	/**
	 * Initialize the implementation with the given coefficients.
	 *
	 * @param hcontext horizontal coefficients
	 * @param vcontext vertical coefficients
	 */
	UnresizeImpl(const BilinearContext &hcontext, const BilinearContext &vcontext);
public:
	/**
	 * Destroy implementation
	 */
	virtual ~UnresizeImpl() = 0;

	virtual void process_f32_h(const float *src, float *dst, float *tmp,
	                           int src_width, int src_height, int src_stride, int dst_stride) const = 0;

	virtual void process_f32_v(const float *src, float *dst, float *tmp,
	                           int src_width, int src_height, int src_stride, int dst_stride) const = 0;
};

/**
 * Create and allocate a execution kernel.
 *
 * @param src_width upsampled image width
 * @param src_height upsampled image height
 * @param dst_width unresized image width
 * @param dst_height unresized image height
 * @param shift_w horizontal center shift relative to upsampled image
 * @param shift_h vertical center shift relative to upsampled image
 * @param x86 whether to create an x86-optimized kernel
 * @return a pointer to the allocated kernel
 * @throws ZimgIllegalArgument on invalid dimensions
 * @throws ZimgUnsupportedError if not supported
 */
UnresizeImpl *create_unresize_impl(int src_width, int src_height, int dst_width, int dst_height, float shift_w, float shift_h, bool x86);

} // namespace unresize
} // namespace zimg

#endif // UNRESIZE_IMPL_H