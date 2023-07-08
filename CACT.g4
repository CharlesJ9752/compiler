grammar CACT;

@header {
    #include <vector>
    #include <string>
}

INT: 'int';
BOOL: 'bool';
DOUBLE: 'double';
FLOAT: 'float';
WHILE: 'while';
BREAK: 'break';
CONTINUE: 'continue';
IF: 'if';
ELSE: 'else';
RETURN: 'return';
VOID: 'void';
CONST: 'const';
BoolConst: TRUE | FALSE;
TRUE: 'true';
FALSE: 'false';

ADD: '+';
SUB: '-';
MUL: '*';
DIV: '/';
MOD: '%';	

NOT: '!';

NOTEQUAL: '!=';
EQUAL:'==';
GT:'>';
LT:'<';
GE:'>=';
LE:'<=';

AND: '&&';
OR: '||';

/****** parser ******/
compUnit
    : (outsideDecl | funcDef) * EOF //+ or *? 
    ; 

outsideDecl
    : decl;

decl 
    locals [int where]
    : constDecl
    | varDecl
    ;

constDecl
    : CONST bType constDef (',' constDef)* ';'
    ;

bType
    : INT
    | BOOL
    | FLOAT
    | DOUBLE
    ;

constDef
    : Ident '=' constExp										        # constDefVal
	//| Ident ('[' IntConst ']')+ '=' '{' (constExp ( ',' constExp)*)? '}'	# constDefArray
    | Ident ('[' IntConst ']')+ '=' '{' (constExp (',' constExp)*)? '}'	# constDefArray
    ;
//lzr feat: 更改为非终结符 “初始值” ，以应对多维数组，上面的语句要进行修改
//change: 这个地方需要改：constDefArray，需要constInitVal这里产生维度信息，与左边的信息进行比较

// constInitVal
//     : constExp                                              # constInitValVal
//     | '{' (constInitVal (',' constInitVal)*)? '}'           # constInitArray
//     ;
constInitVal
    locals [ 
        int where               //这个声明是在函数内还是在函数外……绝了，区分对待捏
    ]
    : '{' (constExp (',' constExp)*)? '}'
    ;

varDecl
    locals [ 
        int where               //这个声明是在函数内还是在函数外……绝了，区分对待捏
    ]
    : bType varDef (',' varDef)* ';'
    ;

varDef
    locals [ 
        int where               //这个声明是在函数内还是在函数外……绝了，区分对待捏
    ]
    : Ident																# defVal
	| Ident ('[' IntConst ']')+											# defArray
	| Ident '=' constExp											    # defInitVal
	| Ident ('[' IntConst ']')+ '=' '{' (constExp (',' constExp)*)? '}'	# defInitArray
    //lzr feat: 改成初始值，他妈的，真他妈服了
    ;
//lzr feat: 加上+，表示数组可以是多维度的，这样改动之后，IntConst返回结果成为一个数组，就是有一大堆
//lzr feat: 可以通过size来看等号左边数组的维度，然后这个值得作为一个属性，传递一下？



funcDef
    locals [
        int stack_size,   //开栈帧多大的
    ]
    : ret = funcType name = Ident '(' (funcFParam (',' funcFParam)*)? ')' block
    ;


funcType
    : VOID
    | INT
    | FLOAT
    | DOUBLE
    | BOOL
    ;

funcFParams
    : funcFParam (',' funcFParam)*
    ;

funcFParam         
    : bType Ident ('[' (IntConst)?']'  ('[' IntConst']')* )?
    ;

block 
    locals[ 
        bool return_legal,
        int LA,
        int LB
    ]
    : '{' ( blockItem)* '}'
    ;

blockItem
    locals[ bool return_legal,
        int LA,
        int LB
     ]
    : decl
    | stmt
    ;


stmt
    locals[ 
        bool return_legal,
        int LA,
        int LB
    ]
    : lVal '=' exp ';'                          # assignStmt
    | (exp)? ';'                                # exprStmt
    | block                                     # blockStmt
    | RETURN (exp)? ';'                       # returnStmt
    | IF '(' ifCond ')' stmt                  # ifStmt
    | IF '(' ifCond ')' stmt elseStmt         # ifElseStmt
    | WHILE '(' whileCond ')' stmt            # whileStmt
    | BREAK ';'                               # breakStmt
    | CONTINUE ';'                            # continueStmt
    ;

elseStmt
    locals[ bool return_legal,
        int LA,
        int LB
    ]
    :ELSE stmt
    ;

exp
    locals[
	    std::string name,
	    int cls
	]
    : addExp
    | BoolConst
    ;

constExp
    locals[
        std::string name,
	    int cls
    ]
    : '-'? number                # constExpNumber
    | BoolConst             # constExpBoolConst
    ;

ifCond locals[
        std::string name,
        int cls,
        bool value,
        int LA,
        int LB
	]
    : cond
    ;

whileCond locals[
	std::string name,
	int cls,
	bool value,
    int LA,
    int LB
	]
    : cond
    ;

cond
    locals[
	    std::string name,
	    int cls,
	    bool value,
        int LA,
        int LB
	]
    : lOrExp
    ;

lVal
    locals[
	    std::string name,
	    int cls,
	    bool elemwise,
        int left_or_right = 0  //0表示在右边，1表示在左边
	]
    : Ident ('[' exp ']')*
    ;

primaryExp
    locals[
	    std::string name,
	    int cls,
	    bool elemwise
	]
    : '(' exp ')'
    | lVal
    | number
    | BoolConst
    ;

number
    locals[
        std::string name,
	    int cls
    ]
    : IntConst
    | DoubleConst
    | FloatConst
    ;

unaryExp
    locals[
	    std::string name,
	    int cls,
	    bool elemwise
	]
    : primaryExp                        # primary
    | op=(ADD | SUB | NOT) unaryExp        # unary
    | Ident '(' (funcRParams)? ')'      # funcall
    ;

funcRParams
    : exp (',' exp)*
    ;

mulExp
    locals[
	    std::string name,
	    int cls,
	    bool elemwise
	]
    : unaryExp
    | mulExp op=MUL unaryExp
    | mulExp op=DIV unaryExp
    | mulExp op=MOD unaryExp
    ;

addExp
    locals[
	    std::string name,
	    int cls,
	    bool elemwise
	]
    : mulExp
    | addExp op=ADD mulExp
    | addExp op=SUB mulExp
    ;

relExp
    locals[
	    std::string name,
	    int cls,
	    bool value,
        int LA,
        int LB
	]
    : addExp
    | relExp op=LE addExp
    | relExp op=GE addExp
    | relExp op=LT addExp
    | relExp op=GT addExp
    | boolConst
    ;

boolConst
    locals[
	    std::string name,
	    int cls,
	    bool value
	]
    : BoolConst
    ;

eqExp
    locals[
	    std::string name,
	    int cls,
	    bool value,
        int LA,
        int LB
	]
    : relExp
    | eqExp op=EQUAL relExp
    | eqExp op=NOTEQUAL relExp
    ;

lAndExp
    locals[
	    std::string name,
	    int cls,
	    bool value,
        int LA,
        int LB
	]
    : eqExp
    | lAndExp op=AND eqExp
    ;

lOrExp
    locals[
	    std::string name,
	    int cls,
	    bool value,
        int LA,
        int LB
	]
    : lAndExp
    | lOrExp op=OR lAndExp
    ;

/****** lexer  ******/
//--------------------
//BoolConst
//    : true
//    | false
//    ;

//fragment
Ident
    : IdentNondigit [a-zA-Z_0-9]*   //matches zero or more letters (uppercase or lowercase), digits, or underscores.
    ;

fragment
IdentNondigit   //is a fragment that defines the first character of an identifier. It can be any letter (uppercase or lowercase) or an underscore.
    : [a-zA-Z_]
    ;






//----------------

fragment
Digit   //先匹配
    :[0-9]
    ;

IntConst    //整形常量
    : DecimalConst 
    | OctalConst
    | HexadecimalConst
    ;

fragment
DecimalConst
    : '0'
    | NonzeroDigit Digit*
    ;

fragment
OctalConst
    : '0' OctalDigit+
    ;

fragment
HexadecimalConst
    : HexadecimalPrefix HexadecimalDigit+
    ;

fragment
NonzeroDigit
    : [1-9]
    ;

fragment
OctalDigit
    : [0-7]
    ;

fragment
HexadecimalPrefix
    : '0x'
    | '0X'
    ;

fragment
HexadecimalDigit
    : [0-9a-fA-F]
    ;


//----------------
FloatConst      //先float自带f,否则默认double。float:4byte,double:8byte
    : DoubleConst ('F' | 'f')
    ;

DoubleConst
    : Sign? DecimalFraction Exponent?  //不带exp的是经典形式，带上exp的是带小数点的指数形式
    | Sign? DigitSequence Exponent     //不带小数点的指数形式
    ;                                 //注意正负号不能丢！

fragment
DecimalFraction
    : DigitSequence? '.' DigitSequence
    | DigitSequence '.'
    ;

fragment
Exponent
    : ExponentPrefix Sign? DigitSequence
    ;

fragment
ExponentPrefix
    : 'E'
    | 'e'
    ;

fragment
Sign
    : ADD
    | SUB
    ;

fragment
DigitSequence
    : Digit+
    ; 


//cici:4/6 TBC



/****** skips  ******/ 
NewLine
    : ('\r' '\n'? | '\n') -> skip
    ;

WhiteSpace
    : [ \t]+ -> skip
    ;

LineComment
    : '//' ~[\r\n]* -> skip
    ;

BlockComment
    : '/*' .*? '*/' -> skip
    ;
    
 




