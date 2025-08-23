// hash_table.h
#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "cache22.h"

char *my_strdup(const char *s);
unsigned int hash(char *key, int table_size);
HashTable *create_hash_table(int size);
int insert_key_value(HashTable *table, char *key, char *value, int ttl); // latest ttl change
char *get_key_value(HashTable *table, char *key);
int delete_key_value(HashTable *table, char *key);
void destroy_hash_table(HashTable *table); //new

#endif
