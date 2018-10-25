#include "scope.h"

Scope::~Scope()
{
    for(auto i:this->functionList)
        delete i.second;
    for(auto i:this->variableList)
        delete i.second;
    for(Scope* i:this->sonScope)
        delete i;
}

void Scope::addValue(string name)
{
    Variable* var=new Variable();
    #ifdef parserdebug
    var->NAME=name;
    #endif
    this->variableList[name]=var;
}

void Scope::addValue(string name, Variable *var)
{
    #ifdef parserdebug
    var->NAME=name;
    #endif
    this->variableList[name]=var;
}

void Scope::addFunction(string name, Function *fun)
{
    #ifdef parserdebug
    fun->NAME=name;
    #endif
    this->functionList[name]=fun;
}
