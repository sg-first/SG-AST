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
    if(fatherScope)
        for(auto i = fatherScope->sonScope.begin(); i < fatherScope->sonScope.end(); i++)
        {
            if(*i == this)
                fatherScope->sonScope.erase(i);
        }
}

Variable* Scope::addVariable(string name)
{
    Variable* var=new Variable();
#ifdef READABLEGEN
    var->NAME=name;
#endif
    this->variableList[name]=var;
    return var;
}

void Scope::addVariable(string name, Variable *var)
{
#ifdef READABLEGEN
    var->NAME=name;
#endif
    this->variableList[name]=var;
}

void Scope::addFunction(string name, Function *fun)
{
#ifdef READABLEGEN
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

Variable* Scope::findVariable(string name,bool thisScope)
{
    Variable* result=this->variableList[name];

    if(result==0)
        result=nullptr;

    if(thisScope)
        return result;
    else
    {
        if(result==nullptr)
            return nullptr;
        else
            return this->fatherScope->findVariable(name,false);
    }
}

Function* Scope::findFunction(string name,bool thisScope)
{
    Function* result=this->functionList[name];

    if(result==0)
        result=nullptr;

    if(thisScope)
        return result;
    else
    {
        if(result==nullptr)
            return nullptr;
        else
            return this->fatherScope->findFunction(name,false);
    }
}
