#ifndef WALK_H
#define WALK_H

#define _XOPEN_SOURCE 500

int walk_directory_tree(const char *);

int iterate_file_list(void* input_file);

#endif
