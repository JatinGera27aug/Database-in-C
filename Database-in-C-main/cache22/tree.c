#include "tree.h"

//Nodes working as just connections to different data points
//Leaves stores actual data
Tree root = {
    .n = {
        .tag = (TagRoot | TagNode),
        .north = (Node *)&root,
        .west = NULL,
        .east = NULL,
        .path = "/"
    }
};

/*Creating a set of functions that will print the Tree */
//1.)
void print_tree(int fd,Tree *_root){
    int8 indentation;
    int8 buf[256];
    int16 size;
    Node *n;
    Leaf *l;

    indentation=0;
    for(n=(Node *)_root; n ; n=n->west){
        Print(indent(indentation++));//define in .h file
        Print(n->path);
        Print("\n");
        if(n->east){//check if tree have a leaf
            for(l=n->east ; l ; l=l->east){
                Print(indent(indentation));
                Print(n->path);
                Print("/");
                Print(l->key);
                Print("-->");
                write(fd,(char *)l->value, (int)l->size);
                Print("\n")

            }
        }
    }
    return ;
}

//2.)
int8 *indent(int8 n){
    int16 i;
    static int8 buf[256];
    int8 *p;

    //to check if paramter is if exact required size
    if(n<1){
        return (int8 *)"";
    }
    assert(n <120);
    
    zero(buf,256);
    for(i=0,p=buf;i<n;i++,p+=2){
        strncpy((char *)p," ",2);
    }
    return buf;
}
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
    errno=NoError;
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

}

//Finding last leaf node
Leaf *find_last_linear(Node *parent){
    Leaf *l;
    errno=NoError;
    assert(parent);

    if(!parent->east){
        reterr(NoError);
    }
    for(l=parent->east;l->east;l=l->east);
    assert(l);
    return l;
}


Leaf *create_leaf(Node* parent,int8 *key,int8 *value,int16 count){
    Leaf *l, *new;
    
    int16 size;
    assert(parent);
    l=find_last(parent);

    size=sizeof(struct s_leaf);
    new=(Leaf *)malloc(size);
    assert(new);

    if(!l){
        parent->east=new;
        //directly connected
    }
    else{
        l->east=new;
        //l is the leaf
    }

    zero((int8 *)new,size);
    new->tag=TagLeaf;
    new->west=(!l)?(Tree *)parent : (Tree *)l;

    strncpy((char *)new->key,(char *)key,127);
    new->value=(int8 *)malloc(count);
    zero(new->value,count);
    assert(new->value);
    strncpy((char *)new->value ,(char *)value,count);
    new->size=count;
    return new;

    //Node --leaf1--leaf2--etc
}



int main(){
    //printf("%p\n", (void *)&root);
    Node *n, *n2;
    Leaf *l1,*l2;
    int8 *key, *value;
    int16 size;



    n=create_node(&root.n ,(int8 *)"/Users");
    assert(n);
    n2=create_node(n ,(int8 *)"/Users/login");
    assert(n2);
    key=(int8 *)"aditya";
    value=(int8 *)"abd4501aa";
    size=(int16)strlen((char *)value);
    l1=create_leaf(n2,key,value,size);
    assert(l1);

    //printf("%s\n",l1->value);
    key=(int8 *)"Gaur";
    value=(int8 *)"rohitSh18rank2";
    size=(int16)strlen((char *)value);
    l2=create_leaf(n2,key,value,size);
    assert(l2);
    //printf("%s\n",l2->key);

    //printf("%p %p\n", (void *)n, (void *)n2);

    print_tree(1,&root);

    free(n2);
    free(n);
    return 0;
}

#pragma GCC diagnostic pop;