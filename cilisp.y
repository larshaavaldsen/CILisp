%{
    #include "cilisp.h"
    #define ylog(r, p) {fprintf(flex_bison_log_file, "BISON: %s ::= %s \n", #r, #p); fflush(flex_bison_log_file);}
    int yylex();
    void yyerror(char*, ...);
%}

%union {
    double dval;
    int ival;
    char *sval;
    struct ast_node *astNode;
    struct symbol_table_node *symTNode;
};

%token <ival> FUNC
%token <dval> INT DOUBLE
%token <sval> SYMBOL
%token QUIT EOL EOFT LPAREN RPAREN LET INT_TYPECAST DOUBLE_TYPECAST

%type <astNode> s_expr number f_expr s_expr_section s_expr_list
%type <symTNode> let_section let_elem let_list

%%

program:
    s_expr EOL {
        ylog(program, s_expr EOL);
        if ($1) {
            printRetVal(eval($1));
            freeNode($1);
        }
        YYACCEPT;
    }
    | s_expr EOFT {
        ylog(program, s_expr EOFT);
        if ($1) {
            printRetVal(eval($1));
            freeNode($1);
        }
        exit(EXIT_SUCCESS);
    }
    | EOL {
        ylog(program, EOL);
        YYACCEPT;  // paranoic; main skips blank lines
    }
    | EOFT {
        ylog(program, EOFT);
        exit(EXIT_SUCCESS);
    };

number:
    DOUBLE {
        ylog(number, DOUBLE_TYPE);
        $$ = createNumberNode($1, DOUBLE_TYPE);
        } | INT {
        ylog(number, INT_TYPE);
        $$ = createNumberNode($1, INT_TYPE);

        }

f_expr:
    LPAREN FUNC s_expr_section RPAREN {
        ylog(f_expr, LPAREN FUNC s_expr_section RPAREN);
        $$ = createFunctionNode($2, $3);
    }

s_expr_section:
    s_expr_list {
        ylog(s_expr_section, s_expr_list);
    } | {
        ylog(s_expr_section, empty);
        $$ = NULL;
    }

s_expr_list:
    s_expr {
        ylog(s_expr_list, s_expr);
     } | s_expr s_expr_list {
        ylog(s_expr, s_expr_list);
        $$ = addExpressionToList($1, $2);
     }

s_expr:
    number {
        ylog(s_expr, number);
    } | f_expr {
        ylog(s_expr, f_expr);
    }| QUIT {
        ylog(s_expr, QUIT);
        exit(EXIT_SUCCESS);
    }| error {
        ylog(s_expr, error);
        yyerror("unexpected token");
        $$ = NULL;
    } | SYMBOL {
        ylog(s_expr, SYMBOL);
        $$ = createSymbolNode($1);
    } | LPAREN let_section s_expr RPAREN {
        ylog(s_expr, LPAREN let_section s_expr RPAREN);
        $$ = createScopeNode($2, $3);
    };

    let_section:
        LPAREN LET let_list RPAREN {
            ylog(let_section, LPAREN LET let_list RPAREN);
            $$ = $3;
        };

    let_list:
        let_elem {
            ylog(let_list, let_elem);
            $$ = $1;
        }| let_elem let_list {
            ylog(let_list, let_elem let_list);
            $$ = addSymbolToTable($1, $2);
        };

    let_elem:
        LPAREN SYMBOL s_expr RPAREN {
            ylog(let_elem, LPAREN SYMBOL s_expr RPAREN);
            $$ = createSymbol($2, $3);
        } | LPAREN INT_TYPECAST SYMBOL s_expr RPAREN {
            ylog(let_elem, LPAREN INT_TYPECAST SYMBOL s_expr RPAREN);
            $$ = createTypedSymbol($3, $4, true);
        } | LPAREN DOUBLE_TYPECAST SYMBOL s_expr RPAREN{
            ylog(let_elem, LPAREN DOUBLE_TYPECAST SYMBOL s_expr RPAREN);
            $$ = createTypedSymbol($3, $4, false);
        };

%%

