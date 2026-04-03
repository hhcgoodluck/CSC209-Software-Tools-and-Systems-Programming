#ifndef __FSTREE_H__
#define __FSTREE_H__

struct subdir {
	struct fsnode *fptr;
	struct subdir *next;
};

typedef struct subdir Link;

struct fsnode {
	char *name;
	Link *links;
};

typedef struct fsnode Fstree;

#define MAX_PATH 256
#define MAX_LINE 1024


Fstree *create_node(char *name);
void add_node(Fstree *f, char *path, char *name);
void add_hard_link(Fstree *f, char *dest_path, char *src_path, char *name);
void remove_node(Fstree *f, char *path, char *name);
void print_tree(Fstree *f, int depth);

#endif // __FSTREE_H__
