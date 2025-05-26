#ifndef HAKA_BASE_H
#define HAKA_BASE_H

#define BUFSIZE 1024

#define strCpyCat(dest, src, append) \
  strcpy(dest, src);                 \
  strcat(dest, append)

#endif  // !HAKA_BASE_H
