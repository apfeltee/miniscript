
src=<<__eos__
char *mc_util_readhandle(FILE *hnd, size_t *dlen);
char *mc_util_readfile(const char *filename, size_t *dlen);
char *mc_fsutil_fileread(const char *filename, size_t *flen);
size_t mc_fsutil_filewrite(const char *path, const char *string, size_t stringsize);
size_t mc_util_strlen(const char *str);
char *mc_util_strndup(const char *string, size_t n);
char *mc_util_strdup(const char *string);
bool mc_util_strequal(const char *a, const char *b);
bool mc_util_strnequal(const char *a, const char *b, size_t len);
char *mc_util_canonpath(const char *path);
bool mc_util_pathisabsolute(const char *path);
size_t mc_util_hashdata(const void *ptr, size_t len);
size_t mc_util_hashdouble(NumFloat val);
size_t mc_util_upperpowoftwo(size_t v);
NumFloat mc_util_strtod(const char *str, size_t slen, char **endptr);

void mc_state_gcunmarkall();
void mc_state_gcmarkobjlist(Value *objects, size_t count);
void mc_state_gcmarkobject(Value obj);
void mc_state_gcsweep();
int mc_state_gcshouldsweep();
bool mc_state_gcdisablefor(Value obj);
void mc_state_gcenablefor(Value obj);
bool mc_vm_init(State *state);
void mc_vm_reset(State *state);
void mc_state_makestdclasses(State *state);

__eos__

dt = []
src.strip.split(/;/).each{|l|
  l.strip!
  printf("MC_PROTO %s;\n", l)
}
