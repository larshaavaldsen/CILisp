#include "cilisp.h"
#include <math.h>

#define RED             "\033[31m"
#define RESET_COLOR     "\033[0m"


// yyerror:
// Something went so wrong that the whole program should crash.
// You should basically never call this unless an allocation fails.
// (see the "yyerror("Memory allocation failed!")" calls and do the same.
// This is basically printf, but red, with "\nERROR: " prepended, "\n" appended,
// and an "exit(1);" at the end to crash the program.
// It's called "yyerror" instead of "error" so the parser will use it for errors too.
void yyerror(char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 255, format, args);

    printf(RED "\nERROR: %s\nExiting...\n" RESET_COLOR, buffer);
    fflush(stdout);

    va_end (args);
    exit(1);
}

// warning:
// Something went mildly wrong (on the user-input level, probably)
// Let the user know what happened and what you're doing about it.
// Then, move on. No big deal, they can enter more inputs. ¯\_(ツ)_/¯
// You should use this pretty often:
//      too many arguments, let them know and ignore the extra
//      too few arguments, let them know and return NAN
//      invalid arguments, let them know and return NAN
//      many more uses to be added as we progress...
// This is basically printf, but red, and with "\nWARNING: " prepended and "\n" appended.
void warning(char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsnprintf (buffer, 255, format, args);

    printf(RED "WARNING: %s\n" RESET_COLOR, buffer);
    fflush(stdout);

    va_end (args);
}

void setParent(AST_NODE *list, AST_NODE *parent) {
    while(list != NULL) {
        list->parent = parent;
        list = list->next;
    }
}

SYMBOL_TABLE_NODE *findSymbol(char *id, SYMBOL_TABLE_NODE *symbolTable) {
    if(id == NULL) {
        return NULL;
    }
    SYMBOL_TABLE_NODE *current = symbolTable;

    while (current != NULL) {
        if(strcmp(id, symbolTable->id) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

FUNC_TYPE resolveFunc(char *funcName)
{
    // Array of string values for function names.
    // Must be in sync with members of the FUNC_TYPE enum in order for resolveFunc to work.
    // For example, funcNames[NEG_FUNC] should be "neg"
    char *funcNames[] = {
            "neg",
            "abs",
            "add",
            "sub",
            "mult",
            "div",
            "remainder",
            "exp",
            "exp2",
            "pow",
            "log",
            "sqrt",
            "cbrt",
            "hypot",
            "max",
            "min",
            ""
    };
    int i = 0;
    while (funcNames[i][0] != '\0')
    {
        if (strcmp(funcNames[i], funcName) == 0)
        {
            return i;
        }
        i++;
    }
    return CUSTOM_FUNC;
}

AST_NODE *createNumberNode(double value, NUM_TYPE type)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }
    // Populate "node", the AST_NODE * created above with the argument data.
    node->data.number.value = value;
    node->data.number.type = type;
    node->type = NUM_NODE_TYPE;
    // node is a generic AST_NODE, don't forget to specify it is of type NUMBER_NODE
    return node;
}

// Creates a node with id
AST_NODE *createSymbolNode(char *id){
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->type = SYM_NODE_TYPE;
    node->data.symbol.id = malloc(sizeof(id) + 1);
    strcpy(node->data.symbol.id, id);

    return node;
}

AST_NODE *createScopeNode(SYMBOL_TABLE_NODE *symbolTable, AST_NODE *child){
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    node->type = SCOPE_NODE_TYPE;
    node->data.scope.child = child;
    node->symbolTable = symbolTable;
    while(symbolTable != NULL) {
        setParent(symbolTable->value, node);
        symbolTable = symbolTable->next;
    }
    setParent(child, node);


    return node;

}

SYMBOL_TABLE_NODE *createSymbol(char *id, AST_NODE *value) {
    SYMBOL_TABLE_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }

    if(id != NULL) {
        node->id = strdup(id);
        free(id);
    }
    else
        node->id = NULL;
    node->value = value;
    node->next = NULL;

    return node;
}

SYMBOL_TABLE_NODE *addSymbolToTable(SYMBOL_TABLE_NODE *new, SYMBOL_TABLE_NODE *table) {
    if(new != NULL) {
        SYMBOL_TABLE_NODE *old = findSymbol(new->id, table);
        if(old != NULL) {
            warning("Duplicate assignment to symbol");
            free(new->id);
            free(old->value);
            old->value = new->value;
            return table;
        }
        new->next = table;
    }
    return new;
}

AST_NODE *createFunctionNode(FUNC_TYPE func, AST_NODE *opList)
{
    AST_NODE *node;
    size_t nodeSize;

    nodeSize = sizeof(AST_NODE);
    if ((node = calloc(nodeSize, 1)) == NULL)
    {
        yyerror("Memory allocation failed!");
        exit(1);
    }
    // Populate the allocated AST_NODE *node's data
    node->type = FUNC_NODE_TYPE;
    node->data.function.func = func;
    if(opList != NULL) {
        node->data.function.opList = opList;
    }
    node->next = NULL;
    setParent(opList, node);
    return node;
}

AST_NODE *addExpressionToList(AST_NODE *newExpr, AST_NODE *exprList)
{
    newExpr->next = exprList;
    return newExpr;
}

RET_VAL evalNeg(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    if(oplist == NULL) {
        warning("neg called with no operands, NAN returned");
        return NAN_RET_VAL;
    }
    if(oplist->next != NULL) {
        warning("neg called with extra operands");
    }

    num = eval(oplist);

    result.type = num.type;
    result.value = -(num.value);
    return result;
}

RET_VAL evalAdd(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    result.value = 0;

    if(oplist == NULL) {
        warning("add called with no operands, 0 returned");
        return ZERO_RET_VAL;
    }

    while(oplist != NULL) {
        num = eval(oplist);
        result.type = num.type; // wrong type
        result.value += num.value;
        oplist = oplist->next;
    }
    return result;
}

RET_VAL evalAbs(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    if(oplist == NULL) {
        warning("abs called with no operands, NAN returned");
        return NAN_RET_VAL;
    }

    if(oplist->next != NULL) {
        warning("abs called with extra operands");
    }

    num = eval(oplist);

    result.type = num.type;
    result.value = fabs(num.value);

    return result;

}

RET_VAL evalSub(AST_NODE *oplist) {
    RET_VAL result;
    result.value = 0;

    if(oplist == NULL) {
        warning("sub called with no operands, 0 returned");
        return ZERO_RET_VAL;
    } else if(oplist->next == NULL) {
        warning("sub called with 1 operand, NAN returned");
        return NAN_RET_VAL;
    }

    RET_VAL firstOp = eval(oplist);
    RET_VAL secondOP = eval(oplist->next);

    if(oplist->next->next != NULL){
        warning("sub called with too many operands, ignoring extra");
    }

    if(firstOp.type == DOUBLE_TYPE || secondOP.type == DOUBLE_TYPE)
        result.type = DOUBLE_TYPE;
    else
        result.type = INT_TYPE;

    result.value = firstOp.value - secondOP.value;

    return result;
}

RET_VAL evalMult(AST_NODE *oplist){
    RET_VAL result;
    result.value = 0;

    if(oplist == NULL) {
        warning("mult called with no operands, 0 returned");
        return ZERO_RET_VAL;
    } else if(oplist->next == NULL) {
        warning("mult called with 1 operand, NAN returned");
        return NAN_RET_VAL;
    }

    RET_VAL firstOp = eval(oplist);
    RET_VAL secondOP = eval(oplist->next);

    if(oplist->next->next != NULL){
        warning("mult called with too many operands, ignoring extra");
    }

    if(firstOp.type == DOUBLE_TYPE || secondOP.type == DOUBLE_TYPE)
        result.type = DOUBLE_TYPE;
    else
        result.type = INT_TYPE;

    result.value = firstOp.value * secondOP.value;

    return result;
}

RET_VAL evalDiv(AST_NODE *oplist) {
    RET_VAL result;
    result.value = 0;

    if(oplist == NULL) {
        warning("div called with no operands, 0 returned");
        return ZERO_RET_VAL;
    } else if(oplist->next == NULL) {
        warning("div called with 1 operand, NAN returned");
        return NAN_RET_VAL;
    }

    RET_VAL firstOp = eval(oplist);
    RET_VAL secondOP = eval(oplist->next);

    if(oplist->next->next != NULL){
        warning("div called with too many operands, ignoring extra");
    }

    if(firstOp.type == DOUBLE_TYPE || secondOP.type == DOUBLE_TYPE)
        result.type = DOUBLE_TYPE;
    else
        result.type = INT_TYPE;

    result.value = firstOp.value / secondOP.value;

    return result;
}

RET_VAL evalRemainder(AST_NODE *oplist) {
    RET_VAL result;
    result.value = 0;

    if(oplist == NULL) {
        warning("remainder called with no operands, 0 returned");
        return ZERO_RET_VAL;
    } else if(oplist->next == NULL) {
        warning("remainder called with 1 operand, NAN returned");
        return NAN_RET_VAL;
    }

    RET_VAL firstOp = eval(oplist);
    RET_VAL secondOP = eval(oplist->next);

    if(oplist->next->next != NULL){
        warning("remainder called with too many operands, ignoring extra");
    }

    if(firstOp.type == DOUBLE_TYPE || secondOP.type == DOUBLE_TYPE)
        result.type = DOUBLE_TYPE;
    else
        result.type = INT_TYPE;

    result.value = fabs(fmod(firstOp.value, secondOP.value));

    return result;
}

RET_VAL evalExp(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    if(oplist == NULL) {
        warning("exp called with no operands, NAN returned");
        return NAN_RET_VAL;
    }
    if(oplist->next != NULL) {
        warning("exp called with extra operands");
    }

    num = eval(oplist);

    result.type = DOUBLE_TYPE;
    result.value = exp(num.value);
    return result;
}

RET_VAL evalExp2(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    if(oplist == NULL) {
        warning("exp2 called with no operands, NAN returned");
        return NAN_RET_VAL;
    }
    if(oplist->next != NULL) {
        warning("exp2 called with extra operands");
    }

    num = eval(oplist);

    result.type = num.type;
    result.value = exp2(num.value);
    if(result.value < 0){
        result.type = DOUBLE_TYPE;
    }
    return result;
}

RET_VAL evalPow(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    result.value = 0;

    if(oplist == NULL) {
        warning("pow called with no operands, 0 returned");
        return ZERO_RET_VAL;
    } else if(oplist->next == NULL) {
        warning("pow called with 1 operand, NAN returned");
        return NAN_RET_VAL;
    }

    RET_VAL firstOp = eval(oplist);
    RET_VAL secondOP = eval(oplist->next);

    if(oplist->next->next != NULL){
        warning("pow called with too many operands, ignoring extra");
    }

    if(firstOp.type == DOUBLE_TYPE || secondOP.type == DOUBLE_TYPE)
        result.type = DOUBLE_TYPE;
    else
        result.type = INT_TYPE;

    result.value = pow(firstOp.value, secondOP.value);

    return result;
}

RET_VAL evalLog(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    if(oplist == NULL) {
        warning("log called with no operands, NAN returned");
        return NAN_RET_VAL;
    }
    if(oplist->next != NULL) {
        warning("log called with extra operands");
    }

    num = eval(oplist);

    result.type = DOUBLE_TYPE;
    result.value = log(num.value);
    return result;
}

RET_VAL evalSqrt(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    if(oplist == NULL) {
        warning("sqrt called with no operands, NAN returned");
        return NAN_RET_VAL;
    }
    if(oplist->next != NULL) {
        warning("sqrt called with extra operands");
    }

    num = eval(oplist);

    result.type = DOUBLE_TYPE;
    result.value = sqrt(num.value);
    return result;
}

RET_VAL evalCbrt(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    if(oplist == NULL) {
        warning("cbrt called with no operands, NAN returned");
        return NAN_RET_VAL;
    }
    if(oplist->next != NULL) {
        warning("cbrt called with extra operands");
    }

    num = eval(oplist);

    result.type = DOUBLE_TYPE;
    result.value = cbrt(num.value);
    return result;
}

RET_VAL evalHypot(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;
    result.value = 0;

    if(oplist == NULL) {
        warning("hypot called with no operands, 0 returned");
        return ZERO_RET_VAL;
    }

    while(oplist != NULL) {
        num = eval(oplist);
        result.value += pow(num.value, 2);
        oplist = oplist->next;
    }
    result.type = DOUBLE_TYPE;
    result.value = sqrt(result.value);
    return result;
}

RET_VAL evalMin(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;

    if(oplist == NULL) {
        warning("min called with no operands, 0 returned");
        return ZERO_RET_VAL;
    }
    num = eval(oplist);
    result.value = num.value;
    oplist = oplist->next;

    while(oplist != NULL) {
        num = eval(oplist);
        if(result.value > num.value) {
            result.type = num.type;
            result.value = num.value;
        }
        oplist = oplist->next;
    }
    return result;
}

RET_VAL evalMax(AST_NODE *oplist) {
    RET_VAL num;
    RET_VAL result;

    if(oplist == NULL) {
        warning("max called with no operands, 0 returned");
        return ZERO_RET_VAL;
    }
    num = eval(oplist);
    result.value = num.value;
    oplist = oplist->next;

    while(oplist != NULL) {
        num = eval(oplist);
        if(result.value < num.value) {
            result.type = num.type;
            result.value = num.value;
        }
        oplist = oplist->next;
    }
    return result;
}

RET_VAL evalFuncNode(AST_NODE *node)
{
    if (!node)
    {
        yyerror("NULL ast node passed into evalFuncNode!");
        return NAN_RET_VAL; // unreachable but kills a clang-tidy warning
    }

    AST_NODE *oplist = node->data.function.opList;

    switch (node->data.function.func) {
        case NEG_FUNC: return evalNeg(oplist);
        case ADD_FUNC: return evalAdd(oplist);
        case ABS_FUNC: return evalAbs(oplist);
        case SUB_FUNC: return evalSub(oplist);
        case MULT_FUNC: return evalMult(oplist);
        case DIV_FUNC: return evalDiv(oplist);
        case REMAINDER_FUNC: return evalRemainder(oplist);
        case EXP_FUNC: return evalExp(oplist);
        case EXP2_FUNC: return evalExp2(oplist);
        case POW_FUNC: return evalPow(oplist);
        case LOG_FUNC: return evalLog(oplist);
        case SQRT_FUNC: return evalSqrt(oplist);
        case CBRT_FUNC: return evalCbrt(oplist);
        case HYPOT_FUNC: return evalHypot(oplist);
        case MIN_FUNC: return evalMin(oplist);
        case MAX_FUNC: return evalMax(oplist);
        default: return NAN_RET_VAL;
    }

    return NAN_RET_VAL;
}

RET_VAL evalNumNode(AST_NODE *node)
{
    if (!node)
    {
        yyerror("NULL ast node passed into evalNumNode!");
        return NAN_RET_VAL;
    }

    RET_VAL result = NAN_RET_VAL;
    result.value = node->data.number.value;
    result.type = node->data.number.type;

    return result;
}

RET_VAL evalScope(AST_NODE *node) {
    if(!node) {
        warning("null node passed to eval scope");
        return NAN_RET_VAL;
    }
    return eval((node->data.scope.child));
}

RET_VAL evalSymbolNode(AST_NODE *node) {
    if(!node) {
        warning("null node passed to eval scope");
        return NAN_RET_VAL;
    }

    AST_NODE *currScope = node;

    while(currScope != NULL) {
        SYMBOL_TABLE_NODE *table = currScope->symbolTable;
        while(table != NULL) {
            if(strcmp(table->id, node->data.symbol.id) == 0) {
                return eval(table->value);
            }
            table = table->next;
        }
        if(currScope->parent == NULL) {
            printf("parent is null");
        }
        currScope = currScope->parent;
    }
    warning("undefined symbol, nan returned");
    return NAN_RET_VAL;
}

RET_VAL eval(AST_NODE *node)
{
    if (!node)
    {
        yyerror("NULL ast node passed into eval!");
        return NAN_RET_VAL;
    }

    if(node->type == NUM_NODE_TYPE) {
        return evalNumNode(node);
    } else if (node->type == FUNC_NODE_TYPE) {
        return evalFuncNode(node);
    } else if(node->type == SCOPE_NODE_TYPE) {
        return evalScope(node);
    } else if(node->type == SYM_NODE_TYPE) {
        return evalSymbolNode(node);
    }

    return NAN_RET_VAL;
}

// prints the type and value of a RET_VAL
void printRetVal(RET_VAL val)
{
    switch (val.type)
    {
        case INT_TYPE:
            printf("Integer : %.lf\n", val.value);
            break;
        case DOUBLE_TYPE:
            printf("Double : %lf\n", val.value);
            break;
        default:
            printf("No Type : %lf\n", val.value);
            break;
    }
}



void freeNode(AST_NODE *node)
{
    if (!node)
    {
        return;
    }

    // TODO complete the function

    // look through the AST_NODE struct, decide what
    // referenced data should have freeNode called on it
    // (hint: it might be part of an s_expr_list, with more
    // nodes following it in the list)

    // if this node is FUNC_TYPE, it might have some operands
    // to free as well (but this should probably be done in
    // a call to another function, named something like
    // freeFunctionNode)

    // and, finally,

   if (node->type == FUNC_NODE_TYPE) {
        freeNode(node->data.function.opList);
    }

    free(node);
}
