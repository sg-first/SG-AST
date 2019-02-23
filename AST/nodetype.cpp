#include "nodetype.h"

bool copyHelp::isLiteral(int type) //warn:是否为字面量，添加新的字面量要进行修改
{
    return (type==Num||type==String||type==Arr||type==Null); //暂且先把Null安排上
}

bool copyHelp::isLiteral(BasicNode* node)
{
    return copyHelp::isLiteral(node->getType());
}

bool isNotAssignable(BasicNode* val) //warn:是否不可赋值给变量，支持新的节点类型要进行修改
{
    return (val->getType()==Pro||val->getType()==Fun||val->getType()==If||val->getType()==While);
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

void assignmentChecking(BasicNode *val,bool thisTypeRestrictFlag,int thisvaltype)
{
    if(isNotAssignable(val))
        throw cannotAssignedExcep();
    if(thisTypeRestrictFlag&&val->getType()!=thisvaltype)
        throw string("Mismatch between assignment type and variable type");
}

void VarNode::setVal(BasicNode* val)
{
    assignmentChecking(val,this->typeRestrictFlag,this->getType());
    this->clearVal();
    //warn:理论上讲按照目前的设计，变量不应作为具有所有权的值（因为所有权在运行时域），但在此暂不进行检查。如果进行检查，直接在此处添加
    this->valtype=val->getType();
    this->ownershipFlag=true;
    this->val=val;
}

void VarNode::setBorrowVal(BasicNode *val)
{
    assignmentChecking(val,this->typeRestrictFlag,this->getType());
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

void recursionEval(BasicNode* &node)
{
    if(copyHelp::isLiteral(node))
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

        if(node->getType()!=Var&&node->getType()!=VarRef)
        #ifdef PARTEVAL
            if(isNotGiveupEval(node)) //对放弃求值的节点，不进行删除
        #endif
            delete node;
        node=result; //节点的替换在这里（父节点）完成，子节点只需要返回即可
        //对于已经赋值的变量，整体过程是用值替代本身变量在AST中的位置，不过变量本身并没有被析构，因为变量的所有权在scope（后面可能还要访问）
    }
}

BasicNode* Function::eval(vector<BasicNode*> &sonNode)
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
    vector<BasicNode*>&body=this->sonNode;
    for(unsigned int i=0;i<body.size()-1;i++) //最后一个可能是返回值，先留着后面单独处理
    {
        recursionEval(body.at(i));
        if(body.at(i)->isRet())
            return body.at(i);
    }
    //前面都不是返回值，最后一个是
    BasicNode* lastnode=body.at(body.size()-1);
    if(lastnode->getType()!=Null)
        recursionEval(lastnode);
    return lastnode;
}


BasicNode* conditionalControlNode::evalCondition()
{
    #ifdef PARTEVAL
    this->giveupEval=false;
    #endif

    BasicNode* recon;
    #ifdef PARTEVAL
    try
    {
    #endif
    recon=this->condition->eval();
    #ifdef PARTEVAL
    }
    catch(unassignedEvalExcep) //condition直接就是个符号变量，放弃求值返回自身
    {throw string("conditionalControlNode return");}
    #endif

    #ifdef PARTEVAL
    if(recon->getType()==Fun&&dynamic_cast<FunNode*>(recon)->giveupEval) //是一个函数里面有放弃求值的变量
    {
        this->giveupEval=true; //本控制流节点也放弃求值
        throw string("conditionalControlNode return");
    }
    #endif

    return recon;
}

BasicNode* IfNode::eval()
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
            return this; //放弃求值，直接返回
        else
            throw e;
    }
    #endif

    if(recon->getType()!=Num)
        throw string("IfNode condition value's type mismatch");
    BasicNode* result;
    if(dynamic_cast<NumNode*>(recon)->getNum()==0) //这里判断false
        result=this->falsePro->eval();
    else
        result=this->truePro->eval();

    delete recon;
    return result;
}

IfNode::~IfNode()
{
    delete this->condition;
    delete this->truePro;
    delete this->falsePro;
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
            throw string("WhileNode condition value's type mismatch");
        if(dynamic_cast<NumNode*>(recon)->getNum()==1) //为真继续循环
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

WhileNode::~WhileNode()
{
    delete this->condition;
    delete this->body;
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
        throw string("non-VLA Arr cannot add Elm");
    if(this->typeRestrictFlag)
    {
        if(valtype!=this->valtype)
            throw string("Element type does not match array type");
    }

    VarNode* node=new VarNode(valtype);
    this->allelm.push_back(node);
    return node;
}

VarNode* ArrNode::addElm(VarNode *var)
{
    if(!this->isVLA())
        throw string("non-VLA Arr cannot add Elm");
    if(this->typeRestrictFlag)
    {
        if(valtype!=this->valtype)
            throw string("Element type does not match array type");
    }

    this->allelm.push_back(var);
    return var;
}

void ArrNode::delElm(unsigned int sub)
{
    vector<VarNode*>::iterator it=this->allelm.begin()+sub;
    this->allelm.erase(it);
}
