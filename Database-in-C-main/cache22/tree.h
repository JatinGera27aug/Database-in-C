#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#pragma GCC diagnostic push 

#define TagRoot     1
#define TagNode     2
#define TagLeaf     4

//Creating our Own Nullptr
#define nullptr NULL


#define find_last(x)      find_last_linear(x)
#define NoError   0



#define reterr(x) \
    errno=(x); \
    return nullptr

#define Print(x) \
        zero(buf,256); \
        strncpy((char *)buf,(char *)(x),255); \
        size=(int16)strlen((char *)buf); \
        if(size) \
            write(fd,(char *)buf,size);
        


//we will focus on the performance rather than the storage used
typedef unsigned int int32;
typedef unsigned short int int16;
typedef unsigned char int8;
typedef unsigned char Tag;



struct s_node{
    Tag tag;
    struct s_node *north;
    struct s_node *west;
    struct s_leaf *east;

    int8 path[256];
    
};

typedef struct s_node Node;//creation of node

struct s_leaf{//creation of leaf
    Tag tag;
    union u_tree *west;
    struct s_leaf *east;
    int8 key[128];
    int8 *value;//pointer bnaya iska,bcz value can be rather large/json format
    int16 size;
};

typedef struct s_leaf Leaf;

union u_tree{
    Node n;
    Leaf l;
};
typedef union u_tree Tree;

int8 *indent(int8);//to print the tree
void print_tree(int,Tree *);
void zero(int8 *,int16);
Node *create_node(Node *,int8 *);
Leaf *find_last_linear(Node *);
Leaf *create_leaf(Node* ,int8 *,int8 *,int16);
int main(void);

