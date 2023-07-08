#include "symboltable.h"

#define SYM_CLS(x) ((x.cls == CLASS_INT     ? "int"    : \
					 x.cls == CLASS_BOOLEAN ? "bool"   : \
					 x.cls == CLASS_FLOAT   ? "float"  : \
					 x.cls == CLASS_DOUBLE  ? "double" : \
					  						  "void"))

#define SYM_TYP(x) ((x.type == TYPE_CONST       ? "const"       : \
					 x.type == TYPE_VAR         ? "var"         : \
					 x.type == TYPE_CONST_ARRAY ? "const_array" : \
					 x.type == TYPE_ARRAY       ? "array"       : \
					  							  "para"))

Var* SymbolTable::lookup_var(const std::string & name){
    int depth = block_stack.size();
    //从栈顶往下找
	while (depth > 1) {
		BlockTable *block = block_stack[depth - 1];
		if (block->self_symbols.count(name) == 1)
			return &block->self_symbols[name];
		depth--;
	}
    //在全局变量符号表中找
	if (global_symbols.count(name) == 1)
		return &global_symbols[name];
	else
		return nullptr;
}          // look up variable,return 符号表条目;查找顺序：当前块->函数块->全局块

Func* SymbolTable::lookup_func(const std::string & func_name){
	if (func_name == ""){
		return nullptr;
	}
    if (func_symbols.count(func_name) == 1)
		return &func_symbols[func_name];
	else
		return nullptr;
}   // look up funtion

Var* SymbolTable::lookup_param(Func& func, int para_index){
    for (auto i = func.func_block.self_symbols.begin(); i != func.func_block.self_symbols.end(); i++) {
		if (i->second.length == para_index && i->second.type == TYPE_PARAM) {
			return &(i->second);
		}
	}
	return nullptr;
}      // look up parameter in function declaration

void SymbolTable::addVar(std::string name, int cls, int type, int length, int line){
    Var *var = new Var;
	var->name = name;
	var->cls = cls;
	var->type = type;
	var->length = length;
	var->line = line;

	if (type == TYPE_CONST || type == TYPE_VAR || type == TYPE_PARAM) {
		stack_frame += (cls == CLASS_DOUBLE) ? 8 : 4;
		var->offset = stack_frame;
		//std::cout << "var: " << var->name << " offset: " << var->offset <<std::endl;
	}//不是数组，长度为8或4
	else if (type == TYPE_ARRAY || type == TYPE_CONST_ARRAY) {
		stack_frame += ((cls == CLASS_DOUBLE) ? 8 : 4) * length;
		var->offset = stack_frame;
	}//是数组，长度乘上数组长度length

	int depth = block_stack.size();
	if (cur_func == "$") {//全局
		if (global_symbols.count(name) == 0) {//还未出现过
			var->global = 1;
			global_symbols.insert(std::pair<std::string, Var>(name, *var));
		}
		else
			throw std::runtime_error("line " + std::to_string(line) + ": error: redeclaration of '" + name + "'\n");
	}
	else {
		BlockTable *top = block_stack[depth - 1];
		//局部出现过同名var
        if (top->self_symbols.count(name) != 0)
			throw std::runtime_error("line " + std::to_string(line) + ": error: redeclaration of '" + name + "'\n");

		//而且不能和函数参数名相同
		BlockTable *param_block = &func_symbols[cur_func].func_block;//当前函数函数块
		if (block_stack[depth - 2] == param_block && param_block->self_symbols.count(name) != 0)
			throw std::runtime_error("line " + std::to_string(line) + ": error: '" + name + "' redeclared as different kind of symbol\n");
		top->self_symbols.insert(std::pair<std::string, Var>(name, *var));
	}
}//if cur_func == "$" add to global_symbols;else,add to func_symbols,which is the current block stack top+ check if the name is already exist

void SymbolTable::addFunc(std::string name, int ret_cls, int param_num, int line){
    Func *func = new Func;
	func->name = name;
	func->cls = ret_cls;
	func->param_num = param_num;
	func->line = line;

	func->func_block.line = line;
	if (func_symbols.count(name) == 0)//检查同名函数是否出现过
		func_symbols.insert(std::pair<std::string, Func>(name, *func));//入函数符号表
	else
		throw std::runtime_error("line " + std::to_string(line) + ": error: redefinition of '" + name + "'\n");

	// 修改目前函数和block栈
	cur_func = name;
	block_stack.push_back(&func_symbols[name].func_block);
}

void SymbolTable::addBlock(int line){
    BlockTable *block = new BlockTable;
	block->line = line;

	int depth = block_stack.size();
	BlockTable *top = block_stack[depth - 1];
	top->sub_blocks.insert(std::pair<int, BlockTable *>(line, block));//加入栈顶block的子块
	block_stack.push_back(block);//入block栈
}

std::string SymbolTable::gen_temp_var(int lc, int cls, SymbolTable& st,int type){
	//std::cout << "lc: "<< lc << std::endl;
    assert(cls == CLASS_INT || cls == CLASS_BOOLEAN || cls == CLASS_FLOAT || cls == CLASS_DOUBLE);
    std::string name = "#" + std::to_string(++temp_var); //生成临时变量名字
    if(type == TYPE_CONST_ARRAY || type == TYPE_CONST){
        st.addVar(name, cls, TYPE_CONST, 0, lc);
	}
    else if(type == TYPE_ARRAY || type == TYPE_VAR){
        st.addVar(name, cls, TYPE_VAR, 0, lc);
    }
	return name;
}

std::string SymbolTable::gen_temp_array(int lc, int cls, int size, SymbolTable& st){
    assert(cls == CLASS_INT || cls == CLASS_BOOLEAN || cls == CLASS_FLOAT || cls == CLASS_DOUBLE);
    std::string name = "#" + std::to_string(++temp_var);
    st.addVar(name, cls, TYPE_ARRAY, size, lc);
	return name;
}

int getIntStringBase(std::string literal) {
    int base = 10;
    if (literal.substr(0, 1) == "0") {
        if (literal.substr(0, 2) == "0x" || literal.substr(0, 2) == "0X")
            base = 16;
        else
            base = 8;
    }
	else if (literal.substr(0, 1) == "-") {
        if (literal.substr(1, 2) == "0x" || literal.substr(1, 2) == "0X")
            base = 16;
        else if (literal.substr(1, 1) == "0")
            base = 8;
    }
	
    return base;
}