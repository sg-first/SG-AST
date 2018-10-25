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
    Scope* fatherScope;
    Scope(Scope* fatherScope=nullptr):fatherScope(fatherScope){}

    void addValue(string name);
    void addValue(string name,Variable* var);
    void addFunction(string name,Function* fun);
};
