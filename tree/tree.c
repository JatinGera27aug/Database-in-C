#include "tree.h"
Tree root = {
    .n = {
        .tag = (TagRoot | TagNode),
        .north = (Node *)&root,
        .west = NULL,
        .east = NULL,
        .path = "/"
    }
};


/*struct s_node{
    Tag tag;
    struct s_node *north;
    struct s_node *west;
    struct s_leaf *east;

    int8 path[256];
    
}; */

void zero(int8 *str, int16 size){
    int8 *p;
    //create a memset,,without using any library functions
    int16 n;
    for(n=0,p=str; n<size ; p++, n++){
        *p=0;
    }
}
Node *create_node(Node *parent,int8 *path){
    Node *n;
    int16 size;
    assert(parent);

    //ALLOCATE THE MEMORY for new node
    size=sizeof(struct s_node);
    n=(Node *)malloc((int)size);
    if (!n) {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }


    zero((int8 *)n ,size);//zero function

    //Links to parent
    parent ->west=n;//left leg of parent
    n->tag=TagNode;
    n->north=parent;


    // Copy path safely
    strncpy((char *)n->path, (char *)path, 255);
    n->path[255] = '\0';

    return n;

};
int main(){
    //printf("%p\n", (void *)&root);
    Node *n, *n2;
    n=create_node(&root.n ,"/Users");
    assert(n);
    n2=create_node(n ,"/Users/login");
    assert(n2);

    printf("%p %p\n", (void *)n, (void *)n2);

    free(n2);
    free(n);
    return 0;
}