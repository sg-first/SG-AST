#pragma once
#include "marco.h"

class Excep {

};

class unassignedEvalExcep : public Excep {

};

class unassignedAssignedExcep : public Excep {

};

class cannotAssignedExcep : public Excep { //这个或许应该加上类型存储

};

class cannotEvaledExcep : public Excep {
    
};

class VarRefUnbindExcep : public Excep {

};

class argumentNumExceedingExcep : public Excep {

};

class parameterNumExceedingExcep : public Excep {

};

enum callCheckMismatchType{NumberMismatch,TypeMisMatch};
class callCheckMismatchExcep : public Excep
{
private:
    int type;
public:
    callCheckMismatchExcep(int type):type(type) {}
    int getType() {return this->type;}
};

class addSonExcep : public Excep
{
private:
    int type;
public:
    addSonExcep(int type):type(type) {}
    int getType() {return this->type;}
};
