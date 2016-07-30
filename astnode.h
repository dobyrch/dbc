typedef LLVMValueRef (codegen_func)(struct node *);

struct node {
	codegen_func *codegen;
	char *val;
	struct node *one, *two, *three;
};

LLVMValueRef codegen(struct node *);

struct node *leafnode(codegen_func, char *value);
struct node *node0(codegen_func);
struct node *node1(codegen_func, struct node *one);
struct node *node2(codegen_func, struct node *one, struct node *two);
struct node *node3(codegen_func, struct node *one, struct node *two, struct node *three);
