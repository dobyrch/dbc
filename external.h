
enum ntype {
	N_SIMPLEDEF,
	N_VECDEF,
	N_FUNCDEF,
	N_AUTO,
	N_EXTRN,
	N_LABEL,
	N_CASE,
	N_COMPOUND,
	N_IF,
	N_WHILE,
	N_SWITCH,
	N_GOTO,
	N_RETURN,
	N_COMMA,
	N_ASSIGN,
	N_MUL_ASSIGN,
	N_DIV_ASSIGN,
	N_MOD_ASSIGN,
	N_ADD_ASSIGN,
	N_SUB_ASSIGN,
	N_LEFT_ASSIGN,
	N_RIGHT_ASSIGN,
	N_AND_ASSIGN,
	N_XOR_ASSIGN,
	N_OR_ASSIGN,
	N_EQ_ASSIGN,
	N_NE_ASSIGN,
	N_COND,
	N_IOR,
	N_XOR,
	N_AND,
	N_EQ,
	N_NE,
	N_LT,
	N_GT,
	N_LE,
	N_GE,
	N_LEFT,
	N_RIGHT,
	N_ADD,
	N_SUB,
	N_MUL,
	N_DIV,
	N_MOD,
	N_INDIR,
	N_ADDR,
	N_NEG,
	N_NOT,
	N_INDEX,
	N_CALL,
	N_PREINC,
	N_PREDEC,
	N_POSTINC,
	N_POSTDEC,
	N_NAME,
	N_CONST,
	/*
	* A catch-all for nodes whose types
	* can be deduced from their parents
	*/
	N_DUMMY
};

union YYSTYPE {
	char *val;
	struct node *ast;
	enum ntype typ;
};

typedef union YYSTYPE YYSTYPE;

struct node {
	YYSTYPE one, two, three;
	enum ntype typ;
};

int yylex(void);
void yyerror(const char *msg);
char *yytext;
int column;



void compile(struct node *ast);
void free_tree(struct node *ast);


struct node *node0(int type);
struct node *node1(int type, struct node *one);
struct node *node2(int type, struct node *one, struct node *two);
struct node *node3(int type, struct node *one, struct node *two, struct node *three);
struct node *leaf1(int type, char *value);
struct node *leaf2(int type, char *value, struct node *two);
struct node *leaf3(int type, char *value, struct node *two, struct node *three);