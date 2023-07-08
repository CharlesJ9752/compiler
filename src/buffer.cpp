#include"buffer.h"
#include<iostream>
#include<fstream>

void buffer::AddInst(std::string Inst){
    if(this->next != nullptr){
        this->next->AddInst(Inst);
    }
    else{
        this->Inst = Inst;
        this->next = new(buffer);
    }
}

void buffer::PrintBuf(){
    if(this == nullptr)
        return;
    else{
        std::cout << this->Inst;
        this->next->PrintBuf();
    }
}

void buffer::WriteBuf(std::string outpath){
    if(this == nullptr)
        return;
    else{
        std::ofstream outfile;
        outfile.open(outpath, std::ios_base::app);
        //std::cout <<"coping with string: " << this->Inst <<std::endl;
        outfile << this->Inst;
        outfile.close();
        this->next->WriteBuf(outpath);
    }
}

void buffer::Clear(){
    this->Inst = "";
    this->next = nullptr;
}