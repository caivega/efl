#include "evas_filter.h"
#include "evas_filter_private.h"

#include <math.h>
#include <time.h>

#if DIV_USING_BITSHIFT
static int
_smallest_pow2_larger_than(int val)
{
   int n;

   for (n = 0; n < 32; n++)
     if (val <= (1 << n)) return n;

   ERR("Value %d is too damn high!", val);
   return 32;
}

/* Input:
 *  const int pow2 = _smallest_pow2_larger_than(divider * 1024);
 *  const int numerator = (1 << pow2) / divider;
 * Result:
 *  r = ((val * numerator) >> pow2);
 */
# define DEFINE_DIAMETER(rad) const int pow2 = _smallest_pow2_larger_than((radius * 2 + 1) << 10); const int numerator = (1 << pow2) / (radius * 2 + 1);
# define DIVIDE_BY_DIAMETER(val) (((val) * numerator) >> pow2)
#else
# define DEFINE_DIAMETER(rad) const int diameter = radius * 2 + 1;
# define DIVIDE_BY_DIAMETER(val) ((val) / diameter)
#endif

/* RGBA functions */

static void
_box_blur_step_rgba(DATA32 *src, DATA32 *dst, int radius, int len, int step)
{
   DEFINE_DIAMETER(radius);
   int acc[4] = {0};
   DATA8 *d, *sr, *sl;
   int x, k;
   int divider;
   int left = MIN(radius, len);
   int right = MIN(radius, (len - radius));

   d = (DATA8 *) dst;
   sl = (DATA8 *) src;
   sr = (DATA8 *) src;

   // Read-ahead
   for (x = left; x; x--)
     {
        for (k = 0; k < 4; k++)
          acc[k] += sr[k];
        sr += step;
     }

   // Left
   for (x = 0; x < left; x++)
     {
        for (k = 0; k < 4; k++)
          acc[k] += sr[k];
        sr += step;

        divider = x + left + 1;
        d[ALPHA] = acc[ALPHA] / divider;
        d[RED]   = acc[RED]   / divider;
        d[GREEN] = acc[GREEN] / divider;
        d[BLUE]  = acc[BLUE]  / divider;
        d += step;
     }

   // Main part
   for (x = len - (2 * radius); x > 0; x--)
     {
        for (k = 0; k < 4; k++)
          acc[k] += sr[k];
        sr += step;

        d[ALPHA] = DIVIDE_BY_DIAMETER(acc[ALPHA]);
        d[RED]   = DIVIDE_BY_DIAMETER(acc[RED]);
        d[GREEN] = DIVIDE_BY_DIAMETER(acc[GREEN]);
        d[BLUE]  = DIVIDE_BY_DIAMETER(acc[BLUE]);
        d += step;

        for (k = 0; k < 4; k++)
          acc[k] -= sl[k];
        sl += step;
     }

   // Right part
   for (x = right; x; x--)
     {
        divider = x + right;
        d[ALPHA] = acc[ALPHA] / divider;
        d[RED]   = acc[RED]   / divider;
        d[GREEN] = acc[GREEN] / divider;
        d[BLUE]  = acc[BLUE]  / divider;
        d += step;

        for (k = 0; k < 4; k++)
          acc[k] -= sl[k];
        sl += step;
     }
}

static void
_box_blur_horiz_rgba(DATA32 *src, DATA32 *dst, int radius, int w, int h)
{
   int y;
   int step = sizeof(DATA32);

   DEBUG_TIME_BEGIN();

   for (y = 0; y < h; y++)
     {
        _box_blur_step_rgba(src, dst, radius, w, step);
        src += w;
        dst += w;
     }

   DEBUG_TIME_END();
}

static void
_box_blur_vert_rgba(DATA32 *src, DATA32 *dst, int radius, int w, int h)
{
   int x;
   int step = w * sizeof(DATA32);

   DEBUG_TIME_BEGIN();

   for (x = 0; x < w; x++)
     {
        _box_blur_step_rgba(src, dst, radius, h, step);
        src += 1;
        dst += 1;
     }

   DEBUG_TIME_END();
}

static Eina_Bool
_box_blur_horiz_apply_rgba(Evas_Filter_Command *cmd)
{
   RGBA_Image *in, *out;
   unsigned int r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input->backing, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output->backing, EINA_FALSE);

   r = abs(cmd->blur.dx);
   in = cmd->input->backing;
   out = cmd->output->backing;

   EINA_SAFETY_ON_NULL_RETURN_VAL(in->image.data, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out->image.data, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(out->cache_entry.w >= (2*r + 1), EINA_FALSE);

   _box_blur_horiz_rgba(in->image.data, out->image.data, r,
                        in->cache_entry.w, in->cache_entry.h);

   return EINA_TRUE;
}

static Eina_Bool
_box_blur_vert_apply_rgba(Evas_Filter_Command *cmd)
{
   RGBA_Image *in, *out;
   unsigned int r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input->backing, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output->backing, EINA_FALSE);

   r = abs(cmd->blur.dy);
   in = cmd->input->backing;
   out = cmd->output->backing;

   EINA_SAFETY_ON_NULL_RETURN_VAL(in->image.data, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out->image.data, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(out->cache_entry.h >= (2*r + 1), EINA_FALSE);

   _box_blur_vert_rgba(in->image.data, out->image.data, r,
                       in->cache_entry.w, in->cache_entry.h);

   return EINA_TRUE;
}

/* Alpha only functions */

/* Box blur */

static void
_box_blur_step_alpha(DATA8 *src, DATA8 *dst, int radius, int len, int step)
{
   int k;
   int acc = 0;
   DATA8 *sr = src, *sl = src, *d = dst;
   DEFINE_DIAMETER(radius);
   int left = MIN(radius, len);
   int right = MIN(radius, (len - radius));

   for (k = left; k; k--)
     {
        acc += *sr;
        sr += step;
     }

   for (k = 0; k < left; k++)
     {
        acc += *sr;
        *d = acc / (k + left + 1);
        sr += step;
        d += step;
     }

   for (k = len - (2 * radius); k; k--)
     {
        acc += *sr;
        *d = DIVIDE_BY_DIAMETER(acc);
        acc -= *sl;
        sl += step;
        sr += step;
        d += step;
     }

   for (k = right; k; k--)
     {
        *d = acc / (k + right);
        acc -= *sl;
        d += step;
        sl += step;
     }
}

static void
_box_blur_horiz_alpha(DATA8 *src, DATA8 *dst, int radius, int w, int h)
{
   int k;

   DEBUG_TIME_BEGIN();

   for (k = h; k; k--)
     {
        _box_blur_step_alpha(src, dst, radius, w, 1);
        dst += w;
        src += w;
     }

   DEBUG_TIME_END();
}

static void
_box_blur_vert_alpha(DATA8 *src, DATA8 *dst, int radius, int w, int h)
{
   int k;

   DEBUG_TIME_BEGIN();

   for (k = w; k; k--)
     {
        _box_blur_step_alpha(src, dst, radius, h, w);
        dst += 1;
        src += 1;
     }

   DEBUG_TIME_END();
}

static Eina_Bool
_box_blur_horiz_apply_alpha(Evas_Filter_Command *cmd)
{
   RGBA_Image *in, *out;
   unsigned int r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input->backing, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output->backing, EINA_FALSE);

   r = abs(cmd->blur.dx);
   in = cmd->input->backing;
   out = cmd->output->backing;

   EINA_SAFETY_ON_NULL_RETURN_VAL(in->image.data8, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out->image.data8, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(out->cache_entry.w >= (2*r + 1), EINA_FALSE);

   _box_blur_horiz_alpha(in->image.data8, out->image.data8, r,
                         in->cache_entry.w, in->cache_entry.h);

   return EINA_TRUE;
}

static Eina_Bool
_box_blur_vert_apply_alpha(Evas_Filter_Command *cmd)
{
   RGBA_Image *in, *out;
   unsigned int r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input->backing, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output->backing, EINA_FALSE);

   r = abs(cmd->blur.dy);
   in = cmd->input->backing;
   out = cmd->output->backing;

   EINA_SAFETY_ON_NULL_RETURN_VAL(in->image.data8, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out->image.data8, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(out->cache_entry.h >= (2*r + 1), EINA_FALSE);

   _box_blur_vert_alpha(in->image.data8, out->image.data8, r,
                        in->cache_entry.w, in->cache_entry.h);

   return EINA_TRUE;
}

/* Gaussian blur */

static void
_sin_blur_weights_get(int *weights, int *pow2_divider, int radius)
{
   const int diameter = 2 * radius + 1;
   double x, divider, sum = 0.0;
   double dweights[diameter];
   int k, nextpow2, isum = 0;
   const int FAKE_PI = 3.0;

   /* Base curve:
    * f(x) = sin(x+pi/2)/2+1/2
    */

   for (k = 0; k < diameter; k++)
     {
        x = ((double) k / (double) (diameter - 1)) * FAKE_PI * 2.0 - FAKE_PI;
        dweights[k] = ((sin(x + M_PI_2) + 1.0) / 2.0) * 1024.0;
        sum += dweights[k];
     }

   // Now we need to normalize to have a 2^N divider.
   nextpow2 = log2(2 * sum);
   divider = (double) (1 << nextpow2);

   for (k = 0; k < diameter; k++)
     {
        weights[k] = round(dweights[k] * divider / sum);
        isum += weights[k];
     }

   // Final correction. The difference SHOULD be small...
   weights[radius] += (int) divider - isum;

   if (pow2_divider)
     *pow2_divider = nextpow2;
}

#define FUNCTION_NAME _gaussian_blur_horiz_alpha_step
#define STEP 1
#include "./blur/blur_gaussian_alpha_.c"

static void
_gaussian_blur_horiz_alpha(const DATA8 *src, DATA8 *dst, int radius, int w, int h)
{
   int *weights;
   int pow2_div = 0;

   weights = alloca((2 * radius + 1) * sizeof(int));
   _sin_blur_weights_get(weights, &pow2_div, radius);

   DEBUG_TIME_BEGIN();
   _gaussian_blur_horiz_alpha_step(src, dst, radius, w, h, w, weights, pow2_div);
   DEBUG_TIME_END();
}

// Step size is w (row by row), loops = w, so STEP = 'loops'
#define FUNCTION_NAME _gaussian_blur_vert_alpha_step
#define STEP loops
#include "./blur/blur_gaussian_alpha_.c"

static void
_gaussian_blur_vert_alpha(const DATA8 *src, DATA8 *dst, int radius, int w, int h)
{
   int *weights;
   int pow2_div = 0;

   weights = alloca((2 * radius + 1) * sizeof(int));
   _sin_blur_weights_get(weights, &pow2_div, radius);

   DEBUG_TIME_BEGIN();
   _gaussian_blur_vert_alpha_step(src, dst, radius, h, w, 1, weights, pow2_div);
   DEBUG_TIME_END();
}

#define FUNCTION_NAME _gaussian_blur_horiz_rgba_step
#define STEP 1
#include "./blur/blur_gaussian_rgba_.c"

static void
_gaussian_blur_horiz_rgba(DATA32 *src, DATA32 *dst, int radius, int w, int h)
{
   int *weights;
   int pow2_div = 0;

   weights = alloca((2 * radius + 1) * sizeof(int));
   _sin_blur_weights_get(weights, &pow2_div, radius);

   DEBUG_TIME_BEGIN();
   _gaussian_blur_horiz_rgba_step(src, dst, radius, w, h, w, weights, pow2_div);
   DEBUG_TIME_END();
}

#define FUNCTION_NAME _gaussian_blur_vert_rgba_step
#define STEP loops
#include "./blur/blur_gaussian_rgba_.c"

static void
_gaussian_blur_vert_rgba(DATA32 *src, DATA32 *dst, int radius, int w, int h)
{
   int *weights;
   int pow2_div = 0;

   weights = alloca((2 * radius + 1) * sizeof(int));
   _sin_blur_weights_get(weights, &pow2_div, radius);

   DEBUG_TIME_BEGIN();
   _gaussian_blur_vert_rgba_step(src, dst, radius, h, w, 1, weights, pow2_div);
   DEBUG_TIME_END();
}

static Eina_Bool
_gaussian_blur_horiz_apply_alpha(Evas_Filter_Command *cmd)
{
   RGBA_Image *in, *out;
   unsigned int r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input->backing, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output->backing, EINA_FALSE);

   r = abs(cmd->blur.dx);
   in = cmd->input->backing;
   out = cmd->output->backing;

   EINA_SAFETY_ON_NULL_RETURN_VAL(in->image.data8, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out->image.data8, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(out->cache_entry.w >= (2*r + 1), EINA_FALSE);

   _gaussian_blur_horiz_alpha(in->image.data8, out->image.data8, r,
                              in->cache_entry.w, in->cache_entry.h);

   return EINA_TRUE;
}

static Eina_Bool
_gaussian_blur_vert_apply_alpha(Evas_Filter_Command *cmd)
{
   RGBA_Image *in, *out;
   unsigned int r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input->backing, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output->backing, EINA_FALSE);

   r = abs(cmd->blur.dy);
   in = cmd->input->backing;
   out = cmd->output->backing;

   EINA_SAFETY_ON_NULL_RETURN_VAL(in->image.data8, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out->image.data8, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(out->cache_entry.h >= (2*r + 1), EINA_FALSE);

   _gaussian_blur_vert_alpha(in->image.data8, out->image.data8, r,
                             in->cache_entry.w, in->cache_entry.h);

   return EINA_TRUE;
}

static Eina_Bool
_gaussian_blur_horiz_apply_rgba(Evas_Filter_Command *cmd)
{
   RGBA_Image *in, *out;
   unsigned int r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input->backing, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output->backing, EINA_FALSE);

   r = abs(cmd->blur.dx);
   in = cmd->input->backing;
   out = cmd->output->backing;

   EINA_SAFETY_ON_NULL_RETURN_VAL(in->image.data, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out->image.data, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(out->cache_entry.w >= (2*r + 1), EINA_FALSE);

   _gaussian_blur_horiz_rgba(in->image.data, out->image.data, r,
                             in->cache_entry.w, in->cache_entry.h);

   return EINA_TRUE;
}

static Eina_Bool
_gaussian_blur_vert_apply_rgba(Evas_Filter_Command *cmd)
{
   RGBA_Image *in, *out;
   unsigned int r;

   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input->backing, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output->backing, EINA_FALSE);

   r = abs(cmd->blur.dy);
   in = cmd->input->backing;
   out = cmd->output->backing;

   EINA_SAFETY_ON_NULL_RETURN_VAL(in->image.data, EINA_FALSE);
   EINA_SAFETY_ON_NULL_RETURN_VAL(out->image.data, EINA_FALSE);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(out->cache_entry.h >= (2*r + 1), EINA_FALSE);

   _gaussian_blur_vert_rgba(in->image.data, out->image.data, r,
                            in->cache_entry.w, in->cache_entry.h);

   return EINA_TRUE;
}

/* Main entry point */

Evas_Filter_Apply_Func
evas_filter_blur_cpu_func_get(Evas_Filter_Command *cmd)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->input, NULL);
   EINA_SAFETY_ON_NULL_RETURN_VAL(cmd->output, NULL);
   EINA_SAFETY_ON_FALSE_RETURN_VAL(cmd->mode == EVAS_FILTER_MODE_BLUR, NULL);

   switch (cmd->blur.type)
     {
      case EVAS_FILTER_BLUR_BOX:
        if (!cmd->input->alpha_only && !cmd->output->alpha_only)
          {
             if (cmd->blur.dx)
               return _box_blur_horiz_apply_rgba;
             else if (cmd->blur.dy)
               return _box_blur_vert_apply_rgba;
          }
        else if (cmd->input->alpha_only && cmd->output->alpha_only)
          {
             if (cmd->blur.dx)
               return _box_blur_horiz_apply_alpha;
             else if (cmd->blur.dy)
               return _box_blur_vert_apply_alpha;
          }
        CRI("Unsupported operation: mixing RGBA and Alpha surfaces.");
        return NULL;
      case EVAS_FILTER_BLUR_GAUSSIAN:
        if (!cmd->input->alpha_only && !cmd->output->alpha_only)
          {
             if (cmd->blur.dx)
               return _gaussian_blur_horiz_apply_rgba;
             else if (cmd->blur.dy)
               return _gaussian_blur_vert_apply_rgba;
          }
        else if (cmd->input->alpha_only && cmd->output->alpha_only)
          {
             if (cmd->blur.dx)
               return _gaussian_blur_horiz_apply_alpha;
             else if (cmd->blur.dy)
               return _gaussian_blur_vert_apply_alpha;
          }
        CRI("Unsupported operation: mixing RGBA and Alpha surfaces.");
        return NULL;
      default:
        CRI("Unsupported blur type %d", cmd->blur.type);
        return NULL;
     }
}
