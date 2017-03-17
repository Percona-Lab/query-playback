#ifndef PERCONA_PLAYBACK_TEST_UTIL_H
#define PERCONA_PLAYBACK_TEST_UTIL_H

#include <iostream>
#include <cstdio>
#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

void fprintf_curr_working_dir() {
  #ifdef __APPLE__
      char * dir = (char *) malloc(PATH_MAX * sizeof(char));
      getcwd(dir, PATH_MAX);
      fprintf(stderr, "Working dir: %s\n\n", dir);
      free(dir);
  #else
      fprintf(stderr, "Working dir: %s\n\n", get_current_dir_name()); 
  #endif
}

#ifdef __cplusplus
}
#endif

#endif /* PERCONA_PLAYBACK_TEST_UTIL_H */
