#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sha256.h"

#define USERNAME_LEN 50
#define PASSWORD_LEN 50
#define HASH_LEN 65  // 64 for SHA-256 hex + null terminator
#define TABLE_SIZE 100

// Node for linked list in hash table
typedef struct Node {
    char username[USERNAME_LEN];
    char password[HASH_LEN];
    struct Node *next;
} Node;

// Hash table struct
typedef struct {
    Node *table[TABLE_SIZE];
} HashTable;

// Function declarations
unsigned int hash(const char *str);
HashTable *createTable();
void insertUser(HashTable *ht, const char *username, const char *hashedPassword);
Node *findUser(HashTable *ht, const char *username);
void hashPassword(const char *password, char *hashedPassword);
int registerUser(HashTable *ht, const char *username, const char *password, const char *file_name);
int loginUser(HashTable *ht, const char *username, const char *password);
void loadUsersFromCSV(HashTable *ht, const char *file_name);

int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("[ERROR] Usage: %s <action> <username> <password> <user_type>\n", argv[0]);
        return 1;
    }

    const char *action = argv[1];
    const char *username = argv[2];
    const char *password = argv[3];
    const char *user_type = argv[4];

    char file_name[30];
    if (strcmp(user_type, "mentor") == 0) {
        strcpy(file_name, "login/mentors.csv");
    } else if (strcmp(user_type, "mentee") == 0) {
        strcpy(file_name, "login/mentees.csv");
    } else {
        printf("[ERROR] Invalid user type. Use 'mentor' or 'mentee'.\n");
        return 1;
    }

    HashTable *ht = createTable();
    loadUsersFromCSV(ht, file_name);

    if (strcmp(action, "register") == 0) {
        if (registerUser(ht, username, password, file_name)) {
            printf("[SUCCESS] Registration complete!\n");
            return 0;
        } else {
            printf("[ERROR] Registration failed.\n");
            return 1;
        }
    } else if (strcmp(action, "login") == 0) {
        if (loginUser(ht, username, password)) {
            printf("[SUCCESS] Login successful!\n");
            return 0;
        } else {
            printf("[ERROR] Invalid username or password.\n");
            return 1;
        }
    } else {
        printf("[ERROR] Invalid action. Use 'register' or 'login'.\n");
        return 1;
    }
}

unsigned int hash(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++)) hash = ((hash << 5) + hash) + c;
    return hash % TABLE_SIZE;
}

HashTable *createTable() {
    HashTable *ht = (HashTable *)malloc(sizeof(HashTable));
    for (int i = 0; i < TABLE_SIZE; i++) ht->table[i] = NULL;
    return ht;
}

void insertUser(HashTable *ht, const char *username, const char *hashedPassword) {
    unsigned int idx = hash(username);
    Node *newNode = (Node *)malloc(sizeof(Node));
    strcpy(newNode->username, username);
    strcpy(newNode->password, hashedPassword);
    newNode->next = ht->table[idx];
    ht->table[idx] = newNode;
}

Node *findUser(HashTable *ht, const char *username) {
    unsigned int idx = hash(username);
    Node *current = ht->table[idx];
    while (current) {
        if (strcmp(current->username, username) == 0) return current;
        current = current->next;
    }
    return NULL;
}

void hashPassword(const char *password, char *hashedPassword) {
    BYTE hash[32];
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, (const BYTE *)password, strlen(password));
    sha256_final(&ctx, hash);
    for (int i = 0; i < 32; i++) sprintf(hashedPassword + (i * 2), "%02x", hash[i]);
    hashedPassword[64] = '\0';
}

void loadUsersFromCSV(HashTable *ht, const char *file_name) {
    FILE *file = fopen(file_name, "r");
    if (!file) return;

    char line[150], user[USERNAME_LEN], pass[HASH_LEN];
    while (fgets(line, sizeof(line), file)) {
        sscanf(line, "%[^,],%s", user, pass);
        insertUser(ht, user, pass);
    }

    fclose(file);
}

int registerUser(HashTable *ht, const char *username, const char *password, const char *file_name) {
    if (findUser(ht, username)) {
        printf("[ERROR] User already exists.\n");
        return 0;
    }

    char hashedPassword[HASH_LEN];
    hashPassword(password, hashedPassword);
    insertUser(ht, username, hashedPassword);

    FILE *file = fopen(file_name, "a");
    if (!file) {
        printf("[ERROR] Could not open file to write.\n");
        return 0;
    }
    fprintf(file, "%s,%s\n", username, hashedPassword);
    fclose(file);

    return 1;
}

int loginUser(HashTable *ht, const char *username, const char *password) {
    Node *user = findUser(ht, username);
    if (!user) return 0;

    char hashedPassword[HASH_LEN];
    hashPassword(password, hashedPassword);

    return strcmp(user->password, hashedPassword) == 0;
}
