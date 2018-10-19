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
