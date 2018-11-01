#include "nodetype.h"

BasicNode* copyNode(BasicNode* node) //拷贝单个子节点，支持新的常规子节点类型后要进行修改
{
    if(node->getType()==Num)
        return new NumNode(*dynamic_cast<NumNode*>(node));
    if(node->getType()==String)
        return new StringNode(*dynamic_cast<StringNode*>(node));
    if(node->getType()==Var) //Var所有权在域，此处不进行复制，直接返回
        return node;
    if(node->getType()==Fun)
        return new FunNode(*dynamic_cast<FunNode*>(node));
    if(node->getType()==VarRef) //VarRef所有权在函数节点，此处不进行复制，直接返回
        return node;
    if(node->getType()==If)
        return new IfNode(*dynamic_cast<IfNode*>(node));
    throw string ("The type is not regular son nodes to copy"); //Pro不作为常规子节点，不在此考虑
}

BasicNode::BasicNode(const BasicNode &n)
{
    this->retFlag=n.retFlag;
    for(BasicNode* i:n.sonNode)
        this->sonNode.push_back(copyNode(i));
}

IfNode::IfNode(const IfNode &n):BasicNode(n)
{
    this->condition=copyNode(n.condition);
    this->truePro=new ProNode(*(n.truePro));
    this->falsePro=new ProNode(*(n.falsePro));
}
