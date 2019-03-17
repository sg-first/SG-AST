#pragma once
#include <map>
#include "nodetype.h"

class Scope //作用是记录不同语句块内函数及变量名到实体的映射，不是运行时栈
{
private:
    BasicNode* topASTNode=nullptr; //这是完全包含本域所有实体引用的根节点，否则会释放外部需要使用的实体
public:
    ~Scope();
    map<string,Variable*> variableList;
    map<string,Function*> functionList;
    vector<Scope*> sonScope;
    Scope* fatherScope; //因为可能顺着作用域向上层找变量，所以需要上级节点的指针
    Scope(Scope* fatherScope=nullptr):fatherScope(fatherScope){}

    Variable *addVariable(string name);
    void addVariable(string name,Variable* var);
    void addFunction(string name,Function* fun);
    bool haveVariable(string name) {return this->variableList[name]!=0;}
    bool haveFunction(string name) {return this->functionList[name]!=0;}
    //直接删除实体，在已经将所有实体引用在树中清空时使用（目前还没看到有什么用）
    //void deleteVariable(string name);
    //void deleteFunction(string name);
    //删除实体指针在域中的存储，在已经将所有实体引用在树中清空时使用且已经delete时使用
    void deleteVariable(Variable* var);
    void deleteFunction(Function* fun);
    void settopASTNode(BasicNode* topnode) {this->topASTNode=topnode;}
};
