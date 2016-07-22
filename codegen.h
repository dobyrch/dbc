extern int lex_line;
extern int lex_column;

void compile(struct node *ast);
void free_tree(struct node *ast);

/*
 * It'd be awfully repetitive declare every code generation function
 * with its full signature, so they're all declared in this big ugly
 * block instead.
 */
codegen_func
	gen_add_assign, gen_addr, gen_and, gen_and, gen_add,
	gen_and_assign, gen_args, gen_assign, gen_auto, gen_call,
	gen_case, gen_comma, gen_compound, gen_cond, gen_const, gen_defs,
	gen_div, gen_div_assign, gen_eq, gen_eq_assign, gen_expression,
	gen_extrn, gen_funcdef, gen_ge, gen_goto, gen_gt, gen_if,
	gen_index, gen_indir, gen_init, gen_inits, gen_ior, gen_ivals,
	gen_label, gen_le, gen_left, gen_left_assign, gen_lt, gen_mod,
	gen_mod_assign, gen_mul, gen_mul_assign, gen_name, gen_names,
	gen_ne, gen_ne_assign, gen_neg, gen_not, gen_or_assign,
	gen_postdec, gen_postinc, gen_predec, gen_preinc, gen_return,
	gen_right, gen_right_assign, gen_simpledef, gen_statements,
	gen_sub, gen_sub_assign, gen_switch, gen_switch, gen_vecdef,
	gen_while, gen_xor, gen_xor_assign;
