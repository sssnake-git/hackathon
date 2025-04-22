#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_SIZE 1024

struct hash {
	char **keys;
	char **values;
	int *index_hash;
	int word_num;
	int hash_size;
};

struct hash *init_hash();
void hash_insert(struct hash *hash, const char *key, const char *value);
int get_index(struct hash *hash, char *word);
void release_hash(struct hash *hash);
