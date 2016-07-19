
int yylex(void);
void yyerror(const char *msg);
char *yytext;
int column;

void compinit(void);
void compile(struct node *ast);
void free_tree(struct node *ast);


struct node *node0(int type);
struct node *node1(int type, struct node *one);
struct node *node2(int type, struct node *one, struct node *two);
struct node *node3(int type, struct node *one, struct node *two, struct node *three);
struct node *leaf(int type, char *value);
