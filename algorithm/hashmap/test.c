#include "hash.h"

#define MAX_LEN 2048

int main(int argc, char *argv[])
{
	char *file = argv[1];

	struct hash *hash = init_hash(file);

	char key1[] = "北京", value1[] = "010";
	char key2[] = "上海", value2[] = "021";
	char key3[] = "广州", value3[] = "020";
	char key4[] = "深圳", value4[] = "0755";

	hash_insert(hash, key1, value1);
	hash_insert(hash, key2, value2);
	hash_insert(hash, key3, value3);

	int id1 = get_index(hash, key1);
	printf("%s, %s\n", key1, hash->values[id1]);
	int id2 = get_index(hash, key2);
	printf("%s, %s\n", key2, hash->values[id2]);
	int id3 = get_index(hash, key3);
	printf("%s, %s\n", key3, hash->values[id3]);

	release_hash(hash);
	return 0;
}
