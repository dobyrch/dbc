struct node {
	LLVMValueRef (*codegen)(struct node *);
	YYSTYPE one, two, three;
};

LLVMValueRef codegen(struct node *ast);
char *leafval(struct node *ast);
struct node *one(struct node *ast);
struct node *two(struct node *ast);
struct node *three(struct node *ast);

struct node *leaf(LLVMValueRef (*)(struct node *), char *value);
struct node *node0(LLVMValueRef (*)(struct node *));
struct node *node1(LLVMValueRef (*)(struct node *), struct node *one);
struct node *node2(LLVMValueRef (*)(struct node *), struct node *one, struct node *two);
struct node *node3(LLVMValueRef (*)(struct node *), struct node *one, struct node *two, struct node *three);
