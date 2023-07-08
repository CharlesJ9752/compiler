#include<string>

class buffer
{
public:
    std::string Inst = "";
    buffer* next = nullptr;
    void AddInst(std::string Inst);
    void PrintBuf();
    void WriteBuf(std::string outpath);
    void Clear();
};