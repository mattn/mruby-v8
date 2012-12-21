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
static mrb_value functable;
static mrb_state* last_mrb = NULL;

typedef struct {
  void* v8context;
  mrb_value instance;
  mrb_state* mrb;
} mrb_v8context;

static mrb_value
json_parse(mrb_state* mrb, const char* ptr) {
  mrb_value val;
  if (*ptr) {
    mrb_value str = mrb_str_new_cstr(mrb, "{\"value\":");
    mrb_str_cat2(mrb, str, ptr);
    mrb_str_cat2(mrb, str, "}");
    struct RClass* clazz = mrb_class_get(mrb, "JSON");
    mrb_value args[1];
    args[0] = str;
    mrb_value hash = mrb_funcall_argv(mrb, mrb_obj_value(clazz), mrb_intern(mrb, "parse"), 1, args);
    val = mrb_hash_get(mrb, hash, mrb_str_new_cstr(mrb, "value"));
  } else {
    val = mrb_nil_value();
  }
  return val;
}

static char*
stringify_json(mrb_state* mrb, mrb_value val) {
  struct RClass* clazz = mrb_class_get(mrb, "JSON");
  mrb_value args[1];
  args[0] = val;
  val = mrb_funcall_argv(mrb, mrb_obj_value(clazz), mrb_intern(mrb, "stringify"), 1, args);
  return strdup(RSTRING_PTR(val));
}

char*
_v8wrap_callback(char* id, char* name, char* arguments) {
  mrb_state* mrb = last_mrb;
  mrb_value funcs = mrb_hash_get(mrb, functable, mrb_str_new_cstr(mrb, id));
  if (mrb_nil_p(funcs)) {
    return strdup("null");
  }
  mrb_value proc = mrb_hash_get(mrb, funcs, mrb_str_new_cstr(mrb, name));
  mrb_value args = json_parse(mrb, arguments);
  mrb_value val = mrb_yield_argv(mrb, proc, RARRAY_LEN(args), RARRAY_PTR(args));
  return stringify_json(mrb, val);
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
  mrb_hash_set(mrb, functable, mrb_funcall(mrb, self, "inspect", 0, NULL), mrb_hash_new(mrb));
  return self;
}


static mrb_value
_v8_exec(mrb_state *mrb, void* v8context, mrb_value str)
{
  last_mrb = mrb;
  char* ret = v8_execute(v8context, RSTRING_PTR(str));
  mrb_value val = mrb_nil_value();
  if (!ret) {
    mrb_raise(mrb, E_RUNTIME_ERROR, v8_error(v8context));
  }
  val = json_parse(mrb, ret);
  free(ret);
  return val;
}

static mrb_value
mrb_v8_eval(mrb_state *mrb, mrb_value self)
{
  mrb_value value_context;
  mrb_v8context* context = NULL;
  mrb_value expr;

  mrb_get_args(mrb, "S", &expr);

  value_context = mrb_iv_get(mrb, self, mrb_intern(mrb, "context"));
  Data_Get_Struct(mrb, value_context, &v8context_type, context);
  if (!context) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }

  return _v8_exec(mrb, context->v8context, expr);
}

static mrb_value
mrb_v8_add_func(mrb_state *mrb, mrb_value self)
{
  mrb_value value_context;
  mrb_v8context* context = NULL;
  mrb_value name, func;

  mrb_get_args(mrb, "S&", &name, &func);

  value_context = mrb_iv_get(mrb, self, mrb_intern(mrb, "context"));
  Data_Get_Struct(mrb, value_context, &v8context_type, context);
  if (!context) {
    mrb_raise(mrb, E_ARGUMENT_ERROR, "invalid argument");
  }

  mrb_value id = mrb_funcall(mrb, self, "inspect", 0, NULL);
  mrb_value funcs = mrb_hash_get(mrb, functable, id);
  mrb_hash_set(mrb, funcs, name, func);
  mrb_value str = mrb_str_new_cstr(mrb, "(function(self) { self.");
  mrb_str_concat(mrb, str, name);
  mrb_str_cat2(mrb, str, " = function() { return _mrb_v8_call(");
  mrb_str_concat(mrb, str, mrb_funcall(mrb, id, "inspect", 0, NULL));
  mrb_str_cat2(mrb, str, ", ");
  mrb_str_concat(mrb, str, mrb_funcall(mrb, name, "inspect", 0, NULL));
  mrb_str_cat2(mrb, str, ", JSON.stringify([].slice.call(arguments))); }})(this)"),
  _v8_exec(mrb, context->v8context, str);

  return mrb_nil_value();
}

/*********************************************************
 * register
 *********************************************************/

void
mrb_mruby_v8_gem_init(mrb_state* mrb) {
  ARENA_SAVE;

  _class_v8 = mrb_define_class(mrb, "V8", mrb->object_class);
  mrb_define_method(mrb, _class_v8, "initialize", mrb_v8_init, ARGS_NONE());
  mrb_define_method(mrb, _class_v8, "eval", mrb_v8_eval, ARGS_ANY());
  mrb_define_method(mrb, _class_v8, "add_func", mrb_v8_add_func, ARGS_REQ(1));
  ARENA_RESTORE;

  v8_init(&_v8wrap_callback);

  functable = mrb_hash_new(mrb);
  mrb_define_const(mrb, _class_v8, "$FUNCTABLE", functable);
}

/* vim:set et ts=2 sts=2 sw=2 tw=0: */
