/**
 * Some text process tools by Tianzhe.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

char *to_upper(char *str)
{
	for (unsigned int i = 0; i < strlen(str); i++) {
		if ((unsigned char)str[i] >= 0x80) {
			i++;
			continue;
		}
		str[i] = toupper(str[i]);
	}
	return str;
}

char *to_lower(char *str)
{
	for (unsigned int i = 0; i < strlen(str); i++) {
		if ((unsigned char)str[i] >= 0x80) {
			i++;
			continue;
		}
		str[i] = tolower(str[i]);
	}
	return str;
}

char *str_tok(char *str, char *delim, char *tokens)
{
	while (*str) {
		if (strchr(delim, *str)) {
			str++;
			break;
		}
		*(tokens++) = (*(str++));
	}
	*tokens = '\0';
	return str;
}

char **split_utf8(char *s, int *size)
{
	if (strlen(s) <= 0) {
		return NULL;
	}

	int *position_list = (int *)malloc(sizeof(int));

	int i = 0;
	*size = 0;
	int f_len = strlen(s);
	while (i < f_len) {
		if (s[i] >> 7 & 1 && s[i + 1] >> 7 & 1) {
			// chinese character
			i = i + 3;
			*size += 1;
			position_list = (int *)realloc(position_list, sizeof(int) * *size);
			position_list[*size - 1] = i - 3;
		} else {
			// other utf-8 character
			i = i + 1;
			*size += 1;
			position_list = (int *)realloc(position_list, sizeof(int) * *size);
			position_list[*size - 1] = i - 1;
		}
	}

	char **result = (char **)malloc(sizeof(char *) * *size);
	int offset = 0;
	for (i = 0; i < *size; i++) {
		if (is_chinese(s + offset)) {
			result[i] = (char *)malloc(sizeof(char) * 3 + 1);
			memcpy(result[i], s + position_list[i], sizeof(char) * 3);
			result[i][sizeof(char) * 3] = '\0';
			offset += 3;
			// printf("CHN: %s, %d, %s\n", result[i], i, s + i);
		} else {
			result[i] = (char *)malloc(sizeof(char) + 1);
			memcpy(result[i], s + position_list[i], sizeof(char));
			result[i][sizeof(char)] = '\0';
			offset += 1;
			// printf("ENG: %s\n", result[i]);
		}
	}
	free(position_list);
	return result;
}

int is_chinese(char *word)
{
	char c = word[0];
	if (c & 0x80) {
		return 1;
	}
	return 0;
}

char **forward_split(char **str, int size, struct hash *hash, int *result_size)
{
	char **result = (char **)malloc(sizeof(char *));
	*result_size = 0;
	int max_length = size;
	int index = 0;
	while (index < size) {
		int word_length = size - index >= max_length ? max_length : size - index;
		while (word_length >= 1) {
			char *word = sub_word(str, index, word_length);
			// printf("%s\n", word);
			int hash_value = get_index(hash, word);
			char *word_dup = strdup(word);
			if (hash_value != -1 || get_index(hash, to_upper(word_dup)) != -1) {
				index += word_length;
				*result_size += 1;
				result = (char **)realloc(result, sizeof(char *) * *result_size);
				result[*result_size - 1] =
					(char *)malloc(sizeof(char) * strlen(word) + 1);
				memcpy(result[*result_size - 1], word, strlen(word) + 1);
				free(word);
				break;
			} else if (word_length == 1 && hash_value == -1) {
				index += word_length;
				*result_size += 1;
				result = (char **)realloc(result, sizeof(char *) * *result_size);
				result[*result_size - 1] =
					(char *)malloc(sizeof(char) * strlen(word) + 1);
				memcpy(result[*result_size - 1], word, strlen(word) + 1);
			}
			word_length--;
			free(word);
			free(word_dup);
		}
	}
	return result;
}

char *sub_word(char **str, int start, int len)
{
	char *result = (char *)malloc(len * 3 + 1);
	memset(result, 0, (len * 3));
	for (int i = start; i < start + len; i++) {
		result = strcat(result, str[i]);
	}
	result[len * 3] = '\0';
	return result;
}

// c++ code
int32_t StringLength(const string &str, vector<string> &elems)
{
	// Split sentence
	string strChar;
	vector<string> words;
	int32 i = 0;
	while (i < str.size()) {
		char chr = str[i];
		// chr is '\0xxx xxxx', ascii code
		if ((chr & 0x80) == 0) {
			strChar = str.substr(i, 1);
			words.push_back(strChar);
			++i;
		}
		// chr is '\1111 1xxx'
		else if ((chr & 0xF8) == 0xF8) {
			strChar = str.substr(i, 5);
			words.push_back(strChar);
			i += 5;
		}
		// chr is '\1111 xxxx'
		else if ((chr & 0xF0) == 0xF0) {
			strChar = str.substr(i, 4);
			words.push_back(strChar);
			i += 4;
		}
		// chr is '\111x xxxx'
		else if ((chr & 0xE0) == 0xE0) {
			strChar = str.substr(i, 3);
			words.push_back(strChar);
			i += 3;
		}
		// chr is '\11xx xxxx'
		else if ((chr & 0xC0) == 0xC0) {
			strChar = str.substr(i, 2);
			words.push_back(strChar);
			i += 2;
		}
	}
	// Format vector
	i = 0;
	strChar = "";
	while (i < words.size()) {
		if (words[i] == " ") {
			i++;
		} else if (words[i].size() > 1) {
			elems.push_back(words[i]);
			i++;
		} else {
			strChar.append(words[i]);
			if (i + 1 < words.size() && words[i + 1].size() > 1) {
				elems.push_back(strChar);
				strChar = "";
			} else if (i + 1 < words.size() && words[i + 1] == " ") {
				elems.push_back(strChar);
				strChar = "";
			} else if (i + 1 >= words.size()) {
				elems.push_back(strChar);
				strChar = "";
			}
			i++;
		}
	}
	// Get length
	return elems.size();
}
