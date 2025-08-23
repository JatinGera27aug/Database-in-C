#include "cache22.h"
#include "hash_table.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>   // added

char *my_strdup(const char *s) {
    char *d = malloc(strlen(s) + 1);
    if (d == NULL) return NULL;
    strcpy(d, s);
    return d;
}

// Hash function (simple modulo)
unsigned int hash(char *key, int table_size) {
    unsigned int hashval = 0;
    for (; *key != '\0'; key++)
        hashval = *key + (hashval << 5) - hashval;
    return hashval % table_size;
}

// Create a new hash table
HashTable *create_hash_table(int size) {
    HashTable *table = (HashTable *)malloc(sizeof(HashTable));
    if (table == NULL) {
        return NULL;
    }

    table->table = (HashEntry **)malloc(sizeof(HashEntry *) * size);
    if (table->table == NULL) {
        free(table);
        return NULL;
    }

    for (int i = 0; i < size; i++) {
        table->table[i] = NULL;
    }

    table->size = size;
    return table;
}

// Insert a key-value pair into the hash table (with ttl seconds)
// ttl <= 0 will use 0 (meaning immediate expiry if 0 or negative) 
int insert_key_value(HashTable *table, char *key, char *value, int ttl) {
    if (!table || !key || !value) return -1;
    unsigned int index = hash(key, table->size);

    // If same key exists, replace value and update expiry
    HashEntry *entry = table->table[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            free(entry->value);
            entry->value = my_strdup(value);
            if (entry->value == NULL) return -1;
            entry->expire = (ttl > 0) ? (time(NULL) + ttl) : 0;
            return 0;
        }
        entry = entry->next;
    }

    // create new entry
    entry = (HashEntry *)malloc(sizeof(HashEntry));
    if (entry == NULL) return -1;

    entry->key = my_strdup(key);
    entry->value = my_strdup(value);
    entry->next = NULL;
    entry->expire = (ttl > 0) ? (time(NULL) + ttl) : 0;

    if (table->table[index] == NULL) {
        table->table[index] = entry;
    } else {
        entry->next = table->table[index];
        table->table[index] = entry;
    }

    return 0;
}

// Get the value associated with a key from the hash table (removes expired entries)
// Returns pointer to value (owned by table) or NULL if expired/ ya nhi mili
char *get_key_value(HashTable *table, char *key) {
    if (!table || !key) return NULL;
    unsigned int index = hash(key, table->size);

    HashEntry *entry = table->table[index];
    HashEntry *prev = NULL;
    time_t now = time(NULL);

    while (entry != NULL) {
        if (entry->expire != 0 && entry->expire <= now) {
            // expired -> remove this node
            HashEntry *tofree = entry;
            if (prev) prev->next = entry->next;
            else table->table[index] = entry->next;
            entry = entry->next;
            if (tofree->key) free(tofree->key);
            if (tofree->value) free(tofree->value);
            free(tofree);
            continue;
        }

        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        prev = entry;
        entry = entry->next;
    }

    return NULL; // Key not found
}

// --- new: free a hash table and all entries ---
void destroy_hash_table(HashTable *table) {
    if (table == NULL) return;
    for (int i = 0; i < table->size; i++) {
        HashEntry *e = table->table[i];
        while (e) {
            HashEntry *next = e->next;
            if (e->key) free(e->key);
            if (e->value) free(e->value);
            free(e);
            e = next;
        }
    }
    free(table->table);
    free(table);
}

// Delete a single key from the hash table  //new
// returns 1 if deleted, 0 if key not found, -1 on error
int delete_key_value(HashTable *table, char *key) {
    if (table == NULL || key == NULL) return -1;
    unsigned int index = hash(key, table->size);

    HashEntry *prev = NULL;
    HashEntry *e = table->table[index];
    while (e) {
        if (strcmp(e->key, key) == 0) {
            if (prev) prev->next = e->next;
            else table->table[index] = e->next;
            if (e->key) free(e->key);
            if (e->value) free(e->value);
            free(e);
            return 1;
        }
        prev = e;
        e = e->next;
    }
    return 0;
}