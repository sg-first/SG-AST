#include "nodetype.h"

BasicNode* copyHelp::copyVal(BasicNode* node) //（值类型）拷贝
{
    //调用前应该对参数类型进行检查
    if(node->getType()==Num)
        return new NumNode(*dynamic_cast<NumNode*>(node));
    if(node->getType()==String)
        return new StringNode(*dynamic_cast<StringNode*>(node));
    if(node->getType()==Arr)
        return new ArrNode(*dynamic_cast<ArrNode*>(node));
    if(node->getType()==Null)
        return new nullNode(*dynamic_cast<nullNode*>(node));
    //warn:支持更多具拷贝构造函数类型（目前都是字面量）后还需要在此处进行添加
    return nullptr; //如果进行参数检查了不会走到这一步
}

BasicNode* copyHelp::copyNode(BasicNode* node) //拷贝单个子节点，warn:支持新的常规子节点类型后要进行修改
{
    if(copyHelp::isLiteral(node))
        return copyHelp::copyVal(node);
    if(node->getType()==Var) //Var所有权在域，此处不进行复制，直接返回
        return node;
    if(node->getType()==Fun)
        return new FunNode(*dynamic_cast<FunNode*>(node));
    if(node->getType()==VarRef) //VarRef所有权在函数节点，此处不进行复制，直接返回
        return node;
    if(node->getType()==If)
        return new IfNode(*dynamic_cast<IfNode*>(node));
    if(node->getType()==While)
        return new WhileNode(*dynamic_cast<WhileNode*>(node));
    throw string("The type is not regular son nodes to copy"); //Pro不作为常规子节点，不在此考虑
}

BasicNode::BasicNode(const BasicNode &n)
{
    this->retFlag=n.isRet();
    for(BasicNode* i:n.sonNode)
        this->sonNode.push_back(copyHelp::copyNode(i));
}

IfNode::IfNode(const IfNode &n):conditionalControlNode(n)
{
    this->condition=copyHelp::copyNode(n.condition);
    this->truePro=new ProNode(*(n.truePro));
    this->falsePro=new ProNode(*(n.falsePro));
}

WhileNode::WhileNode(const WhileNode &n):conditionalControlNode(n)
{
    this->condition=copyHelp::copyNode(n.condition);
    this->body=new ProNode(*(n.body));
}

VarNode::VarNode(VarNode &n)
{
    this->typeRestrictFlag=n.istypeRestrict();
    this->ownershipFlag=n.isOwnership();
    this->valtype=n.getValType();
    if(this->ownershipFlag) //有所有权拷贝，没有直接赋值
        this->val=copyHelp::copyVal(n.eval()); //此处按照默认的策略——只有字面量作为有所有权的值，所以直接调用拷贝字面量的方法进行拷贝
    else
        this->val=n.eval();
}

ArrNode::ArrNode(const ArrNode &n):BasicNode(n)
{
    this->typeRestrictFlag=n.istypeRestrict();
    this->valtype=n.getValType();
    this->len=n.getLen();
    for(VarNode* pn:n.allelm)
        this->allelm.push_back(new VarNode(*pn));
}
