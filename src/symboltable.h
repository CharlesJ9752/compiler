#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <cassert>
#include <string>
#include <stack>
#include <array>
#include <vector>
#include <list>
#include <queue>
#include <set>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <iomanip>

enum
{
	CLASS_INT,
	CLASS_BOOLEAN,
	CLASS_FLOAT,
	CLASS_DOUBLE,
	CLASS_VOID
};//数据/函数返回值类型
enum
{
	TYPE_CONST,         //常量
	TYPE_VAR,           //变量
	TYPE_CONST_ARRAY,   //常量数组
	TYPE_ARRAY,         //数组
	TYPE_PARAM          //函数参数
};

struct Var{
    int cls;                // 变量的基本类型:int bool float double
    int type;               // 是否为const或array 或函数参数
    int length;             // 对于数组变量表示长度，对于函数参数表示第几个。对于常量就是记录值
    int line;               // 行号
    int global = 0;      // 是否是全局变量
    int offset = 0; // 相对fp寄存器的偏移值
    std::string name;		// 变量的名字
    int array[2] = {0,0};           //如果是数组的话，存一下维度信息
};

struct BlockTable{//用map来实现表
    int line;                                       // 行号
    std::map<int, struct BlockTable*> sub_blocks;   // 嵌套的子块
    std::map<std::string, Var> self_symbols;       // 本块中定义的变量及其对应的符号表条目，比如foo(){int a=2;} self_symbols->a
};

struct Func{
    int cls;                // 返回值类型, class
    int param_num;          // 参数个数
    int line;               // 行号
    int stack_size = 0;     // 栈大小
    struct BlockTable func_block;//记录函数符号信息和子块
    std::string name;       //函数名
};

//全局作用域-函数作用域-块作用域
class SymbolTable{
public:
    std::map<std::string, Var> global_symbols;  // symbols for global variable 全局变量名称对应符号表条目
    std::map<std::string, Func> func_symbols;   // symbols for functions
    std::vector<BlockTable*> block_stack;           // stack for blocks
    std::string cur_func ="nullptr";                       // current function 
    int stack_frame;		// 目前相对于栈底的偏移                          
    int temp_var;                 //产生的临时变量数量 比如return int

public:
	SymbolTable(){
        temp_var = 0;
        cur_func = "$";
        block_stack.push_back(nullptr);
    }
    Var* lookup_var(const std::string & name);          // look up variable,return 符号表条目;查找顺序：当前块->函数块->全局块
    Func* lookup_func(const std::string & func_name);   // look up funtion
    Var* lookup_param(Func& func, int para_index);      // look up parameter in function declaration
    
    void addVar(std::string name, int cls, int type, int length, int line);
    //if cur_func == "$" add to global_symbols; else, add to func_symbols, which is the current block stack top+ check if the name is already exist
    void addFunc(std::string name, int ret_cls, int param_num, int line);
    void addBlock(int line);

    std::string gen_temp_var(int lc, int cls, SymbolTable& st, int type = TYPE_VAR);
    std::string gen_temp_array(int lc, int cls, int size, SymbolTable& st);
};


int getIntStringBase(std::string literal);

#endif