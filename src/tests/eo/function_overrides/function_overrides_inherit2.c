#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "Eo.h"
#include "function_overrides_simple.h"
#include "function_overrides_inherit.h"
#include "function_overrides_inherit2.h"

#include "../eunit_tests.h"

#define MY_CLASS INHERIT2_CLASS

static void
_a_set(Eo *obj, void *class_data EINA_UNUSED, int a)
{
   printf("%s %d\n", eo_class_name_get(MY_CLASS), a);
   eo2_do(obj, simple_a_print());
   eo2_do_super(obj, MY_CLASS, simple_a_set(a + 1));

   Eina_Bool called;
   eo2_do_super(obj, MY_CLASS, called = simple_a_print());
   fail_if(!called);
}

static Eina_Bool
_print(Eo *obj, void *class_data EINA_UNUSED)
{
   Eina_Bool called;
   printf("Hey\n");
   eo2_do_super(obj, MY_CLASS, called = inherit2_print());
   fail_if(called);

   return EINA_TRUE;
}

static Eina_Bool
_print2(Eo *obj EINA_UNUSED, void *class_data EINA_UNUSED)
{
   printf("Hey2\n");

   return EINA_TRUE;
}

static Eina_Bool
_class_print(Eo_Class *klass, void *data EINA_UNUSED)
{
   Eina_Bool called;
   printf("Print %s-%s\n", eo_class_name_get(klass), eo_class_name_get(MY_CLASS));
   eo2_do_super(klass, MY_CLASS, called = simple_class_print());
   fail_if(!called);

   eo2_do_super(klass, MY_CLASS, called = simple_class_print2());
   fail_if(!called);

   return EINA_TRUE;
}

EAPI EO2_FUNC_BODY(inherit2_print, Eina_Bool, EINA_FALSE);
EAPI EO2_FUNC_BODY(inherit2_print2, Eina_Bool, EINA_FALSE);

static Eo2_Op_Description op_descs[] = {
     EO2_OP_FUNC(inherit2_print, _print, "Print hey"),
     EO2_OP_FUNC(inherit2_print2, _print2, "Print hey2"),
     EO2_OP_CLASS_FUNC_OVERRIDE(simple_class_print, _class_print),
     EO2_OP_FUNC_OVERRIDE(simple_a_set, _a_set),
     EO2_OP_SENTINEL
};

static const Eo_Class_Description class_desc = {
     EO2_VERSION,
     "Inherit2",
     EO_CLASS_TYPE_REGULAR,
     EO2_CLASS_DESCRIPTION_OPS(op_descs),
     NULL,
     0,
     NULL,
     NULL
};

EO_DEFINE_CLASS(inherit2_class_get, &class_desc, INHERIT_CLASS, NULL);

