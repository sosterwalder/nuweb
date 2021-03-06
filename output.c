#include "global.h"

/*
 * Function with behaviour like `mkdir -p'
 * http://niallohiggins.com/2009/01/08/mkpath-mkdir-p-alike-in-c-for-unix/
 *
 */
int mkpath(const char *s, mode_t mode) {
    char *q, *r = NULL, *path = NULL, *up = NULL;
    int rv;

    rv = -1;
    if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0)
        return (0);

    if ((path = strdup(s)) == NULL)
        exit(1);

    if ((q = strdup(s)) == NULL)
        exit(1);

    if ((r = dirname(q)) == NULL)
        goto out;

    if ((up = strdup(r)) == NULL)
        exit(1);

    if ((mkpath(up, mode) == -1) && (errno != EEXIST))
        goto out;

    if ((mkdir(path, mode) == -1) && (errno != EEXIST))
        rv = -1;
    else
        rv = 0;

out:
    if (up != NULL) {
        free(up);
    }
    free(q);
    free(path);
    return (rv);
}

void write_files(files)
     Name *files;
{
  while (files) {
    write_files(files->llink);
    /* Write out \verb|files->spelling| */
    {
      static char temp_name[FILENAME_MAX];
      static char real_name[FILENAME_MAX];
      static char real_dir[FILENAME_MAX];
      static int temp_name_count = 0;
      char indent_chars[MAX_INDENT];
      int temp_file_fd;
      FILE *temp_file;

      /* Find a free temporary file */
      for( temp_name_count = 0; temp_name_count < 10000; temp_name_count++) {
        sprintf(temp_name,"%s%snw%06d", dirpath, path_sep, temp_name_count);
      #ifdef O_EXCL
        if (-1 != (temp_file_fd = open(temp_name, O_CREAT|O_WRONLY|O_EXCL))) {
           temp_file = fdopen(temp_file_fd, "w");
           break;
        }
      #else
        if (0 != (temp_file = fopen(temp_name, "a"))) {
           if ( 0L == ftell(temp_file)) {
              break;
           } else {
              fclose(temp_file);
              temp_file = 0;
           }
        }
      #endif
      }
      if (!temp_file) {
        fprintf(stderr, "%s: can't create %s for a temporary file\n",
                command_name, temp_name);
        exit(-1);
      }

      sprintf(real_name, "%s%s%s", dirpath, path_sep, files->spelling);
      if (verbose_flag) {
          fprintf(stderr, "writing %s [%s]\n", files->spelling, temp_name);
      }
      write_scraps(temp_file, files->spelling, files->defs, 0, indent_chars,
                   files->debug_flag, files->tab_flag, files->indent_flag,
                   files->comment_flag, NULL, NULL, 0, files->spelling);
      fclose(temp_file);

      /* Move the temporary file to the target, if required */
      if (compare_flag) {
          /* Compare the temp file and the old file */
          FILE *old_file = fopen(real_name, "r");
          if (old_file) {
              int x, y;
              temp_file = fopen(temp_name, "r");
              do {
                  x = getc(old_file);
                  y = getc(temp_file);
              } while (x == y && x != EOF);
              fclose(old_file);
              fclose(temp_file);
              if (x == y) {
                  remove(temp_name);
                  fprintf(stdout, "%s: %s already exists, removed temp file %s\n", command_name, real_name, temp_name);
              }
              else {
                  remove(real_name);
                  /* Rename the temporary file to the target */
                  if (0 != rename(temp_name, real_name)) {
                      fprintf(stderr, "%s: can't rename output file to %s (%s, %d)\n",
                              command_name, real_name, strerror(errno), __LINE__);
                  }
                  else {
                      fprintf(stdout, "%s: Renamed %s to %s\n", command_name, temp_name, real_name);
                  }
              }
          }
          /* No comparision */
          else {
              struct stat sb;
              strncpy(real_dir, real_name, strlen(real_name));
              char* real_path = dirname(real_dir);
              DIR* dir = opendir(real_path);

              /* Check if directory exists */
              if (dir) {
                  fprintf(stdout, "Directory %s already exists\n", real_dir);
                  closedir(dir);
              }
              else if (ENOENT == errno) {
                  /* If not, create recursively */
                  mkpath(real_path, 0755);
                  fprintf(stdout, "Directory %s does not exist, creating..\n", real_dir);
              }
              else {
                  fprintf(stderr, "%s: can't create directory %s\n",
                          command_name, real_dir);
                  exit(-1);
              }
              /* Check if real_name is actually a regular file and not a directory */
              if (0 != rename(temp_name, real_name)) {
                  fprintf(stderr, "%s: can't rename output file %s to %s (%s, %d)\n",
                          command_name, temp_name, real_name, strerror(errno), __LINE__);
              }
              else {
                  fprintf(stdout, "%s: Renamed %s to %s\n", command_name, temp_name, real_name);
              }
          }
      }
      else {
          remove(real_name);
          /* Rename the temporary file to the target */
          if (0 != rename(temp_name, real_name)) {
              fprintf(stderr, "%s: can't rename output file to %s (%s, %d)\n",
                      command_name, real_name, strerror(errno), __LINE__);
          }
          else {
              fprintf(stdout, "%s: Renamed %s to %s\n", command_name, temp_name, real_name);
          }
      }
    }
    files = files->rlink;
  }
}
