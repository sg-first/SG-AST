#pragma once
#include <string>
#include <vector>
#include <functional>
#include "marco.h"
#include "excep.h"
using namespace std;

class BasicNode //不可直接创建对象
{
protected:
    bool retFlag=false;
public:
    virtual int getType()=0;
    virtual void addNode(BasicNode* node) {this->sonNode.push_back(node);} //使用该方法添加成员可查错
    virtual BasicNode* eval()=0;
    virtual ~BasicNode();
    BasicNode(const BasicNode& n);
    BasicNode(){}

    //fix:这个ret的实现方法可能不太对劲
    void setRet() {this->retFlag=true;} //不可eval节点设置ret无效
    bool isRet() {return this->retFlag;}
    vector<BasicNode*> sonNode;
};
typedef function<bool(vector<BasicNode*>&sonNode)>canBE; //检测函数基础求值参数表是否合法
typedef function<BasicNode*(vector<BasicNode*>&sonNode)>BE; //进行基础求值


class nullNode : public BasicNode
{
public:
    virtual int getType() {return Null;}
    virtual void addNode(BasicNode*) {throw addSonExcep(Null);}
    virtual BasicNode* eval() {throw cannotEvaledExcep();}
};


class NumNode : public BasicNode
{
protected:
    double num;
public:
    virtual int getType() {return Num;}
    virtual void addNode(BasicNode*) {throw addSonExcep(Num);}
    virtual BasicNode* eval() {return this;}
    NumNode(double num) {this->num=num;}
    NumNode(const NumNode& n):BasicNode(n) {this->num=n.num;}

    double getNum() {return this->num;}
};


class StringNode : public BasicNode
{
protected:
    string str;
public:
    virtual int getType() {return String;}
    virtual void addNode(BasicNode*) {throw addSonExcep(String);}
    virtual BasicNode* eval() {return this;}
    StringNode(string str) {this->str=str;}
    StringNode(const StringNode& n):BasicNode(n) {this->str=n.str;}

    string getStr() {return this->str;}
};


class VarNode : public BasicNode
{
protected:
    BasicNode* val=nullptr;
    bool typeRestrictFlag=false;
    int valtype;
    bool ownershipFlag;

    void assignmentChecking(BasicNode* val);
public:
    virtual int getType() {return Var;}
    virtual void addNode(BasicNode*) {throw addSonExcep(Var);}
    virtual BasicNode* eval();
    virtual ~VarNode();
    VarNode(int valtype=-1);

    bool isEmpty() {return (this->val==nullptr);}
    bool istypeRestrict() {return this->typeRestrictFlag;}
    int getValType() {return this->valtype;}
    bool isOwnership() {return this->ownershipFlag;}
    void setVal(BasicNode* val); //直接对值进行赋值，用这个传进来意味着转移所有权到本类（一般赋值为字面量用）
    void setBorrowVal(BasicNode* val); //直接对值进行赋值，用这个不转移所有权（一般赋值为变量指针用）
    void setVarVal(VarNode* node); //传递变量的值到this的值，即需要进行一次解包
    void clearVal();

    #ifdef READABLEGEN
    string NAME;
    #endif
};
typedef VarNode Variable; //内存实体是Variable，其指针才作为节点（不像某些节点一样是遇到一个就new一次），参考函数实体和函数节点的思想


class VarRefNode : public BasicNode
{
protected:
    BasicNode* val=nullptr;
    bool typeRestrictFlag=false;
    int valtype;
    bool ownershipFlag;

    void assignmentChecking(BasicNode* val);
    void setVal(BasicNode* val);
    void setBorrowVal(BasicNode* val);
public:
    virtual int getType() {return VarRef;}
    virtual void addNode(BasicNode*) {throw addSonExcep(VarRef);}
    virtual ~VarRefNode();
    virtual BasicNode* eval(); //eval结果是目前形参绑定到的实参
    VarRefNode(int valtype=-1);

    void bind(BasicNode* val);
    void unbind();
    bool isbind() {return (this->val!=nullptr);}
    //因为本类对象只作为其值的一个别名使用，所以不提供任何属性的访问器
};
typedef VarRefNode VarReference; //同上


class ProNode : public BasicNode
{
public:
    virtual int getType() {return Pro;}
    virtual BasicNode* eval();
    ProNode(){}
    ProNode(const ProNode& n):BasicNode(n){}
    //fix:该节点现在可以求值，实际应该做成逗号表达式一类的结构，支持PARTEVAL。但现在pro eval完了都释放，所以没啥用
    //BasicNode* getHeadNode() {return this->sonNode.at(0);}
    BasicNode* getSen(int sub) {return this->sonNode.at(sub);}
    void passEffect(vector<BasicNode*>sonNode);
};


class Function
{
private:
    unsigned int parnum; //参数个数
    bool VLP; //是否不进行参数个数检查
    //关于基础求值
    canBE canBEfun;
    BE BEfun;
    bool iscanBE=false;
    //关于pro求值
    ProNode* body=nullptr; //是ret节点返回，最后一个元素视为返回值（如果没有填nullNode）（fix:这个ret路子可能是错的）
    vector<VarReference*>argumentList; //形参列表，持有所有权。（warn:用了这种方法将很难并行化，一个函数实体同时只能被一组实参占用）
    void unbindArgument();
    void bindArgument(vector<BasicNode*>&sonNode);
public:
    Function(unsigned int parnum,ProNode* body,bool VLP=false):parnum(parnum),body(body),VLP(VLP){} //普通函数（有函数体）
    Function(unsigned int parnum,canBE canBEfun,BE BEfun,bool VLP=false):
        parnum(parnum),canBEfun(canBEfun),BEfun(BEfun),VLP(VLP),iscanBE(true){} //调用到函数接口
    ~Function();

    ProNode* getFunBody() {return this->body;}
    unsigned int getParnum() {return this->parnum;}
    bool isVLP() {return this->VLP;}
    void addArgument(VarReference* var); //先在外面new好，然后转移所有权进来
    BasicNode* eval(vector<BasicNode *> &sonNode);

    #ifdef READABLEGEN
    string NAME;
    #endif
};


class FunNode : public BasicNode
{
protected:
    Function* funEntity; //所有权在scope，不在这里析构
public:
    virtual int getType() {return Fun;}
    virtual void addNode(BasicNode* node);
    virtual BasicNode* eval();
    FunNode(Function* funEntity=nullptr):funEntity(funEntity){}
    FunNode(const FunNode& n):BasicNode(n) {this->funEntity=n.funEntity;} //函数实体所有权不在此，所以可以放心不复制

    bool haveEntity() {return this->funEntity!=nullptr;}
    void setEntity(Function* funEntity) {this->funEntity=funEntity;}
    Function* getEntity(){return this->funEntity;}
    ProNode* getFunBody() {return this->funEntity->getFunBody();}

    #ifdef PARTEVAL
    bool giveupEval; //如果里边有符号变量，暂时放弃对此节点（基本为函数节点）的求值，并在此做标记防止根函数节点被视为求值结束而delete
    //所有控制流节点也要有该成员（若控制流条件中含有符号变量，放弃对整个控制流节点的执行（求值））
    #endif
};

class conditionalControlNode : public BasicNode
{
protected:
    BasicNode* condition;
    BasicNode* evalCondition();
public:
    conditionalControlNode(BasicNode* condition):condition(condition){}
    conditionalControlNode(const conditionalControlNode& n):BasicNode(n){}

    #ifdef PARTEVAL
    bool giveupEval;
    #endif
};

class IfNode : public conditionalControlNode
{
protected:
    ProNode* truePro;
    ProNode* falsePro;
public:
    virtual int getType() {return If;}
    virtual void addNode(BasicNode*) {throw addSonExcep(If);}
    virtual BasicNode* eval();
    IfNode(const IfNode& n);
    IfNode(BasicNode* condition,ProNode* truePro,ProNode* falsePro):conditionalControlNode(condition),truePro(truePro),falsePro(falsePro){}
    virtual ~IfNode();
};

class WhileNode : public conditionalControlNode
{
protected:
    ProNode* body;
public:
    virtual int getType() {return While;}
    virtual void addNode(BasicNode*) {throw addSonExcep(While);}
    virtual BasicNode* eval(); //该类的eval完全不求值，只通过循环本身产生副作用
    WhileNode(const WhileNode& n);
    WhileNode(BasicNode* condition,ProNode* body):conditionalControlNode(condition),body(body){}
    virtual ~WhileNode();
};

class copyHelp
{
public:
    static BasicNode* copyNode(BasicNode* node);
    static BasicNode* copyVal(BasicNode* node);
    static bool isLiteral(BasicNode* node);
};
