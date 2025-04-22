#include "hash.h"

char *get_key(struct hash *hash, int index);
char *get_value(struct hash *hash, int index);
void make_hash(struct hash *hash);

char *get_key(struct hash *hash, int index)
{
	return hash->keys[index];
}

char *get_value(struct hash *hash, int index)
{
	return hash->values[index];
}

struct hash *init_hash(char *file)
{
	struct hash *hash = (struct hash *)malloc(sizeof(struct hash));
	hash->word_num = 0;
	hash->hash_size = HASH_SIZE;

	hash->keys = (char **)malloc(sizeof(hash->keys));
	hash->values = (char **)malloc(sizeof(hash->values));

	hash->index_hash = (int *)malloc(sizeof(int) * hash->hash_size);
	for (int i = 0; i < hash->hash_size; i++) {
		hash->index_hash[i] = -1;
	}

	return hash;
}

void make_hash(struct hash *hash)
{
	int i;
	for (i = 0; i < hash->word_num; i++) {
		int j = 1, hash_id = 0, k;
		char *wd = hash->keys[i];
		for (k = strlen(wd) - 1; k >= 0; k--) {
			hash_id = (hash_id << 3) + (int)wd[k];
		}
		do {
			hash_id = (abs(hash_id + (j++)) % hash->hash_size);
		} while (hash->index_hash[hash_id] >= 0);
		hash->index_hash[hash_id] = i;
	}
	return;
}

void hash_insert(struct hash *hash, const char *key, const char *value)
{
	int previous_hash_num = hash->word_num;
	hash->word_num += 1;

	hash->keys = realloc(hash->keys, sizeof(char *) * hash->word_num);
	hash->values = realloc(hash->values, sizeof(char *) * hash->word_num);

	// insert element
	int i = hash->word_num - 1;
	hash->keys[i] = (char *)malloc(strlen(key) + 1);
	memcpy(hash->keys[i], key, strlen(key));
	hash->keys[i][strlen(key)] = '\0';

	hash->values[i] = (char *)malloc(strlen(value) + 1);
	memcpy(hash->values[i], value, strlen(value));
	hash->values[i][strlen(value)] = '\0';

	// remake hash
	i = hash->word_num * 4;
	for (i = previous_hash_num; i < hash->word_num; i++) {
		int j = 1, hash_id = 0, k;
		char *wd = hash->keys[i];
		for (k = strlen(wd) - 1; k >= 0; k--) {
			hash_id = (hash_id << 3) + (int)wd[k];
		}
		do {
			hash_id = (abs(hash_id + (j++)) % hash->hash_size);
		} while (hash->index_hash[hash_id] >= 0);
		hash->index_hash[hash_id] = i;
	}
}

int get_index(struct hash *hash, char *word)
{
	int i, hash_id = 0, j = 1;
	for (int k = strlen(word) - 1; k >= 0; k--) {
		hash_id = (hash_id << 3) + (int)word[k];
	}
	do {
		hash_id = (abs(hash_id + (j++)) % hash->hash_size);
		if ((i = hash->index_hash[hash_id]) < 0)
			return -1;
	} while (strcmp(hash->keys[i], word));
	return i;
}

void release_hash(struct hash *hash)
{
	for (int i = 0; i < hash->word_num; i++) {
		free(hash->keys[i]);
		free(hash->values[i]);
	}
	free(hash->keys);
	free(hash->values);
	if (hash->index_hash) {
		free(hash->index_hash);
	}
	free(hash);
}
