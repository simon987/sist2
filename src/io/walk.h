#ifndef WALK_H
#define WALK_H

#undef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500

int walk_directory_tree(const char *);

int iterate_file_list(void* input_file);

#endif
