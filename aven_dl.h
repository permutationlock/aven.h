#ifndef AVEN_DL_H
#define AVEN_DL_H

void *aven_dl_open(const char *fname);
void *aven_dl_sym(void *handle, const char *symbol);
int aven_dl_close(void *handle);

#endif // AVEN_DL_H
