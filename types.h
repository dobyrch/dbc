typedef union {
	char *val;
	struct node *ast;
} YYSTYPE;

struct node {
	/* TODO: figure out why LLVMValueRef doesn't work */
	/* LLVMValueRef (*codegen)(struct node *); */
	struct LLVMOpaqueValue *(*codegen)(struct node *);
	YYSTYPE one, two, three;
};
