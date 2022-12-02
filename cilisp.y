%{
    #include "cilisp.h"
    #define ylog(r, p) {fprintf(flex_bison_log_file, "BISON: %s ::= %s \n", #r, #p); fflush(flex_bison_log_file);}
    int yylex();
    void yyerror(char*, ...);
%}

%union {
    double dval;
    int ival;
    struct ast_node *astNode;
};

%token <ival> FUNC
%token <dval> INT DOUBLE
%token QUIT EOL EOFT LPAREN RPAREN

%type <astNode> s_expr number f_expr s_expr_section s_expr_list

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
    };

%%

