#include "nodetype.h"

bool isLiteral(BasicNode* node) //是否为字面量，添加新的字面量要进行修改
{
    return (node->getType()==Num||node->getType()==String);
}

bool isNotAssignable(BasicNode* val) //是否不可赋值给变量，支持新的值类型要进行修改
{
    return (val->getType()==Pro||val->getType()==Fun);
    //fix:目前暂不支持函数指针，因为函数实体的变量表示还没设计好
}


BasicNode::~BasicNode()
{
    for(BasicNode* node:this->sonNode)
    {
        if(node->getType()!=Var) //这个随着域释放，不被连环析构
            delete node;
    }
}

BasicNode* copyVal(BasicNode* oriVal) //（值类型）拷贝
{
    //调用前应该对参数类型进行检查
    if(oriVal->getType()==Num)
        return new NumNode(dynamic_cast<NumNode*>(oriVal));
    if(oriVal->getType()==String)
        return new StringNode(dynamic_cast<StringNode*>(oriVal));
    //支持更多具拷贝构造函数类型（目前都是字面量）后还需要在此处进行添加
    return nullptr; //如果进行参数检查了不会走到这一步
}


VarNode::VarNode(int valtype)
{
    this->valtype=valtype;
    if(valtype!=-1)
        this->typeRestrictFlag=true;
}

VarNode::~VarNode()
{
    if((!this->isEmpty())&&this->ownershipFlag)
        delete this->val;
    //然后BasicNode析构
}

void VarNode::assignmentChecking(BasicNode *val)
{
    if(isNotAssignable(val))
        throw cannotAssignedExcep();
    if(this->typeRestrictFlag&&val->getType()!=this->valtype)
        throw assignedTypeMismatchExcep();
}

void VarNode::setVal(BasicNode* val)
{
    this->assignmentChecking(val);
    //warn:理论上讲按照目前的设计，变量不应作为具有所有权的值（因为所有权在运行时域），但在此暂不进行检查。如果进行检查，直接在此处添加
    this->valtype=val->getType();
    this->ownershipFlag=true;
    this->val=val;
}

void VarNode::setBorrowVal(BasicNode *val)
{
    this->assignmentChecking(val);
    this->valtype=val->getType();
    this->ownershipFlag=false;
    this->val=val;
}

void VarNode::setVarVal(VarNode *node)
{
    if(node->isEmpty())
        throw unassignedAssignedExcep();
    BasicNode* oriVal=node->eval();
    //目前策略为：字面量进行拷贝（有所有权），变量作为无所有权指针传递
    if(isLiteral(oriVal))
        this->setVal(copyVal(oriVal));
    if(oriVal->getType()==Var)
        this->setBorrowVal(oriVal);
    //fix:支持函数指针之后还需要在此处进行添加
}

BasicNode* VarNode::eval()
{
    if(this->isEmpty())
        throw unassignedEvalExcep();
    else
        return this->val;
    //注意，多级指针也只会解包一次。不过从返回值基本无法判断返回的是自身还是自身的变量指针值，所以先前需要getValType进行判断
}

void VarNode::clearVal()
{
    //调用前应进行是否为空的检查，否则有所有权的情况下会delete nullptr
    if(this->ownershipFlag)
    {
        delete this->val;
        this->val=nullptr;
    }
}


VarRefNode::VarRefNode(int valtype)
{
    this->valtype=valtype;
    if(valtype!=-1)
        this->typeRestrictFlag=true;
}

VarRefNode::~VarRefNode()
{
    if(this->ownershipFlag&&this->isbind()) //一般来讲应该不会出现在绑定（函数调用期间）就释放实体的情况
        delete this->val;
}

void VarRefNode::unbind()
{
    //调用前应进行是否为空的检查，否则有所有权的情况下会delete nullptr
    if(this->ownershipFlag)
        delete this->val;
}

BasicNode* VarRefNode::eval()
{
    if(!this->isbind())
        throw VarRefUnbindExcep();
    return this->val;
}

void VarRefNode::setVal(BasicNode* val)
{
    this->valtype=val->getType();
    this->ownershipFlag=true;
    this->val=val;
}

void VarRefNode::setBorrowVal(BasicNode *val)
{
    if(val->getType()==Var&&dynamic_cast<Variable*>(val)->isEmpty()) //按目前情况，传过来的只可能是Var，第一个条件有点多余
        throw unassignedAssignedExcep();
    this->valtype=val->getType();
    this->ownershipFlag=false;
    this->val=val;
}

void VarRefNode::assignmentChecking(BasicNode *val)
{
    if(isNotAssignable(val))
        throw cannotAssignedExcep();
    if(this->typeRestrictFlag&&val->getType()!=this->valtype)
        throw assignedTypeMismatchExcep();
}

void VarRefNode::bind(BasicNode *val)
{
    this->assignmentChecking(val);
    //目前策略为：字面量进行拷贝（有所有权），变量作为无所有权指针传递
    if(isLiteral(val))
        this->setVal(copyVal(val));
    if(val->getType()==Var)
        this->setBorrowVal(val);
    //fix:支持函数指针之后还需要在此处进行添加
}


void FunNode::addNode(BasicNode *node)
{
    if(!this->funEntity->isVLP()) //支持变长参数就先不进行参数个数检查
        if(this->sonNode.size()+1>this->funEntity->getParnum())
            throw parameterNumExceedingExcep();

    this->sonNode.push_back(node);
}

BasicNode* FunNode::eval()
{
    #ifdef PARTEVAL
    this->giveupEval=false;
    #endif

    if(this->funEntity==nullptr)
        throw string("funEntity is null");

    #ifdef PARTEVAL
    try
    {
    #endif
    return this->funEntity->eval(this->sonNode);
    }
    #ifdef PARTEVAL
    catch(callCheckMismatchExcep e) //因为未赋值变量未求值使得参数类型不匹配，放弃对这个函数求值
    {
        if(e.getType()==TypeMisMatch)
        {
            this->giveupEval=true;
            return this;
        }
        else
            throw e;
    }
    #endif
}


void recursionEval(BasicNode* &node)
{
    if(node->getType()==Pro) //按正常函数里面不要套Pro
        throw string("ProNode cannot be function's sonNode");
    else
    {
        if(isLiteral(node))
            return; //如果是字面量，自己就是求值结果，下面再重新赋值一次就重复了
        else
        {
            BasicNode* result;
            #ifdef PARTEVAL
            try
            {
            #endif
            result=node->eval();
            #ifdef PARTEVAL
            }
            catch(unassignedEvalExcep) //对未赋值变量求值，保持原样
            {result=node;}
            #endif

            #ifdef PARTEVAL
            if(node->getType()!=Var)
                if(!(node->getType()==Fun&&dynamic_cast<FunNode*>(node)->giveupEval)) //没有放弃求值
            #else
            if(node->getType()!=Var)
            #endif
                delete node;
            node=result; //节点的替换在这里（父节点）完成，子节点只需要返回即可
            //对于已经赋值的变量，整体过程是用值替代本身变量在AST中的位置，不过变量本身并没有被析构，因为变量的所有权在scope（后面可能还要访问）
            //对于未赋值的变量，求值结果是变量本身，AST没有变化
        }
    }
}

BasicNode* Function::eval(vector<BasicNode *> &sonNode)
{
    //对所有参数求值
    for(BasicNode* &node:sonNode)
        recursionEval(node);

    //函数求值
    if(this->iscanBE) //基础求值模式
    {
        if(this->canBEfun(sonNode)) //参数合法
            return this->BEfun(sonNode);
        else
            throw callCheckMismatchExcep(TypeMisMatch);
    }
    else //不能基础求值就是正常有函数体Pro的
    {
        this->bindArgument(sonNode); //子节点绑定到实参
        vector<BasicNode*>&funbody=this->pronode->sonNode;
        for(unsigned int i=0;i<funbody.size()-1;i++) //最后一个可能是返回值，先留着后面单独处理
        {
            recursionEval(funbody.at(i));
            if(funbody.at(i)->isRet())
            {
                this->unbindArgument();
                return funbody.at(i);
            }
        }
        //前面都不是返回值，最后一个是
        BasicNode* lastnode=funbody.at(funbody.size()-1);
        if(lastnode==nullptr)
        {
            this->unbindArgument();
            return nullptr;
        }
        else
        {
            recursionEval(lastnode);
            this->unbindArgument();
            return lastnode;
        }
    }
}

Function::~Function()
{
    delete this->pronode;
    for(VarReference* i:this->argumentList)
        delete i;
}

void Function::addArgument(VarReference *var)
{
    if(this->argumentList.size()+1>this->getParnum())
        throw argumentNumExceedingExcep();
    this->argumentList.push_back(var);
}

void Function::unbindArgument()
{
    for(VarReference* i:this->argumentList)
        i->unbind();
}

void Function::bindArgument(vector<BasicNode *> &sonNode)
{
    if(sonNode.size()!=this->argumentList.size())
        throw callCheckMismatchExcep(NumberMismatch);
    for(unsigned int i=0;i<this->argumentList.size();i++)
    {
        VarReference* formalpar=this->argumentList.at(i);
        formalpar->bind(sonNode.at(i));
    }
}
