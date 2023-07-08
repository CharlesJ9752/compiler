#include "../grammar/CACTBaseListener.h"
#include "../grammar/CACTLexer.h"
#include "symboltable.h"

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include"buffer.h"

#define OUTSIDE 403
#define INSIDE 402

const std::string VOID = "";


class analysis : public CACTBaseListener
{
public:
	SymbolTable sym_table;
	buffer buf;
    std::stack<std::string> u_stack;        //type stack
	std::vector<std::string> label_stack;	//label stack

	std::ofstream outfile;
	std::string outpath;
	int LableIndex = 0;
	int WhileLable[5][2];
	int WhileNum = 0;

	// if语句用到的label
	antlr4::tree::ParseTreeProperty<std::vector<std::string>> if_labels;

	// while语句用到的label
	antlr4::tree::ParseTreeProperty<std::vector<std::string>> while_labels;

public:

	void enterCompUnit(CACTParser::CompUnitContext * /*ctx*/) override;
	void exitCompUnit(CACTParser::CompUnitContext * /*ctx*/) override;

	// const decl
	void enterConstDecl(CACTParser::ConstDeclContext * /*ctx*/) override;
	void exitConstDecl(CACTParser::ConstDeclContext * /*ctx*/) override;

	void enterConstDefVal(CACTParser::ConstDefValContext * /*ctx*/) override;
	void exitConstDefVal(CACTParser::ConstDefValContext * /*ctx*/) override;

	//lzr feat: 常数数组初始化判定
	void enterConstInitArray(CACTParser::ConstDefArrayContext *ctx);
	void exitConstInitArray(CACTParser::ConstDefArrayContext *ctx);

	void enterConstDefArray(CACTParser::ConstDefArrayContext * /*ctx*/) override;
	void exitConstDefArray(CACTParser::ConstDefArrayContext * /*ctx*/) override;

	// var decl
	void enterVarDecl(CACTParser::VarDeclContext * /*ctx*/) override;
	void exitVarDecl(CACTParser::VarDeclContext * /*ctx*/) override;

	void enterDefVal(CACTParser::DefValContext * /*ctx*/) override;
	void exitDefVal(CACTParser::DefValContext * /*ctx*/) override;

	void enterDefArray(CACTParser::DefArrayContext * /*ctx*/) override;
	void exitDefArray(CACTParser::DefArrayContext * /*ctx*/) override;

	void enterDefInitVal(CACTParser::DefInitValContext * /*ctx*/) override;
	void exitDefInitVal(CACTParser::DefInitValContext * /*ctx*/) override;

	void enterDefInitArray(CACTParser::DefInitArrayContext * /*ctx*/) override;
	void exitDefInitArray(CACTParser::DefInitArrayContext * /*ctx*/) override;

	// func def
	void enterFuncDef(CACTParser::FuncDefContext * /*ctx*/) override;
	void exitFuncDef(CACTParser::FuncDefContext * /*ctx*/) override;

	// block
	void enterBlock(CACTParser::BlockContext * /*ctx*/) override;
	void exitBlock(CACTParser::BlockContext * /*ctx*/) override;

	// blockItem
	void enterBlockItem(CACTParser::BlockItemContext * /*ctx*/) override;
	void exitBlockItem(CACTParser::BlockItemContext * /*ctx*/) override;

	// function
	void enterFuncall(CACTParser::FuncallContext * /*ctx*/) override;
	void exitFuncall(CACTParser::FuncallContext * /*ctx*/) override;

    void enterReturnStmt(CACTParser::ReturnStmtContext * /*ctx*/) override;
	void exitReturnStmt(CACTParser::ReturnStmtContext * /*ctx*/) override;
	
	// condition
	void enterCond(CACTParser::CondContext * /*ctx*/) override;
	void exitCond(CACTParser::CondContext * /*ctx*/) override;

	void enterIfCond(CACTParser::IfCondContext * /*ctx*/) override;
	void exitIfCond(CACTParser::IfCondContext * /*ctx*/) override;

	void enterWhileCond(CACTParser::WhileCondContext * /*ctx*/) override;
	void exitWhileCond(CACTParser::WhileCondContext * /*ctx*/) override;

	void enterExprStmt(CACTParser::ExprStmtContext * /*ctx*/) override;
	void exitExprStmt(CACTParser::ExprStmtContext * /*ctx*/) override;

	void enterBlockStmt(CACTParser::BlockStmtContext * /*ctx*/) override;
	void exitBlockStmt(CACTParser::BlockStmtContext * /*ctx*/) override;

	void enterIfStmt(CACTParser::IfStmtContext * /*ctx*/) override;
	void exitIfStmt(CACTParser::IfStmtContext * /*ctx*/) override;

	void enterIfElseStmt(CACTParser::IfElseStmtContext * /*ctx*/) override;
	void exitIfElseStmt(CACTParser::IfElseStmtContext * /*ctx*/) override;

	void enterElseStmt(CACTParser::ElseStmtContext * /*ctx*/) override;
	void exitElseStmt(CACTParser::ElseStmtContext * /*ctx*/) override;

	void enterWhileStmt(CACTParser::WhileStmtContext * /*ctx*/) override;
	void exitWhileStmt(CACTParser::WhileStmtContext * /*ctx*/) override;

	void enterBreakStmt(CACTParser::BreakStmtContext * /*ctx*/) override;
  	void exitBreakStmt(CACTParser::BreakStmtContext * /*ctx*/) override;

 	void enterContinueStmt(CACTParser::ContinueStmtContext * /*ctx*/) override;
	void exitContinueStmt(CACTParser::ContinueStmtContext * /*ctx*/) override;

	// expression
	void enterAssignStmt(CACTParser::AssignStmtContext * /*ctx*/) override;
	void exitAssignStmt(CACTParser::AssignStmtContext * /*ctx*/) override;

	void enterExp(CACTParser::ExpContext * /*ctx*/) override;
	void exitExp(CACTParser::ExpContext * /*ctx*/) override;

	void enterLVal(CACTParser::LValContext * /*ctx*/) override;
	void exitLVal(CACTParser::LValContext * /*ctx*/) override;

	void enterPrimary(CACTParser::PrimaryContext * /*ctx*/) override;
	void exitPrimary(CACTParser::PrimaryContext * /*ctx*/) override;

	void enterUnary(CACTParser::UnaryContext * /*ctx*/) override;
	void exitUnary(CACTParser::UnaryContext * /*ctx*/) override;

	void enterMulExp(CACTParser::MulExpContext * /*ctx*/) override;
	void exitMulExp(CACTParser::MulExpContext * /*ctx*/) override;

	void enterAddExp(CACTParser::AddExpContext * /*ctx*/) override;
	void exitAddExp(CACTParser::AddExpContext * /*ctx*/) override;

	void enterRelExp(CACTParser::RelExpContext * /*ctx*/) override;
	void exitRelExp(CACTParser::RelExpContext * /*ctx*/) override;

	void enterEqExp(CACTParser::EqExpContext * /*ctx*/) override;
	void exitEqExp(CACTParser::EqExpContext * /*ctx*/) override;

	void enterLAndExp(CACTParser::LAndExpContext * /*ctx*/) override;
	void exitLAndExp(CACTParser::LAndExpContext * /*ctx*/) override;

	void enterLOrExp(CACTParser::LOrExpContext * /*ctx*/) override;
	void exitLOrExp(CACTParser::LOrExpContext * /*ctx*/) override;

	void enterOutsideDecl(CACTParser::OutsideDeclContext * ) override;
	void exitOutsideDecl(CACTParser::OutsideDeclContext * ) override;

	void enterDecl(CACTParser::DeclContext * ) override;
	void exitDecl(CACTParser::DeclContext * ) override;
};