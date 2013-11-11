#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eo.h"
#include "mixin_mixin.h"
#include "mixin_mixin3.h"
#include "mixin_simple.h"

#include "../eunit_tests.h"

#define MY_CLASS MIXIN3_CLASS

static int
_ab_sum_get(Eo *obj, void *class_data EINA_UNUSED)
{
   /* This cast is just a hack for the test. */
   Mixin3_Public_Data *pd = (Mixin3_Public_Data *) class_data;
   int sum;
   printf("%s %s\n", eo_class_name_get(MY_CLASS), __func__);
   eo2_do_super(obj, MY_CLASS, sum = mixin_ab_sum_get());

   ++sum;
   pd->count += 3;

     {
        int _a, _b;
        eo2_do(obj, _a = simple_a_get(), _b = simple_b_get());
        fail_if(sum != _a + _b + 2);
     }

   return sum;
}

static void
_constructor(Eo *obj, void *class_data EINA_UNUSED, va_list *list EINA_UNUSED)
{
   eo2_do_super(obj, MY_CLASS, eo2_constructor());
}

static void
_destructor(Eo *obj, void *class_data EINA_UNUSED, va_list *list EINA_UNUSED)
{
   eo2_do_super(obj, MY_CLASS, eo2_destructor());
}

static Eo2_Op_Description op_descs[] = {
     EO2_OP_FUNC_OVERRIDE(eo2_constructor, _constructor),
     EO2_OP_FUNC_OVERRIDE(eo2_destructor, _destructor),
     EO2_OP_FUNC_OVERRIDE(mixin_ab_sum_get, _ab_sum_get),
     EO2_OP_SENTINEL
};

static const Eo_Class_Description class_desc = {
     EO2_VERSION,
     "Mixin3",
     EO_CLASS_TYPE_MIXIN,
     EO2_CLASS_DESCRIPTION_OPS(op_descs),
     NULL,
     sizeof(Mixin3_Public_Data),
     NULL,
     NULL
};

EO_DEFINE_CLASS(mixin3_class_get, &class_desc, MIXIN_CLASS, NULL);

