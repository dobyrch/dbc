typedef union {
	char *val;
	struct node *ast;
} YYSTYPE;

struct node {
	void * (*codegen)(void *);
	YYSTYPE one, two, three;
};
