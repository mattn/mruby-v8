#include <errno.h>
#include <memory.h>
#include <mruby.h>
#include <mruby/proc.h>
#include <mruby/data.h>
#include <mruby/string.h>
#include <mruby/array.h>
#include <mruby/hash.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include "v8wrap.h"
#include <stdio.h>
#include <fcntl.h>

#if 1
#define ARENA_SAVE \
  int ai = mrb_gc_arena_save(mrb); \
  if (ai == MRB_ARENA_SIZE) { \
    mrb_raise(mrb, E_RUNTIME_ERROR, "arena overflow"); \
  }
#define ARENA_RESTORE \
  mrb_gc_arena_restore(mrb, ai);
#else
#define ARENA_SAVE
#define ARENA_RESTORE
#endif

static struct RClass *_class_v8;

typedef struct {
  void* v8context;
  mrb_value instance;
  mrb_state* mrb;
} mrb_v8context;

char*
_v8wrap_callback(unsigned int id, char* name, char* code) {
  return NULL;
}

static mrb_v8context*
v8context_alloc(mrb_state* mrb)
{
  mrb_v8context* context = (mrb_v8context*) malloc(sizeof(mrb_v8context));
  if (!context) return NULL;
  memset(context, 0, sizeof(mrb_v8context));
  context->v8context = v8_create();
  context->mrb = mrb;
  return context;
}

static void
v8context_free(mrb_state *mrb, void *p)
{
  mrb_v8context* context = (mrb_v8context*) p;
  v8_release(context->v8context);
  free(p);
}

static const struct mrb_data_type v8context_type = {
  "v8context", v8context_free,
};

static mrb_value
mrb_v8_init(mrb_state *mrb, mrb_value self)
{
  mrb_v8context* context = v8context_alloc(mrb);
  if (!context) {
    mrb_raise(mrb, E_RUNTIME_ERROR, "can't alloc memory");
  }
  context->instance = self;
  mrb_iv_set(mrb, self, mrb_intern(mrb, "context"), mrb_obj_value(
    Data_Wrap_Struct(mrb, mrb->object_class,
    &v8context_type, (void*) context)));
  return self;
}

static mrb_value
mrb_v8_eval(mrb_state *mrb, mrb_value self)
{
  mrb_value value_context;
  mrb_v8context* context = NULL;
  mrb_value arg;

  mrb_get_args(mrb, "S", &arg);

  value_context = mrb_iv_get(mrb, self, mrb_intern(mrb, "context"));
  Data_Get_Struct(mrb, value_context, &v8context_type, context);
  if (!context) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }

  char* ret = v8_execute(context->v8context, RSTRING_PTR(arg));
  mrb_value val = mrb_nil_value();
  if (ret) {
    mrb_value str = mrb_str_new_cstr(mrb, "{\"value\":");
    mrb_str_cat2(mrb, str, ret);
    mrb_str_cat2(mrb, str, "}");
    free(ret);

    struct RClass* clazz = mrb_class_get(mrb, "JSON");
    mrb_value args[1];
    args[0] = str;
    mrb_value hash = mrb_funcall_argv(mrb, mrb_obj_value(clazz), mrb_intern(mrb, "parse"), 1, args);
    val = mrb_hash_get(mrb, hash, mrb_str_new_cstr(mrb, "value"));
  }
  return val;
}

/* TODO */
/*
static mrb_value
mrb_v8_add_func(mrb_state *mrb, mrb_value self)
{
  return mrb_nil_value();
}
*/

/*********************************************************
 * register
 *********************************************************/

void
mrb_mruby_v8_gem_init(mrb_state* mrb) {
  ARENA_SAVE;

  _class_v8 = mrb_define_class(mrb, "V8", mrb->object_class);
  mrb_define_method(mrb, _class_v8, "initialize", mrb_v8_init, ARGS_NONE());
  mrb_define_method(mrb, _class_v8, "eval", mrb_v8_eval, ARGS_ANY());
  //mrb_define_method(mrb, _class_v8, "add_func", mrb_v8_add_func, ARGS_REQ(1));
  ARENA_RESTORE;

  v8_init(&_v8wrap_callback);
}

/* vim:set et ts=2 sts=2 sw=2 tw=0: */
