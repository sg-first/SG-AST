#include "nodetype.h"

bool isNotAssignable(BasicNode* val) //warn:是否不可赋值给变量，支持新的节点类型要进行修改
{
    return (val->getType()==Pro||val->getType()==Fun||val->getType()==If||val->getType()==While);
    //fix:目前暂不支持函数指针，因为函数实体的变量表示还没设计好
}


BasicNode::~BasicNode()
{
    copyHelp::delTree(this);
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

void assignmentChecking(BasicNode *val,bool thisTypeRestrictFlag,nodeType thisvaltype)
{
    if(isNotAssignable(val))
        throw cannotAssignedExcep();
    if (thisTypeRestrictFlag && val->getType() != thisvaltype)
        throw callCheckMismatchExcep(TypeMisMatch);
}

void VarNode::setVal(BasicNode* val)
{
    assignmentChecking(val,this->typeRestrictFlag,this->getValType());
    this->clearVal();
    //warn:理论上讲按照目前的设计，变量不应作为具有所有权的值（因为所有权在运行时域），但在此暂不进行检查。如果进行检查，直接在此处添加
    this->valtype=val->getType();
    this->ownershipFlag=true;
    this->val=val;
}

void VarNode::setBorrowVal(BasicNode *val)
{
    assignmentChecking(val,this->typeRestrictFlag,this->getValType());
    this->clearVal();
    this->valtype=val->getType();
    this->ownershipFlag=false;
    this->val=val;
}

void VarNode::setVarVal(VarNode *node)
{
    if(node->isEmpty())
        throw unassignedAssignedExcep();
    this->clearVal();
    BasicNode* oriVal=node->eval();
    //目前策略为：字面量进行拷贝（有所有权），变量作为无所有权指针传递
    if(copyHelp::isLiteral(oriVal))
        this->setVal(copyHelp::copyVal(oriVal));
    if(oriVal->getType()==Var)
        this->setBorrowVal(oriVal);
    //fix:支持函数指针之后还需要在此处进行添加
}

BasicNode* VarNode::eval()
{
    if(this->isEmpty())
        throw unassignedEvalExcep();
    else
    {
        if(copyHelp::isLiteral(this->val))
            return copyHelp::copyVal(this->val);
        else
            return this->val;
    }
    //注意，多级指针也只会解包一次。不过从返回值基本无法判断返回的是自身还是自身的变量指针值，所以先前需要getValType进行判断
}

void VarNode::clearVal()
{
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

void VarRefNode::bind(BasicNode *val)
{
    assignmentChecking(val,this->typeRestrictFlag,this->getType());
    //目前策略为：字面量进行拷贝（有所有权），变量作为无所有权指针传递
    if(copyHelp::isLiteral(val))
        this->setVal(copyHelp::copyVal(val));
    if(val->getType()==Var)
        this->setBorrowVal(val);
    //fix:支持函数指针之后还需要在此处进行添加
}


void FunNode::addNode(BasicNode *node)
{
    if(!this->funEntity->isVLP()) //支持变长参数就先不进行参数个数检查
    {
        if(this->sonNode.size()+1>this->funEntity->getParnum())
            throw parameterNumExceedingExcep();
    }
    if(!this->funEntity->istypeRestrict())
        if (this->getParType()[sonNode.size()] != evalHelp::typeInfer(node)) //类型检查
            throw callCheckMismatchExcep(TypeMisMatch);
    BasicNode::addNode(node);
}

BasicNode* FunNode::eval()
{
#ifdef PARTEVAL
    this->giveupEval=false;

    try
    {
#endif
        return this->funEntity->eval(this->sonNode);
#ifdef PARTEVAL
    }
    catch(callCheckMismatchExcep e) //因为未赋值变量未求值使得参数类型不匹配，放弃对这个函数求值
            //控制流节点对条件的求值会在此处进行，该节点放弃求值会被上层控制流节点检查到，控制流节点也会放弃求值
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

#ifdef PARTEVAL
bool isNotGiveupEval(BasicNode* node)
{
    if(!(node->getType()==Fun&&dynamic_cast<FunNode*>(node)->giveupEval))
        if(!(node->getType()==If&&dynamic_cast<IfNode*>(node)->giveupEval))
            if(!(node->getType()==While&&dynamic_cast<WhileNode*>(node)->giveupEval))
                //warn:支持新的控制流节点后要在此处添加
                return true;
    return false;
}
#endif

BasicNode* evalHelp::literalCopyEval(BasicNode* node)
{
    if (copyHelp::isLiteral(node))
        return copyHelp::copyVal(node->eval());
    else
        return node->eval();
}

nodeType evalHelp::typeInfer(BasicNode*& node)
{
    if (copyHelp::isLiteral(node))
        return node->getType();
    else if (node->getType() == Fun)
        return dynamic_cast<FunNode*>(node)->getRetType();
    else if (node->getType() == Var)
        return dynamic_cast<VarNode*>(node)->getValType();
    else
    throw Excep("This node type cannot infer");
}

set<nodeType> evalHelp::unionTypeInfer(BasicNode*& node)
{
    if (node->getType() == Pro)
        return dynamic_cast<ProNode*>(node)->getRetType();
    else if (node->getType() == If)
        return dynamic_cast<IfNode*>(node)->getRetType();
    else
    {
        set<nodeType> result;
        result.insert(evalHelp::typeInfer(node));
        return result;
    }
}

BasicNode* Function::eval(vector<BasicNode*> &sonNode)
{
    //对所有参数求值
    for(BasicNode* &node:sonNode)
        evalHelp::recursionEval(node);

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
        ProNode* execpro=new ProNode(*this->body); //执行复制那个，防止函数体求值时被破坏
        BasicNode* result=execpro->eval();
        delete execpro;
        this->unbindArgument();
        return result;
    }
}

Function::~Function()
{
    delete this->body;
    for(VarReference* i:this->argumentList)
        delete i;
}

void Function::addArgument(VarReference *var)
{
    if(!this->isVLP()) //支持变长参数就先不进行参数个数检查
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


BasicNode* ProNode::eval()
{
    vector<BasicNode*>& body = this->sonNode;
    for (unsigned int i = 0;i < body.size();i++)
    {
        auto r = evalHelp::literalCopyEval(body.at(i));
        if (this->isRet.at(i))
            return r;
        else
            copyHelp::delLiteral(r);
    }
    return nullptr;
}

set<nodeType> ProNode::getRetType()
{
    set<nodeType> result;
    vector<BasicNode*>& body = this->sonNode;
    for (unsigned int i = 0;i < body.size();i++)
    {
        if (this->isRet.at(i))
            result.insert(evalHelp::typeInfer(body.at(i)));
    }
    return result;
}

IfNode::IfNode(BasicNode* condition, BasicNode* truePro, BasicNode* falsePro)
{
    if(evalHelp::typeInfer(condition)!=Bool)
        throw Excep("IfNode condition's type must be Bool");
    else if(truePro->getType()!=Pro || falsePro->getType()!=Pro)
        throw Excep("IfNode true/falsePro's type must be Pro");
    else
    {
        BasicNode::addNode(condition);
        BasicNode::addNode(truePro);
        BasicNode::addNode(falsePro);
    }
}

BasicNode* IfNode::eval()
{
    BasicNode* recon;
#ifdef PARTEVAL
    try
    {
#endif
        recon = evalHelp::literalCopyEval(this->sonNode[0]);
#ifdef PARTEVAL
    }
    catch(string e) //fix:这个要指定捕获PARTEVAL引发的异常，其它情况向上层throw
    {
        return this; //放弃求值，直接返回
    }
#endif

    BasicNode* result;
    if(dynamic_cast<BoolNode*>(recon)->getData())
        result= evalHelp::literalCopyEval(this->sonNode[1]);
    else
        result= evalHelp::literalCopyEval(this->sonNode[2]);

    copyHelp::delLiteral(recon);
    return result;
}

set<nodeType> IfNode::getRetType() 
{
    auto s1 = dynamic_cast<ProNode*>(sonNode[1])->getRetType();
    auto s2 = dynamic_cast<ProNode*>(sonNode[2])->getRetType();
    set<nodeType> result;
    set_union(s1.begin(), s1.end(), s2.begin(), s2.end(), inserter(result, result.begin()));
    return result;
}

WhileNode::WhileNode(BasicNode* condition, BasicNode* body)
{
    if(evalHelp::typeInfer(condition)!=Bool)
        throw Excep("whileNode condition's type must be Bool");
    else if(body->getType()!=Pro)
        throw Excep("whileNode body's type must be Pro");
    else
    {
        BasicNode::addNode(condition);
        BasicNode::addNode(body);
    }
}

BasicNode* WhileNode::eval()
{
    ProNode* execpro;
    while(1)
    {
        BasicNode* recon;
#ifdef PARTEVAL
        try
        {
#endif
            recon=this->evalCondition();
#ifdef PARTEVAL
        }
        catch(string e)
        {
            if(e=="conditionalControlNode return")
                return this;
            else
                throw e;
        }
#endif

        if(recon->getType()!=Num)
            throw Excep("WhileNode condition value's type mismatch");
        if(dynamic_cast<NumNode*>(recon)->getData()==1) //为真继续循环
        {
            execpro=new ProNode(*this->body);
            execpro->eval();
            delete execpro;
        }
        else
            break;
    }
    return new nullNode();
}


void ArrNode::clearArray()
{
    //ArrayNode所有权由域持有，内含变量所有权由类本身持有，因此所有elm全部释放
    for(BasicNode* node:this->allelm)
        delete node;
}

ArrNode::ArrNode(int valtype, int len)
{
    this->valtype=valtype;
    this->len=len;
    if(valtype!=-1) //开启严格求值
        this->typeRestrictFlag=true;

    //非定长数组会自动全部初始化
    if(len!=-1)
    {
        for(unsigned int i=0;i<len;i++)
            this->allelm.push_back(new VarNode(valtype));
    }
}

VarNode* ArrNode::addElm(int valtype)
{
    if(!this->isVLA())
        throw Excep("non-VLA Arr cannot add Elm");
    if(this->typeRestrictFlag)
    {
        if(valtype!=this->valtype)
            throw Excep("Element type does not match array type");
    }

    VarNode* node=new VarNode(valtype);
    this->allelm.push_back(node);
    return node;
}

VarNode* ArrNode::addElm(VarNode *var)
{
    if(!this->isVLA())
        throw Excep("non-VLA Arr cannot add Elm");
    if(this->typeRestrictFlag)
    {
        if(valtype!=this->valtype)
            throw Excep("Element type does not match array type");
    }

    this->allelm.push_back(var);
    return var;
}

void ArrNode::delElm(unsigned int sub)
{
    vector<VarNode*>::iterator it=this->allelm.begin()+sub;
    this->allelm.erase(it);
}
