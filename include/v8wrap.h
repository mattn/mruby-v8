#ifndef _V8WRAP_H_
#ifdef __cplusplus
extern "C" {
#endif
typedef char* (*v8wrap_callback)(char*, char*, char*);
extern void v8_init(v8wrap_callback callback);
extern void* v8_create();
extern void v8_release(void* ctx);
extern char* v8_execute(void* ctx, const char* str);
extern char* v8_error(void* ctx);
#ifdef __cplusplus
}
#endif
#endif /* _V8WRAP_H_ */
