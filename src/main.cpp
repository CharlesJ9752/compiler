#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <fstream>

#include "antlr4-runtime.h"
#include "tree/ErrorNode.h"

#include "../grammar/CACTLexer.h"
#include "../grammar/CACTParser.h"
#include "../grammar/CACTBaseListener.h"

#include "analysis.h"

using namespace antlr4;


int main(int argc, const char* argv[]) {
  std::ifstream stream;
  stream.open(argv[argc - 1]);
  std::cout << "file path: " << argv[argc - 1] << std::endl;
  
  //获取输出路径
  int length = strlen(argv[1]);
  char* out_path = (char *)malloc(sizeof(char) * length);
  strcpy(out_path, argv[1]);
  //abaaba.cact -> abaaba.s
  out_path[length - 3] = 0;
  out_path[length - 4] = 's';
  //std::cout << "output path: " << out_path << std::endl;

  
  analysis listener;
  listener.outpath = out_path;
  listener.outfile.open(out_path, std::ios_base::trunc);
  //↑写文件的类

  //listener.outfile.close();
  

  ANTLRInputStream   input(stream);
  CACTLexer         lexer(&input);
  CommonTokenStream  tokens(&lexer);
  CACTParser        parser(&tokens);

  tree::ParseTree *tree = parser.compUnit();

  if (lexer.getNumberOfSyntaxErrors() > 0)
  {
      std::cout << lexer.getNumberOfSyntaxErrors() << " syntax errors reported by lexer.  error!" << std::endl;
      exit(1);
  }
  if (parser.getNumberOfSyntaxErrors() > 0)
  {
      std::cout << parser.getNumberOfSyntaxErrors() << " syntax errors reported by parser. error!" << std::endl;
      exit(1);
  }

	tree::ParseTreeWalker walker;
	try {
        walker.walk(&listener, tree);
  }
  catch (const std::exception &e) {
    std::cerr << e.what();
		std::cout << "Semantic Error" << std::endl;
    exit(1);
  }
  std::cout << "Compilation Pass" << std::endl;
  //Analysis visitor;
  //visitor.visit( parser.r() );
  //test ssh
  return 0;
}
