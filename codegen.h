extern int yylineno;

void compile(struct node *ast, const char *outfile);
void free_tree(struct node *ast);

/*
 * It'd be awfully repetitive declare every code generation function
 * with its full signature, so they're all declared in this big ugly
 * block instead.
 */
codegen_func
	gen_add, gen_add_assign, gen_addr, gen_and, gen_and_assign,
	gen_args, gen_assign, gen_auto, gen_call, gen_case,
	gen_compound, gen_cond, gen_const, gen_defs, gen_div,
	gen_div_assign, gen_eq, gen_eq_assign, gen_extrn,
	gen_funcdef, gen_ge, gen_ge_assign, gen_goto, gen_gt,
	gen_gt_assign, gen_if, gen_index, gen_indir, gen_init,
	gen_inits, gen_ivals, gen_label, gen_le, gen_le_assign,
	gen_left, gen_left_assign, gen_lt, gen_lt_assign, gen_mod,
	gen_mod_assign, gen_mul, gen_mul_assign, gen_name, gen_names,
	gen_ne, gen_ne_assign, gen_neg, gen_not, gen_null, gen_or,
	gen_or_assign, gen_postdec, gen_postinc, gen_predec,
	gen_preinc, gen_return, gen_right, gen_right_assign,
	gen_simpledef, gen_statements, gen_sub, gen_sub_assign,
	gen_switch, gen_vecdef, gen_while;
