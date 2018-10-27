#include "scope.h"

Scope::~Scope()
{
    delete this->topASTNode;
    for(auto i:this->functionList)
        delete i.second;
    for(auto i:this->variableList)
        delete i.second;
    for(Scope* i:this->sonScope)
        delete i;
}

void Scope::addVariable(string name)
{
    Variable* var=new Variable();
    #ifdef READABLEcodegen
    var->NAME=name;
    #endif
    this->variableList[name]=var;
}

void Scope::addVariable(string name, Variable *var)
{
    #ifdef READABLEcodegen
    var->NAME=name;
    #endif
    this->variableList[name]=var;
}

void Scope::addFunction(string name, Function *fun)
{
    #ifdef READABLEcodegen
    fun->NAME=name;
    #endif
    this->functionList[name]=fun;
}

/*void Scope::deleteFunction(string name)
{
    delete this->functionList[name];
    this->functionList.erase(name);
}

void Scope::deleteVariable(string name)
{
    delete this->variableList[name];
    this->variableList.erase(name);
}*/

void Scope::deleteVariable(Variable *var)
{
    for(auto p:this->variableList)
    {
        if(p.second==var)
            this->variableList.erase(p.first);
    }
}

void Scope::deleteFunction(Function *fun)
{
    for(auto p:this->functionList)
    {
        if(p.second==fun)
            this->functionList.erase(p.first);
    }
}
