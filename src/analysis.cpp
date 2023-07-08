#include "analysis.h"
#include "symboltable.h"

void analysis::enterCompUnit(CACTParser::CompUnitContext *ctx)
{
    sym_table.addFunc("print_int", CLASS_VOID, 1, 0);
    sym_table.addVar("_int_", CLASS_INT, TYPE_PARAM, 0, 0);
    sym_table.block_stack.pop_back();
    sym_table.cur_func = "$";

    sym_table.addFunc("print_float", CLASS_VOID, 1, 0);
    sym_table.addVar("_float_", CLASS_FLOAT, TYPE_PARAM, 0, 0);
    sym_table.block_stack.pop_back();
    sym_table.cur_func = "$";

    sym_table.addFunc("print_double", CLASS_VOID, 1, 0);
    sym_table.addVar("_double_", CLASS_DOUBLE, TYPE_PARAM, 0, 0);
    sym_table.block_stack.pop_back();
    sym_table.cur_func = "$";

    sym_table.addFunc("print_bool", CLASS_VOID, 1, 0);
    sym_table.addVar("_bool_", CLASS_BOOLEAN, TYPE_PARAM, 0, 0);
    sym_table.block_stack.pop_back();
    sym_table.cur_func = "$";

    sym_table.addFunc("get_int", CLASS_INT, 0, 0);
    sym_table.block_stack.pop_back();
    sym_table.cur_func = "$";

    sym_table.addFunc("get_float", CLASS_FLOAT, 0, 0);
    sym_table.block_stack.pop_back();
    sym_table.cur_func = "$";

    sym_table.addFunc("get_double", CLASS_DOUBLE, 0, 0);
    sym_table.block_stack.pop_back();
    sym_table.cur_func = "$";

    this->outfile << "\t.option nopic\n";
    this->outfile << "\t.attribute arch, \"rv64i2p0_m2p0_a2p0_f2p0_d2p0_c2p0\"\n";
    this->outfile << "\t.attribute unaligned_access, 0\n";
    this->outfile << "\t.attribute stack_align, 16\n";
}

void analysis::exitCompUnit(CACTParser::CompUnitContext *ctx)
{
    if (sym_table.lookup_func("main") == nullptr) //没找到main函数
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: function 'main' not found\n");

    this->outfile.close();
}

/*--------------- const decl ---------------*/
void analysis::enterConstDecl(CACTParser::ConstDeclContext *ctx)
{
    std::string class_str = ctx->bType()->getText();
    u_stack.push(class_str); // push type info into stack for child to use
}
void analysis::exitConstDecl(CACTParser::ConstDeclContext *ctx)
{
    u_stack.pop(); // pop the type info
}

/*--------------- func decl ---------------*/
void analysis::enterFuncDef(CACTParser::FuncDefContext *ctx)
{
    std::string function_name = ctx->name->getText();
    std::string return_type_str = ctx->ret->getText();
    ctx->stack_size = 16;
    sym_table.stack_frame = 0;
    int return_type;

    if (return_type_str == "int") {
        return_type = CLASS_INT;
    }
    else if (return_type_str == "bool") {
        return_type = CLASS_BOOLEAN;
    }
    else if (return_type_str == "float") {
        return_type = CLASS_FLOAT;
    }
    else if (return_type_str == "double") {
        return_type = CLASS_DOUBLE;
    }
    else if (return_type_str == "void") {
        return_type = CLASS_VOID;
    }
    else {
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: unknown type name '" + return_type_str + "'\n");
    }

    // get the number of parameters & check main
    int param_num = ctx->funcFParam().size();
    if (function_name == "main") {
        if (param_num != 0) {
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: too many arguments for function 'main'\n");
        }
        if (return_type != CLASS_INT) {
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: function 'main' returns " + return_type_str + "\n");
        }
    }

    // add function to sym table
    sym_table.addFunc(function_name, return_type, param_num, ctx->getStart()->getLine());

    // add parameters to sym table
    int param_count= 0;
    for (auto &param : ctx->funcFParam()) {
        std::string param_type = param->bType()->getText();
        std::string param_name = param->Ident()->getText();
        int param_class;

        if (param_type == "int") {
            param_class = CLASS_INT;
        }
        else if (param_type == "double") {
            param_class = CLASS_DOUBLE;
        }
        else if (param_type == "float") {
            param_class = CLASS_FLOAT;
        }
        else if (param_type == "bool") {
            param_class = CLASS_BOOLEAN;
        }
        else {
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: unknown type name '" + param_type + "'\n");
        }

        sym_table.addVar(param_name, param_class, TYPE_PARAM, param_count, param->getStart()->getLine());
        Var *info = sym_table.lookup_var(param_name);
        // assert(!info);
        param_count++;
    }
    

    //prj3: enter funcdef here:
    //code gen:
    this->outfile << "\t.text\n";
    this->outfile << "\t.align\t1\n";
    this->outfile << "\t.globl\t" << function_name << "\n";
    this->outfile << "\t.type\t" << function_name << ", @function\n";
    this->outfile << function_name << ":\n";
}

void analysis::exitFuncDef(CACTParser::FuncDefContext *ctx)
{
    if(ctx->ret->getText() != "void" && ctx->block()->return_legal != true){
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: no return\n");
    }
    std::string func_name = ctx->name->getText();
    Func *funcInfo = sym_table.lookup_func(func_name);
    //assert(!funcInfo);
    ////std::cout<< sym_table.stack_frame <<std::endl;
    funcInfo->stack_size = sym_table.stack_frame;
    sym_table.cur_func = "$";
    sym_table.block_stack.pop_back();
    sym_table.stack_frame = 0;

    ctx->stack_size += ((funcInfo->stack_size) / 16 + 1) * 16;
    //栈帧里面的内容：
    //1.存的ra和s0
    //2.局部变量
    //3.传参传多的内容，但我看案例程序都没有这个，所以先不管这个屁
    //code gen:
    //开栈帧
    this->outfile << "\taddi\tsp, sp, -" << ctx->stack_size << std::endl;
    //存ra，位置是旧栈指针下面找8
    this->outfile << "\tsd\t\tra, " << ctx->stack_size - 8  << "(sp)\n";
    //存s0，位置是旧栈指针下面找16
    this->outfile << "\tsd\t\ts0, " << ctx->stack_size - 16 << "(sp)\n";
    //改变s0
    this->outfile << "\taddi\ts0, sp, " << ctx->stack_size << std::endl;

    //这里把函数内部的指令输出，函数内部的指令应该缓存在一个地方，这里都输出来
    //具体逻辑，等会再写
    this->outfile.close();
    buf.WriteBuf(outpath);
    buf.Clear();
    this->outfile.open(outpath, std::ios_base::app);

    //恢复ra，位置是旧栈指针下面找8
    this->outfile << "\tld\t\tra, " << ctx->stack_size - 8  << "(sp)\n";
    //恢复s0，位置是旧栈指针下面找16
    this->outfile << "\tld\t\ts0, " << ctx->stack_size - 16 << "(sp)\n";
    //恢复栈帧
    this->outfile << "\taddi\tsp, sp, " << ctx->stack_size << std::endl;
    //跳转回去
    this->outfile << "\tjr\t\tra" << std::endl;
}

void analysis::enterConstDefVal(CACTParser::ConstDefValContext *ctx)
{
    std::string var_name = ctx->Ident()->getText();         // 常量名
    std::string class_str = u_stack.top();                  // 类型名
    std::string literal_val = ctx->constExp()->getText();   // 常量值
    int var_cls;
    std::string value_str;
    int int_val;
    bool bool_val;
    float float_val;
    double double_val;

    if (class_str == "int") {
        var_cls = CLASS_INT;
        if (ctx->constExp()->getStart()->getType() != CACTLexer::IntConst)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid initializer of '" + var_name + " during enterConstDefVal'\n");
        int_val = std::stoi(literal_val, 0, getIntStringBase(literal_val));
        value_str = std::to_string(int_val);
    }
    else if (class_str == "bool") {
        var_cls = CLASS_BOOLEAN;
        if (ctx->constExp()->getStart()->getType() != CACTLexer::BoolConst)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid initializer of '" + var_name + " during enterConstDefVal'\n");
        bool_val = (literal_val == "true") ? 1 : 0;
        value_str = std::to_string(bool_val);
    }
    else if (class_str == "float") {
        var_cls = CLASS_FLOAT;
        if (ctx->constExp()->getStart()->getType() != CACTLexer::FloatConst)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid initializer of '" + var_name + " during enterConstDefVal'\n");
        float_val = std::stof(literal_val.substr(0, literal_val.length() - 1));
        value_str = std::to_string(float_val);
    }
    else if (class_str == "double") {
        var_cls = CLASS_DOUBLE;
        if (ctx->constExp()->getStart()->getType() != CACTLexer::DoubleConst)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid initializer of '" + var_name + " during enterConstDefVal'\n");
        double_val = std::stod(literal_val);
        value_str = std::to_string(double_val);
    }
    else{
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: unknown type name '" + class_str + "'\n");
    }

    sym_table.addVar(var_name, var_cls, TYPE_CONST, 0, ctx->getStart()->getLine());
    Var *var = sym_table.lookup_var(var_name);
}
void analysis::exitConstDefVal(CACTParser::ConstDefValContext *ctx) {}

//lzr feat: 常数数组初始化判定
void analysis::enterConstInitArray(CACTParser::ConstDefArrayContext *ctx){
    //判断这个数组的形式是否正确

}

void analysis::exitConstInitArray(CACTParser::ConstDefArrayContext *ctx){
    ;
}

void analysis::enterConstDefArray(CACTParser::ConstDefArrayContext *ctx)
{
    std::string var_name = ctx->Ident()->getText(); // 常量名

    //lzr feat: 获取一下数组维度，也就是左边的维度捏
    int array_dimention = ctx->IntConst().size();

    //lzr feat: 变量array_len：处理数组长度问题，以便将其放入符号表中
    int array_len = 1;

    //思考：有必要给数组一个属性，存的是维度！

    //lzr feat: 这段代码：第一是，判断每一个维度是不是负的；第二是，计算总长度
    for (auto &param : ctx->IntConst()) {
        // assert(!param);
        int array_len_dim = std::stoi(param->getText());
        array_len *= array_len_dim;
        if (array_len_dim <= 0)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: size of array '" + var_name + "' is negative\n");
    }


    std::string class_str = u_stack.top();  // 类型名
    int var_cls;
    std::string value_str;
    int int_val;
    bool bool_val;
    float float_val;
    double double_val;

    if (class_str == "int")
        var_cls = CLASS_INT;
    else if (class_str == "bool")
        var_cls = CLASS_BOOLEAN;
    else if (class_str == "float")
        var_cls = CLASS_FLOAT;
    else if (class_str == "double")
        var_cls = CLASS_DOUBLE;
    else
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: unknown type name '" + class_str + "'\n");
    sym_table.addVar(var_name, var_cls, TYPE_CONST_ARRAY, array_len, ctx->getStart()->getLine());

    Var *var = sym_table.lookup_var(var_name);

    
}
void analysis::exitConstDefArray(CACTParser::ConstDefArrayContext *ctx) {}

/*--------------- var decl ---------------*/
void analysis::enterVarDecl(CACTParser::VarDeclContext *ctx)
{
    for(auto &term:ctx->varDef()){
        term->where = ctx->where;
    }
    std::string class_str = ctx->bType()->getText();
    u_stack.push(class_str); // push type info into stack for child to use
}
void analysis::exitVarDecl(CACTParser::VarDeclContext *ctx)
{
    ////std::cout << "start to exit varDecl" << std::endl;
    u_stack.pop(); // pop the type info
    ////std::cout << "exited varDecl" << std::endl;
}

void analysis::enterDefVal(CACTParser::DefValContext *ctx)
{
    std::string var_name = ctx->Ident()->getText();         // 变量名
    std::string class_str = u_stack.top();                  // 类型名
    int var_cls;
    int size;
    if (class_str == "int")
        {var_cls = CLASS_INT;
        size = 4;}
    else if (class_str == "bool")
        {var_cls = CLASS_BOOLEAN;
        size = 4;}
    else if (class_str == "float")
        {var_cls = CLASS_FLOAT;
        size = 4;}
    else if (class_str == "double")
        {var_cls = CLASS_DOUBLE;
        size = 8;}
    else
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: unknown type name '" + class_str + "'\n");
    //这里感觉可以改一下
    //1.在符号表里面存储
    sym_table.addVar(var_name, var_cls, TYPE_VAR, 0, ctx->getStart()->getLine());

    //全局变量代码产生：无初值非数组
    if(ctx->where == OUTSIDE){
        //std::cout<<"outside var: " << var_name << std::endl;
        /*
        	.globl	dp
            .bss
            .align	3
            .type	dp, @object
            .size	dp, 283024
        dp:
            .zero	283024
        */
        this->outfile << "\t.globl\t" << var_name << std::endl;
        this->outfile << "\t.bss\n";
        this->outfile << "\t.align\t3\n";
        this->outfile << "\t.type\t" << var_name << ", @object\n";
        this->outfile << "\t.size\t" << var_name << ", " << size << std::endl;
        this->outfile << var_name << ":\n";
        this->outfile << "\t.zero\t" << size << std::endl;
    }
}
void analysis::exitDefVal(CACTParser::DefValContext *ctx) {}

void analysis::enterDefArray(CACTParser::DefArrayContext *ctx)
{
    std::string var_name = ctx->Ident()->getText(); // 类型名
    
    //lzr feat: 获取一下数组维度，也就是左边的维度捏
    int array_dimention = ctx->IntConst().size();

    //lzr feat: 变量array_len：处理数组长度问题，以便将其放入符号表中
    int array_len = 1;

    //思考：有必要给数组一个属性，存的是维度！

    //lzr feat: 这段代码：第一是，判断每一个维度是不是负的；第二是，计算总长度
    int lzr = 0;
    int array_len_info[2] = {0,0};
    for (auto &param : ctx->IntConst()) {
        int array_len_dim = std::stoi(param->getText());
        array_len_info[lzr++] = array_len_dim;
        array_len *= array_len_dim;
        if (array_len_dim <= 0)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: size of array '" + var_name + "' is negative\n");
    }

    std::string class_str = u_stack.top();  // 类型名
    int var_cls;                            // 类型变量
    int size;
    if (class_str == "int")
        {var_cls = CLASS_INT;
        size = 4 * array_len;}
    else if (class_str == "bool")
        {var_cls = CLASS_BOOLEAN;
        size = 4 * array_len;}
    else if (class_str == "float")
        {var_cls = CLASS_FLOAT;
        size = 4 * array_len;}
    else if (class_str == "double")
        {var_cls = CLASS_DOUBLE;
        size = 8 * array_len;}
    else
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: unknown type name '" + class_str + "'\n");
    sym_table.addVar(var_name, var_cls, TYPE_ARRAY, array_len, ctx->getStart()->getLine());

    Var *array_info = sym_table.lookup_var(var_name);
    array_info->array[0] = array_len_info[0];
    array_info->array[1] = array_len_info[1];
        //全局变量代码产生：无初值数组
    if(ctx->where == OUTSIDE){
        //std::cout<<"outside var: " << var_name << std::endl;
        /*
        	.globl	dp
            .bss
            .align	3
            .type	dp, @object
            .size	dp, 283024
        dp:
            .zero	283024
        */
        this->outfile << "\t.globl\t" << var_name << std::endl;
        this->outfile << "\t.bss\n";
        this->outfile << "\t.align\t3\n";
        this->outfile << "\t.type\t" << var_name << ", @object\n";
        this->outfile << "\t.size\t" << var_name << ", " << size << std::endl;
        this->outfile << var_name << ":\n";
        this->outfile << "\t.zero\t" << size << std::endl;
    }
}
void analysis::exitDefArray(CACTParser::DefArrayContext *ctx) {}

void analysis::enterDefInitVal(CACTParser::DefInitValContext *ctx)
{
    std::string var_name = ctx->Ident()->getText();         // 变量名
    std::string class_str = u_stack.top();                  // 类型名
    std::string literal_val = ctx->constExp()->getText();   // 初始值
    int var_cls;
    int value_str;
    int int_val;
    bool bool_val;
    float float_val;
    double double_val;
    int doub_val = 0;
    int size;
    
    if (class_str == "int") {
        var_cls = CLASS_INT;
        if (ctx->constExp()->getStart()->getType() != CACTLexer::IntConst)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid initializer of '" + var_name + " during enterDefInitVal'\n");
        int_val = std::stoi(literal_val, 0, getIntStringBase(literal_val));
        value_str = int_val;
        size = 4;
    }
    else if (class_str == "bool") {
        var_cls = CLASS_BOOLEAN;
        if (ctx->constExp()->getStart()->getType() != CACTLexer::BoolConst)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid initializer of '" + var_name + " during enterDefInitVal'\n");
        bool_val = (literal_val == "true") ? 1 : 0;
        value_str = bool_val;
        size = 4;
    }
    else if (class_str == "float") {
        var_cls = CLASS_FLOAT;
        if (ctx->constExp()->getStart()->getType() != CACTLexer::FloatConst)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid initializer of '" + var_name + " during enterDefInitVal'\n");
        float_val = std::stof(literal_val.substr(0, literal_val.length() - 1));
        value_str = *(int*)(&float_val);
        size = 4;
    }
    else if (class_str == "double") {
        var_cls = CLASS_DOUBLE;
        if (ctx->constExp()->getStart()->getType() != CACTLexer::DoubleConst)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid initializer of '" + var_name + " during enterDefInitVal'\n");
        double_val = std::stod(literal_val);
        long long temp = *(long long *)(&double_val);
        value_str = temp >> 32;
        doub_val = temp & 0x00000000ffffffff;
        size = 8;
    }
    else{
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: unknown type name '" + class_str + "'\n");
    }

    sym_table.addVar(var_name, var_cls, TYPE_VAR, 0, ctx->getStart()->getLine());
    Var *var = sym_table.lookup_var(var_name);


    //全局变量代码产生：有初值非数组
    if(ctx->where == OUTSIDE){
        //std::cout<<"outside var: " << var_name << std::endl;
        /*
        	.globl	dp
            .bss
            .align	3
            .type	dp, @object
            .size	dp, 283024
        dp:
            .zero	283024
        */
        this->outfile << "\t.globl\t" << var_name << std::endl;
        this->outfile << "\t.data\n";
        this->outfile << "\t.align\t3\n";
        this->outfile << "\t.type\t" << var_name << ", @object\n";
        this->outfile << "\t.size\t" << var_name << ", " << size << std::endl;
        this->outfile << var_name << ":\n";
        this->outfile << "\t.word\t" << value_str << std::endl;
        if(doub_val){
            this->outfile << "\t.word\t" << doub_val << std::endl;
        }
    }
    else{
        //局部变量赋初始值
        //其他类型没写
        buf.AddInst("\tli\t\ta5, ");
        buf.AddInst(std::to_string(value_str));
        buf.AddInst("\n\tsw\t\ta5, -");
        buf.AddInst(std::to_string(var->offset + 16));
        buf.AddInst("(s0)\n");
    }
}
void analysis::exitDefInitVal(CACTParser::DefInitValContext *ctx) {}


void analysis::enterDefInitArray(CACTParser::DefInitArrayContext *ctx)
{
    //ctx->constInitVal()->where = OUTSIDE;
    std::string var_name = ctx->Ident()->getText(); // 类型名

    //lzr feat: 获取一下数组维度，也就是左边的维度捏
    int array_dimention = ctx->IntConst().size();

    //lzr feat: 变量array_len：处理数组长度问题，以便将其放入符号表中
    int array_len = 1;

    //思考：有必要给数组一个属性，存的是维度！

    //lzr feat: 这段代码：第一是，判断每一个维度是不是负的；第二是，计算总长度

    int lzr = 0;
    int array_len_info[2] = {0,0};
    for (auto &param : ctx->IntConst()) {
        int array_len_dim = std::stoi(param->getText());
        array_len_info[lzr++] = array_len_dim;
        array_len *= array_len_dim;
        if (array_len_dim <= 0)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: size of array '" + var_name + "' is negative\n");
    }

    
    std::string class_str = u_stack.top();  // 类型名
    int var_cls;
    std::string value_str;
    int int_val;
    bool bool_val;
    float float_val;
    double double_val;

    int size = 4 * array_len;

    if (class_str == "int")
        var_cls = CLASS_INT;
    else if (class_str == "bool")
        var_cls = CLASS_BOOLEAN;
    else if (class_str == "float")
        var_cls = CLASS_FLOAT;
    else if (class_str == "double")
        {var_cls = CLASS_DOUBLE;
        size = 8 * array_len;}
    else
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: unknown type name '" + class_str + "'\n");
    sym_table.addVar(var_name, var_cls, TYPE_ARRAY, array_len, ctx->getStart()->getLine());

    Var *var = sym_table.lookup_var(var_name);
    var->array[0] = array_len_info[0];
    var->array[1] = array_len_info[1];

    //全局变量代码产生：有初值数组
    if(ctx->where == OUTSIDE){
        //std::cout<<"outside var: " << var_name << std::endl;
        /*
        	.globl	dp
            .bss
            .align	3
            .type	dp, @object
            .size	dp, 283024
        dp:
            .zero	283024
        */
        this->outfile << "\t.globl\t" << var_name << std::endl;
        this->outfile << "\t.data\n";
        this->outfile << "\t.align\t3\n";
        this->outfile << "\t.type\t" << var_name << ", @object\n";
        this->outfile << "\t.size\t" << var_name << ", " << size << std::endl;
        this->outfile << var_name << ":\n";
        int doub_val = 0;
        for(auto &term:ctx->constExp()){
            std::string output_val;
            std::string literal_val = term->getText();   // 初始值
            if (class_str == "int"){
                int output = std::stoi(literal_val, 0, getIntStringBase(literal_val));
                output_val = std::to_string(output);
            }
            else if (class_str == "bool"){
                int output = (literal_val == "true") ? 1 : 0;
                output_val = std::to_string(output);
            }
            else if (class_str == "float"){
                float output = std::stof(literal_val.substr(0, literal_val.length() - 1));
                int temp = *(int*)(&output);
                output_val = std::to_string(temp);
            }
            else if (class_str == "double"){
                double output = std::stod(literal_val);
                long long temp = *(long long *)(&output);
                int value_str = temp >> 32;
                doub_val = temp & 0x00000000ffffffff;
                output_val = std::to_string(value_str);
            }
            this->outfile << "\t.word\t" << output_val << std::endl;
            if(doub_val){
                this->outfile << "\t.word\t" << doub_val << std::endl;
            }
        }
    }
}
void analysis::exitDefInitArray(CACTParser::DefInitArrayContext *ctx) {
    ////std::cout<<"exited defInitArray" << std::endl;
}

/*--------------- block ---------------*/
void analysis::enterBlock(CACTParser::BlockContext *ctx)
{
    sym_table.addBlock(ctx->getStart()->getLine());
    for(auto &term:ctx->blockItem()){
        term->LA = ctx->LA;
        term->LB = ctx->LB;
    }
}
void analysis::exitBlock(CACTParser::BlockContext *ctx)
{
    //先初设是合法的
    ctx->return_legal = false;

    for (auto &Item : ctx->blockItem()){
        ctx->return_legal |= Item->return_legal;
    }

    sym_table.block_stack.pop_back();
}

void analysis::enterBlockItem(CACTParser::BlockItemContext * ctx) {
    if(ctx->stmt()){
        ctx->stmt()->LA = ctx->LA;
        ctx->stmt()->LB = ctx->LB;
    }       
}
void analysis::exitBlockItem(CACTParser::BlockItemContext * ctx) {
    if(ctx->decl()){
        ctx->return_legal = false;
    }
    else{
        ctx->return_legal = ctx->stmt()->return_legal;
    }
}

/*--------------- function ---------------*/
// Ident '(' (funcRParams)? ')'
void analysis::enterFuncall(CACTParser::FuncallContext *ctx)
{
    auto func_name = ctx->Ident()->getText();
    auto func_info = sym_table.lookup_func(func_name);

    int param_num = 0;
    if (ctx->funcRParams())
        param_num = ctx->funcRParams()->exp().size();
    
    if (!func_info){
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: undefined function '" + func_name +"'\n");
    }
    if (param_num < func_info->param_num){
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: too few arguments to function '" + func_name +"'\n");
    }
    if (param_num > func_info->param_num){
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: too many arguments to function '" + func_name +"'\n");
    }

}
void analysis::exitFuncall(CACTParser::FuncallContext *ctx)
{
    auto func_name = ctx->Ident()->getText();
    ////std::cout<<"func: " << func_name << std::endl;
    auto func_info = sym_table.lookup_func(func_name);
    int para_count = 0;
    //准备一下参数！！传参传参！我是说！！！传参！！！
    //参数就顺着a0-a7，fa0-fa7
    ////std::cout << "name: " << func_name << std::endl;
    //得考虑传指针的情况……指针传顶点
    if(ctx->funcRParams() != nullptr){
        int ZhengXingNum = 0, FuDianNum = 0;
        for (auto &param: ctx->funcRParams()->exp()){
            ////std::cout << "param: " << param->name << std::endl;
            auto name = param->name;
            Var *par = sym_table.lookup_var(name);
            auto type = param->cls;
            if(par != nullptr){
                //不是数
                if(par->global){
                    //如果是全局变量
                    buf.AddInst("\tlui\ta");
                    buf.AddInst(", \%hi(");
                    buf.AddInst(name);
                    buf.AddInst(")\n\taddi\ta4, a5,\%lo(");
                    buf.AddInst(name);
                    buf.AddInst(")\n");
                }
                else{
                    if(type == CLASS_INT || type == CLASS_BOOLEAN){
                        //整形，往寄存器里面放，那你就直接放啊你用a5干嘛好迷惑啊
                        buf.AddInst("\tlw\t\ta");
                        buf.AddInst(std::to_string(ZhengXingNum++));
                    }
                    else{
                    buf.AddInst("\tlw\t\tfa");
                    buf.AddInst(std::to_string(FuDianNum++));
                }
                    //要被赋值的变量存在哪里！
                    buf.AddInst(", -");
                    //找一下位置
                    buf.AddInst(std::to_string(par->offset + 16));
                    buf.AddInst("(s0)\n");
                }
                
            }
            else{
                //传参传数
                if(type == CLASS_INT || type == CLASS_BOOLEAN){
                    //整形，往寄存器里面放，那你就直接放啊你用a5干嘛好迷惑啊
                    buf.AddInst("\tli\t\ta");
                    buf.AddInst(std::to_string(ZhengXingNum++));
                }
                else{
                    buf.AddInst("\tli\t\tfa");
                    buf.AddInst(std::to_string(FuDianNum++));
                }
                //数
                buf.AddInst(", ");
                buf.AddInst(name);
                buf.AddInst("\n");
            }
        }
    }
    

    //准备好了参数之后进行调用call
    buf.AddInst("\tcall\t");
    buf.AddInst(func_name);
    buf.AddInst("\n");

    if (ctx->funcRParams()) {
        for (auto &param : ctx->funcRParams()->exp()) {
            auto *para_info = sym_table.lookup_param(*func_info, para_count);
            if (para_info->cls != param->cls)
                throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: incompatible type for argument " + std::to_string(para_count) + " of '" + func_name +"'\n");

            Var *temp_info = sym_table.lookup_var(param->name);
            para_count++;
        }
    }

    if (func_info->cls == CLASS_VOID) {
        ctx->cls = CLASS_VOID;
    }
    else {
        //啊这个函数啊，他nnd有返回值啊，生成了一个临时变量啊，那就得把返回值存进去哈！！！奶奶的哈！！！
        auto ret_var = sym_table.gen_temp_var(ctx->getStart()->getLine(), func_info->cls, sym_table);
        Var *ret_info = sym_table.lookup_var(ret_var);
        ctx->cls = func_info->cls;
        ctx->name = ret_var;
        //TD::这没写完，返回值是浮点还没写、
        //我看了一下，返回值没有浮点的，不管了
        if(ctx->cls == CLASS_INT){
            buf.AddInst("\tsw\t\ta0, -");
            buf.AddInst(std::to_string(16 + ret_info->offset));
            buf.AddInst("(s0)\n");
        }
    }
}

void analysis::enterReturnStmt(CACTParser::ReturnStmtContext *ctx) {
    ////std::cout << "enter return stmt" << std::endl;
    //为exp创建一个临时变量
}
void analysis::exitReturnStmt(CACTParser::ReturnStmtContext *ctx)
{
    ////std::cout << "start to exit return stmt" << std::endl;
    ctx->return_legal = true;
    //std::cout << "ctxxxxxxxxxxx  " << ctx->exp()->getText() << std::endl;
    if (ctx->exp() != nullptr){
        std::string return_name = ctx->exp()->name;
        //std::cout << "return name: " << return_name << std::endl;
        Var *return_info = sym_table.lookup_var(return_name);
        if(return_info == nullptr){
            return_name = ctx->exp()->getText();
        }
        //std::cout << return_info << std::endl;
        Func *func = sym_table.lookup_func(sym_table.cur_func);
        ////std::cout << func->name <<std::endl;
        int cls = ctx->exp()->cls;
        if (cls != func->cls){
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: incompatible type when returning\n");
        }
        //返回语句，，，情况的话就是，数/左值/布尔/函数调用！
        //函数调用、数值和布尔的话，是有名字的，而数是查不到的
        //案例程序里面我看只有return 0和return a，笑死，只考虑这俩了
        if(return_info == nullptr){
            buf.AddInst("\tli\t\ta0, ");
            buf.AddInst(return_name);
            buf.AddInst("\n");
        }
        else{
            buf.AddInst("\tld\t\ta0, -");
            buf.AddInst(std::to_string(return_info->offset + 16));
            buf.AddInst("(s0)\n");
        }
    }
    else{
        Func *func = sym_table.lookup_func(sym_table.cur_func);
        if (func->cls != CLASS_VOID)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: incompatible type when returning\n");
    }

}

/*--------------- condition ---------------*/
void analysis::enterCond(CACTParser::CondContext *ctx) {
    ctx->lOrExp()->LA = ctx->LA;
    ctx->lOrExp()->LB = ctx->LB;
}
void analysis::exitCond(CACTParser::CondContext *ctx)
{
    ctx->cls = ctx->lOrExp()->cls;
    ctx->name = ctx->lOrExp()->name;
    if (ctx->cls != CLASS_BOOLEAN){
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: incompatible type in condition\n");
    }
}

void analysis::enterIfCond(CACTParser::IfCondContext *ctx) {
    ctx->cond()->LA = ctx->LA;
    ctx->cond()->LB = ctx->LB;
    ////std::cout<<"enter if cond\n";
}

void analysis::exitIfCond(CACTParser::IfCondContext *ctx)
{
    ctx->cls = ctx->cond()->cls;
    ctx->name = ctx->cond()->name;
    //std::cout<< "nameeeeeee:   " << ctx->name << std::endl;


}

void analysis::enterWhileCond(CACTParser::WhileCondContext *ctx) {
    ctx->cond()->LA = ctx->LA;
    ctx->cond()->LB = ctx->LB;

    // //更新while标签信息
    // this->WhileLable[WhileNum][0] = ctx->LA;
    // this->WhileLable[WhileNum][0] = ctx->LB;
    // this->WhileNum++;
}

void analysis::exitWhileCond(CACTParser::WhileCondContext *ctx)
{
    ctx->cls = ctx->cond()->cls;
    ctx->name = ctx->cond()->name;
}

void analysis::enterExprStmt(CACTParser::ExprStmtContext * ctx) {

}
void analysis::exitExprStmt(CACTParser::ExprStmtContext * ctx) {
    ctx->return_legal = false;
}

void analysis::enterBlockStmt(CACTParser::BlockStmtContext * ctx) {
    ctx->block()->LA = ctx->LA;
    ctx->block()->LB = ctx->LB;
}
void analysis::exitBlockStmt(CACTParser::BlockStmtContext * ctx) {
    ctx->return_legal = ctx->block()->return_legal;
}

void analysis::enterIfStmt(CACTParser::IfStmtContext *ctx)
{
    ctx->LA = this->LableIndex++;
    ctx->LB = this->LableIndex++;
    ctx->ifCond()->LA = ctx->LA;
    ctx->ifCond()->LB = ctx->LB;
    ctx->stmt()->LA = ctx->LA;
    ctx->stmt()->LB = ctx->LB;
}
void analysis::exitIfStmt(CACTParser::IfStmtContext *ctx)
{
    ctx->return_legal = ctx->stmt()->return_legal;
    //跳转逻辑
    buf.AddInst(".L");
    buf.AddInst(std::to_string(ctx->LA));
    buf.AddInst(":\n");
}

void analysis::enterIfElseStmt(CACTParser::IfElseStmtContext *ctx)
{
    //产生两个标签
    ctx->LA = this->LableIndex++;
    ctx->LB = this->LableIndex++;
    ctx->ifCond()->LA = ctx->LA;
    ctx->ifCond()->LB = ctx->LB;
    ctx->stmt()->LA = ctx->LA;
    ctx->stmt()->LB = ctx->LB;
    ctx->elseStmt()->LA = ctx->LA;
    ctx->elseStmt()->LB = ctx->LB;
}

void analysis::exitIfElseStmt(CACTParser::IfElseStmtContext *ctx)
{
    ctx->return_legal = ctx->stmt()->return_legal && ctx->elseStmt()->return_legal;
    //跳转逻辑
    buf.AddInst(".L");
    buf.AddInst(std::to_string(ctx->LB));
    buf.AddInst(":\n");
}

void analysis::enterElseStmt(CACTParser::ElseStmtContext *ctx)
{
    //跳转逻辑
    buf.AddInst("\tjal\t\ta5, .L");
    buf.AddInst(std::to_string(ctx->LB));
    buf.AddInst("\n");
    buf.AddInst(".L");
    buf.AddInst(std::to_string(ctx->LA));
    buf.AddInst(":\n");
}

void analysis::exitElseStmt(CACTParser::ElseStmtContext *ctx) {
    ctx->return_legal = ctx->stmt()->return_legal;
}
void analysis::enterWhileStmt(CACTParser::WhileStmtContext *ctx) {
    ctx->LA = this->LableIndex++;
    ctx->LB = this->LableIndex++;
    ctx->whileCond()->LA = ctx->LA;
    ctx->whileCond()->LB = ctx->LB;
    ctx->stmt()->LA = ctx->LA;
    ctx->stmt()->LB = ctx->LB;
    //跳转逻辑
    buf.AddInst(".L");
    buf.AddInst(std::to_string(ctx->LB));
    buf.AddInst(":\n");
    //更新while标签信息
    this->WhileLable[WhileNum][0] = ctx->LA;
    this->WhileLable[WhileNum][1] = ctx->LB;
    this->WhileNum++;
}
void analysis::exitWhileStmt(CACTParser::WhileStmtContext *ctx) {
    this->WhileNum--;
    ctx->return_legal = ctx->stmt()->return_legal;
    //跳转逻辑
    buf.AddInst("\tjal\t\ta5, .L");
    buf.AddInst(std::to_string(ctx->LB));
    buf.AddInst("\n");
    buf.AddInst(".L");
    buf.AddInst(std::to_string(ctx->LA));
    buf.AddInst(":\n");
}

void analysis::enterBreakStmt(CACTParser::BreakStmtContext *ctx) {
    //这么取LA貌似不正确，想了想，这里应该是最接近的一层while循环才对
    //用栈吧
    //我维护一个while相关信息的栈？用数组实现吧
    buf.AddInst("\tjal\t\ta5, .L");
    buf.AddInst(std::to_string(this->WhileLable[this->WhileNum-1][0]));
    buf.AddInst("\n");
}
void analysis::exitBreakStmt(CACTParser::BreakStmtContext *ctx)
{
    ctx->return_legal = false;
}

void analysis::enterContinueStmt(CACTParser::ContinueStmtContext *ctx) {
    buf.AddInst("\tjal\t\ta5, .L");
    buf.AddInst(std::to_string(this->WhileLable[this->WhileNum-1][1]));
    buf.AddInst("\n");
}
void analysis::exitContinueStmt(CACTParser::ContinueStmtContext *ctx)
{
    ctx->return_legal = false;;
}

/*--------------- expression ---------------*/
//  lVal '=' exp ';'
void analysis::enterAssignStmt(CACTParser::AssignStmtContext *ctx) {
    //printf("111\n");
    ctx->lVal()->left_or_right = 1;
}
void analysis::exitAssignStmt(CACTParser::AssignStmtContext *ctx)
{
    ctx->return_legal = false;
    bool elemwise = false;
    bool isArray = false;
    std::string lval_name = ctx->lVal()->name;
    std::string index_name;
    // lVal is: array[index]
    ////std::cout<<"exit assignstmt content:" << ctx->getText().c_str() << std::endl;
    //std::cout<<"lval_name:" << lval_name << std::endl;

    //右值
    // check rhs
    std::string rval_name = ctx->exp()->name;
    auto *rval_var_info = sym_table.lookup_var(rval_name);
    //std::cout << "right name: " << rval_name << std::endl;

    //lzr feat: 多维数组情况多个exp，进行修改
    if(!ctx->lVal()->exp().empty()) //先判断是否为空
    {   for (auto &term : ctx->lVal()->exp()) {
            if (term != nullptr) {
                index_name = term->name;
                auto *index_var_info = sym_table.lookup_var(index_name);
                if(index_var_info)
                if (index_var_info->cls != CLASS_INT)
                    throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: array subscript is not an integer\n");
                isArray = true;
            }
        }
        Var * left_info = sym_table.lookup_var(lval_name);
        //左值是数组咯，应该在这写咯
        //std::cout<<"left arrrry\n";
        if(rval_var_info == nullptr){
            if(left_info->cls != CLASS_DOUBLE){
                buf.AddInst("\tli\t\ta7, ");
                buf.AddInst(ctx->exp()->getText());
                buf.AddInst("\n");

                //赋值给左值
                buf.AddInst("\tsw\t\ta7, 0(a3)\n");
            }
            //其余类型
        }
        else{
            //右值不是数了，这样会好很多，因为这种情况，是自带name的，呃呃
            //说真的看别人的代码不能看注释，因为全是f-word和鬼话
            if(rval_var_info->cls == CLASS_INT){
                buf.AddInst("\tlw\t\ta7, -");
                buf.AddInst(std::to_string(rval_var_info->offset + 16));
                buf.AddInst("(s0)\n");
                //赋值给左值
                buf.AddInst("\tsw\t\ta7, 0(a3)\n");
            }
            //其余类型
        }
    }
    else{
        ////std::cout << "lval not array\n";
        //这里是左值不为空的情况捏
        //获取左值信息
        Var * left_info = sym_table.lookup_var(lval_name);
        if(rval_var_info == nullptr){
            //右值是数……救命
            //我的思路是直接store一下，store可以是立即数吗
            //额，似乎是不可以的捏
            //先把立即数存在一个寄存器里面吧
            //我觉得得给寄存器单独写一个类了，要不然似乎是不太行的
            //救命我在干什么，完全不会写
            if(left_info->cls == CLASS_INT){
                buf.AddInst("\tli\t\ta7, ");
                buf.AddInst(ctx->exp()->getText());
                buf.AddInst("\n");

                //赋值给左值
                buf.AddInst("\tsw\t\ta7, -");
                buf.AddInst(std::to_string(left_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            //其余类型
        }
        else{
            //右值不是数了，这样会好很多，因为这种情况，是自带name的，呃呃
            //说真的看别人的代码不能看注释，因为全是f-word和鬼话

            //右值对应的变量/临时变量存到一个寄存器里面，这里先用a7
            //先把int的写了，别的再说
            if(rval_var_info->cls == CLASS_INT){
                buf.AddInst("\tlw\t\ta7, -");
                buf.AddInst(std::to_string(rval_var_info->offset + 16));
                buf.AddInst("(s0)\n");
                //赋值给左值
                buf.AddInst("\tsw\t\ta7, -");
                buf.AddInst(std::to_string(left_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            //其余类型
        }
    }


    // check lhs
    auto *lhs_var_info = sym_table.lookup_var(lval_name);
    if (lhs_var_info == nullptr){
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: lval '" + lval_name + "' undeclared (first use in this function1)\n");
    }
    
    ////std::cout<<"hello"<<std::endl;

    // if (rval_var_info == nullptr){
    //     throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: '" + lval_name + "' undeclared (first use in this function)\n");
    // }

    ////std::cout<<"hello"<<std::endl;
    // 类型检查
    if(rval_var_info){
        if (rval_var_info->cls != lhs_var_info->cls)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: incompatible types when assigning\n");
        
        if (lhs_var_info->type == TYPE_ARRAY &&
            (rval_var_info->type == TYPE_ARRAY || rval_var_info->type == TYPE_CONST_ARRAY)){
            elemwise = true;
        }
        int var_cls = lhs_var_info->cls;
        int var_type = lhs_var_info->type;
    }
    
    ////std::cout<<"hello"<<std::endl;

    if (lhs_var_info->type == TYPE_CONST || lhs_var_info->type == TYPE_CONST_ARRAY){
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: assignment of read-only variable\n");
    }
    if (ctx->exp()->cls == CLASS_VOID)
        throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: void value not ignored as it ought to be\n");

}

void analysis::enterExp(CACTParser::ExpContext *ctx) {
    ////std::cout << "entered exp" << std::endl;
}
void analysis::exitExp(CACTParser::ExpContext *ctx)
{
    ////std::cout<<"start to exit exp" << std::endl;
    // addExp
    if (ctx->addExp()) {
        ctx->cls = ctx->addExp()->cls;
        ctx->name = ctx->addExp()->name;
    }
    // BoolConst
    if (ctx->BoolConst()) {
        ctx->cls = CLASS_BOOLEAN;
    }
    ////std::cout << "exp name: " << std::endl;
    ////std::cout << "exit exp content:" <<ctx->getText().c_str() << std::endl;
}

void analysis::enterLVal(CACTParser::LValContext *ctx) {}
void analysis::exitLVal(CACTParser::LValContext *ctx)
{

    
    //lzr feat: 多维数组情况多个exp，进行修改
    
    ctx->name = ctx->Ident()->getText();
    if (ctx->exp().empty()) {
            std::string var_name = ctx->Ident()->getText();
            Var *var_info = sym_table.lookup_var(var_name);
            if (!var_info)
                throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: '" + var_name + "' undeclared (first use in this function)\n");
            ctx->cls = var_info->cls;
    }
    else{
        std::string array_name = ctx->Ident()->getText();
        Var *array_info = sym_table.lookup_var(array_name);
        if (!array_info)
            throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: '" + array_name + "' undeclared (first use in this function)\n");
        //if (!(array_info->type == TYPE_ARRAY || array_info->type == TYPE_CONST_ARRAY))
            //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: subscripted value is not array\n");
        
        ctx->cls = array_info->cls;
        ctx->name = sym_table.gen_temp_var(ctx->getStart()->getLine(), ctx->cls, sym_table);
        Var *temp = sym_table.lookup_var(ctx->name);
        //std::cout << "array temp var: " << ctx->name <<std::endl;
        //std::cout << "array: " << array_info->array[0] << "   " << array_info->array[1] << std::endl;
        int dim = 0;
        Var *exp1;
        Var *exp2;
        for(auto &term: ctx->exp()){
            if(dim == 0){
                exp1 = sym_table.lookup_var(term->name);
                dim++;
            }
            else{
                exp2 = sym_table.lookup_var(term->name);
                dim++;
            }
        }

        //处理数组，找到数组首地址，此时数组首地址存在a4
        // lui	a5,%hi(d)
	    // addi	a4,a5,%lo(d)
        buf.AddInst("\tlui\ta5, \%hi(");
        buf.AddInst(array_name);
        buf.AddInst(")\n\taddi\ta4, a5,\%lo(");
        buf.AddInst(array_name);
        buf.AddInst(")\n");

        //根据a4计算偏移
        // lw	a5,-20(s0)
        // slli	a5,a5,2
        // add	a5,a4,a5
        if(dim == 1){
            //一维数组
            buf.AddInst("\tlw\t\ta5, -");
            buf.AddInst(std::to_string(16+exp1->offset));
            if(ctx->cls != CLASS_DOUBLE){
                buf.AddInst("(s0)\n\tslli\ta5, a5, 2");
            }
            else{
                buf.AddInst("(s0)\n\tslli\ta5, a5, 3");
            }
            buf.AddInst("\n\tadd\ta5, a4, a5\n");
        }
        else{
            //二维数组感觉就是exp1 * array_info->array[1] + exp2
            //exp1存到a5
            buf.AddInst("\tlw\t\ta5, -");
            buf.AddInst(std::to_string(16+exp1->offset));
            buf.AddInst("(s0)\n");
            //array_info->array[1]存到a6
            buf.AddInst("\tli\t\ta6, ");
            buf.AddInst(std::to_string(array_info->array[1]));
            buf.AddInst("\n");
            //乘，结果放在a5
            buf.AddInst("\tmul\ta5, a5, a6\n");
            //exp2存到a6
            buf.AddInst("\tlw\t\ta6, -");
            buf.AddInst(std::to_string(16+exp2->offset));
            buf.AddInst("(s0)\n");
            //加，结果放在a5
            buf.AddInst("\tadd\ta5, a6, a5\n");
            //移动
            if(ctx->cls != CLASS_DOUBLE){
                buf.AddInst("\tslli\ta5, a5, 2\n");
            }
            else{
                buf.AddInst("\tslli\ta5, a5, 3\n");
            }
            buf.AddInst("\tadd\ta5, a4, a5\n");
        }

        //如果是在右边的时候要这么写……在左边其实……其实……其实是另一种写法啊……
        //现在要取的东西的地址存在a5，利用a5寻址
        // lw	a5,0(a5)
        if(ctx->left_or_right == 0){
            if(ctx->cls != CLASS_DOUBLE){
                buf.AddInst("\tlw\t\ta5, 0(a5)\n");
                buf.AddInst("\tsw\t\ta5, -");
                buf.AddInst(std::to_string(16+temp->offset));
                buf.AddInst("(s0)\n");
            }
            else{
                buf.AddInst("\tld\t\ta5, 0(a5)\n");
                buf.AddInst("\tsd\t\ta5, -");
                buf.AddInst(std::to_string(16+temp->offset));
                buf.AddInst("(s0)\n");
            }
        }
        else{
            //算出来了左边的地址了，这个地址存在了a5了
            //但是怎么感觉这个a5可能被用到，为了保险我挪一下地方
            buf.AddInst("\tmv\t\ta3, a5\n");
            //也就是说a3是要被赋值的东西的位置
        }
    }
    

}

void analysis::enterPrimary(CACTParser::PrimaryContext *ctx) {}
void analysis::exitPrimary(CACTParser::PrimaryContext *ctx)
{
    // ( exp )
    ////std::cout<<"wuhu" <<std::endl;
    if (ctx->primaryExp()->exp() != nullptr) {
        ctx->cls = ctx->primaryExp()->exp()->cls;
        ctx->name = ctx->primaryExp()->exp()->name;
    }
    // lVal
    else if (ctx->primaryExp()->lVal() != nullptr) {
        ctx->cls = ctx->primaryExp()->lVal()->cls;
        ctx->name = ctx->primaryExp()->lVal()->name;
    }
    // bool
    else if (ctx->getText() == "true" || ctx->getText() == "false") {
        std::string temp = sym_table.gen_temp_var(ctx->getStart()->getLine(), CLASS_BOOLEAN, sym_table);
        ctx->cls = CLASS_BOOLEAN;
        ctx->name = temp;
        Var *tmp_info = sym_table.lookup_var(temp);
        int value = (ctx->getText() == "true") ? 1 : 0;
    }
    // number
    else {
        //数值，给赋值给name，要串类型……
        int num_cls = ctx->primaryExp()->getStart()->getType();
        std::string literal_val = ctx->getText();
        std::string value_str;
        int int_val;
        float float_val;
        double double_val;

        if (num_cls == CACTLexer::IntConst) {
            ctx->cls = CLASS_INT;
            int_val = std::stoi(literal_val);
            value_str = std::to_string(int_val);
        }
        else if (num_cls == CACTLexer::FloatConst) {
            ctx->cls = CLASS_FLOAT;
            float_val = std::stof(literal_val.substr(0, literal_val.length() - 1));
            value_str = std::to_string(float_val);
        }
        else if (num_cls == CACTLexer::DoubleConst) {
            ctx->cls = CLASS_DOUBLE;
            double_val = std::stod(literal_val);
            value_str = std::to_string(double_val);
        }

        ////std::cout << "hey" << value_str <<std::endl;
        ctx->name = value_str;

        Var *tmp_info = sym_table.lookup_var(ctx->name);
    }
}

void analysis::enterUnary(CACTParser::UnaryContext *ctx) {}
void analysis::exitUnary(CACTParser::UnaryContext *ctx)
{
    if (ctx->op->getType() == CACTLexer::NOT) {
        if (ctx->unaryExp()->cls != CLASS_BOOLEAN)
            //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: wrong type argument to unary exclamation mark\n");
        ctx->cls = CLASS_BOOLEAN;
        Var *tmp_info = sym_table.lookup_var(ctx->name);
        std::string child_name = ctx->unaryExp()->name;
        Var *child_info = sym_table.lookup_var(child_name);
    }
    else if (ctx->op->getType() == CACTLexer::ADD) {
        if (ctx->cls == CLASS_BOOLEAN)
            //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: wrong type argument to unary '+' mark\n");
        ctx->cls = ctx->unaryExp()->cls;
        ctx->name = ctx->unaryExp()->name;
    }
    else if (ctx->op->getType() == CACTLexer::SUB) {
        if (ctx->cls == CLASS_BOOLEAN)
            //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: wrong type argument to unary '-' mark\n");
        ctx->cls = ctx->unaryExp()->cls;
        Var *tmp_info = sym_table.lookup_var(ctx->name);
        std::string child_name = ctx->unaryExp()->name;
        Var *child_info = sym_table.lookup_var(child_name);
    }
}

void analysis::enterMulExp(CACTParser::MulExpContext *ctx) {
    //printf("debug\n");
}
void analysis::exitMulExp(CACTParser::MulExpContext *ctx)
{
    // unaryexp
    if (!ctx->mulExp()) {
        ctx->cls = ctx->unaryExp()->cls;
        ctx->name = (ctx->cls == CLASS_VOID) ? "NULL" : ctx->unaryExp()->name;
    }
    // mulExp (MUL | DIV | MOD) unaryExp
    else {
        //if (ctx->mulExp()->cls != ctx->unaryExp()->cls) //类型检查
            //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: wrong type to binary * or / or %\n");
        auto op1 = ctx->mulExp()->name;
        auto op2 = ctx->unaryExp()->name;
        Var *op1_info = sym_table.lookup_var(op1);
        Var *op2_info = sym_table.lookup_var(op2);
        
        if(op1_info != nullptr){
            if(op2_info != nullptr){
                //参量 OP 参量
                ctx->elemwise = false;
                //传递一下cls
                ctx->cls = op1_info->cls;
                //产生这一步的名字
                ctx->name = sym_table.gen_temp_var(ctx->getStart()->getLine(), ctx->cls, sym_table);
                
                //其余类型
            }
            else{
                //参量 OP 数
                ctx->cls = op1_info->cls;
                //产生这一步的名字
                ctx->name = sym_table.gen_temp_var(ctx->getStart()->getLine(), ctx->cls, sym_table);
            }
        }
        else{
            // 我觉得不会有数OP数这种脑残代码，所以，这里是数OP参量
            ctx->cls = op2_info->cls;
            //产生这一步的名字
            ctx->name = sym_table.gen_temp_var(ctx->getStart()->getLine(), ctx->cls, sym_table);
        }


        //代码生成
        //其他类型
        if(ctx->cls == CLASS_INT){
            //取op1，放a6
            if(op1_info != nullptr){
                //参量
                buf.AddInst("\tlw\t\ta6, -");
                buf.AddInst(std::to_string(op1_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            else{
                //数
                buf.AddInst("\tli\t\ta6, ");
                buf.AddInst(ctx->mulExp()->getText());
                buf.AddInst("\n");
            }

            //取op2，放a7
            if(op2_info != nullptr){
                //参量
                buf.AddInst("\tlw\t\ta7, -");
                buf.AddInst(std::to_string(op2_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            else{
                //数
                buf.AddInst("\tli\t\ta7, ");
                buf.AddInst(ctx->unaryExp()->getText());
                buf.AddInst("\n");
            }

            //运算并返回结果
            //std::cout << "op: " << ctx->op->getText() << std::endl;
            if(ctx->op->getText() == "*"){
                buf.AddInst("\tmul\t\ta6, a6, a7\n");
            }
            if(ctx->op->getText() == "/"){
                buf.AddInst("\tdiv\t\ta6, a6, a7\n");
            }
            buf.AddInst("\tsw\t\ta6, -");
            buf.AddInst(std::to_string(sym_table.lookup_var(ctx->name)->offset + 16));
            buf.AddInst("(s0)\n");
        }
    }
    
}

void analysis::enterAddExp(CACTParser::AddExpContext *ctx) {
    //printf("debug\n");
}
void analysis::exitAddExp(CACTParser::AddExpContext *ctx)
{
    // mulexp
    ////std::cout << "add term content: " << ctx->getText().c_str() << std::endl;
    if (!ctx->addExp()) {
        ctx->cls = ctx->mulExp()->cls;
        ctx->name = ctx->mulExp()->name;
        ctx->elemwise = ctx->mulExp()->elemwise;
    ////std::cout<<"name: " << ctx->name << std::endl;
    }
    // addExp (ADD | SUB) mulExp
    else {
        //if (ctx->addExp()->cls != ctx->mulExp()->cls) //类型检查
            //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: wrong type to binary + or -\n");
        //printf(" %d vs %d \n", ctx->addExp()->cls,ctx->mulExp()->cls);
        auto op1 = ctx->addExp()->name;
        auto op2 = ctx->mulExp()->name;
        Var *op1_info = sym_table.lookup_var(op1);
        Var *op2_info = sym_table.lookup_var(op2);
        
        if(op1_info != nullptr){
            if(op2_info != nullptr){
                //参量 OP 参量
                ctx->elemwise = false;
                //传递一下cls
                ctx->cls = op1_info->cls;
                //产生这一步的名字
                ctx->name = sym_table.gen_temp_var(ctx->getStart()->getLine(), ctx->cls, sym_table);
                
                //其余类型
            }
            else{
                //参量 OP 数
                ctx->cls = op1_info->cls;
                //产生这一步的名字
                ctx->name = sym_table.gen_temp_var(ctx->getStart()->getLine(), ctx->cls, sym_table);
            }
        }
        else{
            // 我觉得不会有数OP数这种脑残代码，所以，这里是数OP参量
            ctx->cls = op2_info->cls;
            //产生这一步的名字
            ctx->name = sym_table.gen_temp_var(ctx->getStart()->getLine(), ctx->cls, sym_table);
        }


        //代码生成
        //其他类型
        //std::cout << "gen code" << std::endl;
        if(ctx->cls == CLASS_INT){
            //std::cout << "cope with op1" << std::endl;
            //取op1，放a6
            if(op1_info != nullptr){
                //参量
                //std::cout<<"op1 is var" << std::endl;
                buf.AddInst("\tlw\t\ta6, -");
                buf.AddInst(std::to_string(op1_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            else{
                //数
                //std::cout<<"op1 is num" << std::endl;
                buf.AddInst("\tli\t\ta6, ");
                buf.AddInst(ctx->addExp()->getText());
                buf.AddInst("\n");
            }

            //取op2，放a7
            if(op2_info != nullptr){
                //参量
                buf.AddInst("\tlw\t\ta7, -");
                buf.AddInst(std::to_string(op2_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            else{
                //数
                buf.AddInst("\tli\t\ta7, ");
                buf.AddInst(ctx->mulExp()->getText());
                buf.AddInst("\n");
            }

            //运算并返回结果
            ////std::cout << "op: " << ctx->op->getText() << std::endl;
            if(ctx->op->getText() == "+")
                buf.AddInst("\tadd\t\ta6, a6, a7\n");
            if(ctx->op->getText() == "-")
                buf.AddInst("\tsub\t\ta6, a6, a7\n");
            buf.AddInst("\tsw\t\ta6, -");
            buf.AddInst(std::to_string(sym_table.lookup_var(ctx->name)->offset + 16));
            buf.AddInst("(s0)\n");
        }
    }
}

void analysis::enterRelExp(CACTParser::RelExpContext *ctx) {
    //printf("deb111111111ug\n");
}
void analysis::exitRelExp(CACTParser::RelExpContext *ctx)
{
    // boolconst    
    ////std::cout << "rel term content: " << ctx->getText().c_str() << std::endl;
    if (ctx->boolConst()) {
        ctx->cls = CLASS_BOOLEAN;
        ctx->value = ctx->boolConst()->getText() == "true" ? 1 : 0;
    }
    // relExp op addExp
    else if (ctx->relExp()) {
        // op1 op op2
        std::string op1 = ctx->relExp()->name;
        std::string op2 = ctx->addExp()->name;
        //std::cout << "comparrrrrr " << op1 << std::endl;
        //std::cout << "comparrrrrr " << op2 << std::endl;
        Var *op1_info = sym_table.lookup_var(op1);
        Var *op2_info = sym_table.lookup_var(op2);
        // 用b的指令，然后就是，对了跳哪，错了调哪

        //产生临时变量
        ctx->cls = CLASS_BOOLEAN;
        ctx->name = sym_table.gen_temp_var(ctx->getStart()->getLine(), ctx->cls, sym_table);
        //找一下左右两边的表达式的值

        //代码生成
        //其他类型
        //std::cout << "gen code" << std::endl;
        if(1){
            //std::cout << "cope with op1" << std::endl;
            //取op1，放a6
            if(op1_info != nullptr){
                //参量
                //std::cout<<"op1 is var" << std::endl;
                buf.AddInst("\tlw\t\ta6, -");
                buf.AddInst(std::to_string(op1_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            else{
                //数
                //std::cout<<"op1 is num" << std::endl;
                buf.AddInst("\tli\t\ta6, ");
                buf.AddInst(ctx->relExp()->getText());
                buf.AddInst("\n");
            }

            //取op2，放a7
            if(op2_info != nullptr){
                //参量
                buf.AddInst("\tlw\t\ta7, -");
                buf.AddInst(std::to_string(op2_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            else{
                //数
                buf.AddInst("\tli\t\ta7, ");
                buf.AddInst(ctx->addExp()->getText());
                buf.AddInst("\n");
            }

            //运算并决定怎么跳
            if(ctx->op->getText() == "<="){
                //op2 < op1 跳
                buf.AddInst("\tblt\t\ta7, a6, .L");
                buf.AddInst(std::to_string(ctx->LA));
                buf.AddInst("\n");
            }
            if(ctx->op->getText() == "<"){
                //op1 >= op2 跳
                buf.AddInst("\tbge\t\ta6, a7, .L");
                buf.AddInst(std::to_string(ctx->LA));
                buf.AddInst("\n");
            }
            if(ctx->op->getText() == ">="){
                //op1 < op2 跳
                buf.AddInst("\tblt\t\ta6, a7, .L");
                buf.AddInst(std::to_string(ctx->LA));
                buf.AddInst("\n");
            }
            if(ctx->op->getText() == ">"){
                //op2 >= op1 跳
                buf.AddInst("\tbge\t\ta7, a6, .L");
                buf.AddInst(std::to_string(ctx->LA));
                buf.AddInst("\n");
            }
        }
    }
    // addExp
    else {
        ctx->cls = ctx->addExp()->cls;
        ctx->name = ctx->addExp()->name;
    }
}

void analysis::enterEqExp(CACTParser::EqExpContext *ctx) {
    if(ctx->eqExp()){
        //std::cout<<"consider later\n";
        ctx->relExp()->LA = ctx->LA;
        ctx->relExp()->LB = ctx->LB;
        ctx->eqExp()->LA = ctx->LA;
        ctx->eqExp()->LB = ctx->LB;
    }
    else{
        //std::cout<<"pass\n";
        ctx->relExp()->LA = ctx->LA;
        ctx->relExp()->LB = ctx->LB;
    }
}
void analysis::exitEqExp(CACTParser::EqExpContext *ctx)
{
    // eqExp (EQUAL | NOTEQUAL) relExp
    ////std::cout << "in eq term content: " << ctx->getText().c_str() << std::endl;
    if (ctx->eqExp()) {
        std::string op1 = ctx->eqExp()->name;
        std::string op2 = ctx->relExp()->name;
        Var *op1_info = sym_table.lookup_var(op1);
        Var *op2_info = sym_table.lookup_var(op2);


        ctx->cls = CLASS_BOOLEAN;
        ctx->name = sym_table.gen_temp_var(ctx->getStart()->getLine(), ctx->cls, sym_table);
        //这里也得写代码

        //代码生成
        //其他类型
        //std::cout << "gen code" << std::endl;
        if(1){
            //std::cout << "cope with op1" << std::endl;
            //取op1，放a6
            if(op1_info != nullptr){
                //参量
                //std::cout<<"op1 is var" << std::endl;
                buf.AddInst("\tlw\t\ta6, -");
                buf.AddInst(std::to_string(op1_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            else{
                //数
                //std::cout<<"op1 is num" << std::endl;
                buf.AddInst("\tli\t\ta6, ");
                buf.AddInst(ctx->eqExp()->getText());
                buf.AddInst("\n");
            }

            //取op2，放a7
            if(op2_info != nullptr){
                //参量
                buf.AddInst("\tlw\t\ta7, -");
                buf.AddInst(std::to_string(op2_info->offset + 16));
                buf.AddInst("(s0)\n");
            }
            else{
                //数
                buf.AddInst("\tli\t\ta7, ");
                buf.AddInst(ctx->relExp()->getText());
                buf.AddInst("\n");
            }

            //运算并决定怎么跳
            if(ctx->op->getText() == "=="){
                //op2 < op1 跳
                buf.AddInst("\tbne\t\ta7, a6, .L");
                buf.AddInst(std::to_string(ctx->LA));
                buf.AddInst("\n");
            }
            if(ctx->op->getText() == "!="){
                //op1 >= op2 跳
                buf.AddInst("\tbeq\t\ta6, a7, .L");
                buf.AddInst(std::to_string(ctx->LA));
                buf.AddInst("\n");
            }
        }

    }
    // relExp
    else {
        ctx->cls = ctx->relExp()->cls;
        ctx->name = ctx->relExp()->name;
    }
    ////std::cout << "out eq term content: " << ctx->getText().c_str() << std::endl;
}

void analysis::enterLAndExp(CACTParser::LAndExpContext *ctx) {
    if(ctx->lAndExp()){
        //std::cout<<"consider later\n";
        ctx->eqExp()->LA = ctx->LA;
        ctx->eqExp()->LB = ctx->LB;
        ctx->lAndExp()->LA = ctx->LA;
        ctx->lAndExp()->LB = ctx->LB;
    }
    else{
        //std::cout<<"pass\n";
        ctx->eqExp()->LA = ctx->LA;
        ctx->eqExp()->LB = ctx->LB;
    }
}
void analysis::exitLAndExp(CACTParser::LAndExpContext *ctx)
{
    //lAndExp AND eqExp
    if (ctx->lAndExp()) {
        std::string op1 = ctx->lAndExp()->name;
        std::string op2 = ctx->eqExp()->name;

        if(op1 != "" && op2 != "") {
            Var *op1_info = sym_table.lookup_var(op1);
            Var *op2_info = sym_table.lookup_var(op2);

            if (op1_info->cls != op2_info->cls) {
                //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: wrong type to &7\n");
            }
            if (op1_info->type == TYPE_ARRAY || op1_info->type == TYPE_CONST_ARRAY ||
                op2_info->type == TYPE_ARRAY || op2_info->type == TYPE_CONST_ARRAY) {
                //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid operands to &&\n");
            }
        }
        ctx->cls = CLASS_BOOLEAN;
    }
    // eqExp
    else {
        ctx->cls = ctx->eqExp()->cls;
        ctx->name = ctx->eqExp()->name; 
    }
}

void analysis::enterLOrExp(CACTParser::LOrExpContext *ctx) {
    if(ctx->lOrExp()){
        //std::cout<<"consider later\n";
        ctx->lAndExp()->LA = ctx->LA;
        ctx->lAndExp()->LB = ctx->LB;
        ctx->lOrExp()->LA = ctx->LA;
        ctx->lOrExp()->LB = ctx->LB;
    }
    else{
        //std::cout<<"pass\n";
        ctx->lAndExp()->LA = ctx->LA;
        ctx->lAndExp()->LB = ctx->LB;
    }
}
void analysis::exitLOrExp(CACTParser::LOrExpContext *ctx)
{
    // lOrExp OR lAndExp
    if (ctx->lOrExp()) {
        std::string op1 = ctx->lOrExp()->name;
        std::string op2 = ctx->lAndExp()->name;
        if(op1 != "" && op2 != "") {
        Var *op1_info = sym_table.lookup_var(op1);
        Var *op2_info = sym_table.lookup_var(op2);

        if (op1_info->cls != op2_info->cls) {
            //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: wrong type to ||\n");
        }
        if (op1_info->type == TYPE_ARRAY || op1_info->type == TYPE_CONST_ARRAY ||
            op2_info->type == TYPE_ARRAY || op2_info->type == TYPE_CONST_ARRAY) {
            //throw std::runtime_error("line " + std::to_string(ctx->getStart()->getLine()) + ": error: invalid operands to ||\n");
        }
        }
        ctx->cls = CLASS_BOOLEAN;
    }
    // lAndExp
    else {
        ctx->cls = ctx->lAndExp()->cls;
        ctx->name = ctx->lAndExp()->name;
    }
}

void analysis::enterOutsideDecl(CACTParser::OutsideDeclContext *ctx) {
    ctx->decl()->where = OUTSIDE;
}
void analysis::exitOutsideDecl(CACTParser::OutsideDeclContext *ctx) {
    ;
}

void analysis::enterDecl(CACTParser::DeclContext *ctx) {
    ctx->varDecl()->where = ctx->where;
}
void analysis::exitDecl(CACTParser::DeclContext *ctx) {
    ;
}