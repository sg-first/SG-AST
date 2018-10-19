#pragma once
#include <map>
#include "nodetype.h"

class Scope //作用是记录不同语句块内函数及变量名到实体的映射，不是运行时栈
{
public:
    ~Scope();
    map<string,Variable*> variableList;
    map<string,Function*> functionList;
    vector<Scope*> sonScope;

    void addValue(string name) {this->variableList[name]=new Variable();}
    void addValue(string name,Variable* var) {this->variableList[name]=var;}
    void addFunction(string name,Function* fun) {this->functionList[name]=fun;}
};
