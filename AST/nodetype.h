#pragma once
#include <string>
#include <vector>
#include <functional>
#include "marco.h"
#include "excep.h"
using namespace std;

class BasicNode
{
public:
    virtual nodeType getType() const=0;
    virtual void addNode(BasicNode* node) {this->sonNode.push_back(node);} //使用该方法添加成员可查错
    virtual BasicNode* eval()=0;
    virtual ~BasicNode();
    BasicNode(const BasicNode& n);
    BasicNode(){}

    vector<BasicNode*> sonNode;
};
typedef function<bool(vector<BasicNode*>&sonNode)>canBE; //检测函数基础求值参数表是否合法
typedef function<BasicNode*(vector<BasicNode*>&sonNode)>BE; //进行基础求值


class nullNode : public BasicNode
{
public:
    virtual nodeType getType() const { return Null; }
    virtual void addNode(BasicNode*) { throw addSonExcep(Null); }
    virtual BasicNode* eval() {throw cannotEvaledExcep();}
};

template<typename T, nodeType TAG>
class PackNode : public BasicNode
{
protected:
    T data;
public:
    virtual nodeType getType() const { return TAG; }
    virtual void addNode(BasicNode*) { throw addSonExcep(TAG); }
    virtual BasicNode* eval() { return this; }
    PackNode(T data) : data(data) {}
    PackNode(const PackNode& n) : BasicNode(n), data(n.getData()) {}
    T getData() const { return this->data; }
};

typedef PackNode<double, Num> NumNode;
typedef PackNode<bool, Bool> BoolNode;
typedef PackNode<string, String> StringNode;


class VarNode : public BasicNode
{
protected:
    BasicNode* val=nullptr;
    bool typeRestrictFlag=false;
    int valtype;
    bool ownershipFlag;

public:
    virtual int getType() const {return Var;}
    virtual void addNode(BasicNode*) {throw addSonExcep(Var);}
    virtual BasicNode* eval(); //字面量会拷贝
    virtual ~VarNode();
    VarNode(int valtype=-1);
    VarNode(VarNode &n);

    bool isEmpty() const {return (this->val==nullptr);}
    bool istypeRestrict() const {return this->typeRestrictFlag;}
    int getValType() const {return this->valtype;}
    bool isOwnership() const {return this->ownershipFlag;}
    void setVal(BasicNode* val); //直接对值进行赋值，用这个传进来意味着转移所有权到本类（一般赋值为字面量用）
    void setBorrowVal(BasicNode* val); //直接对值进行赋值，用这个不转移所有权（一般赋值为变量指针用）
    void setVarVal(VarNode* node); //传递变量的值到this的值，即需要进行一次解包
    void clearVal();
    BasicNode* getVal() {return this->val;} //不会拷贝字面量
#ifdef READABLEGEN
    string NAME;
#endif
};
typedef VarNode Variable; //内存实体是Variable，其指针才作为节点（不像某些节点一样是遇到一个就new一次），参考函数实体和函数节点的思想

class ArrNode : public BasicNode //注意，该节点是作为字面量存在的，传递时应当转移所有权
{
protected:
    bool typeRestrictFlag=false;
    int valtype; //fix:应当记录交叉类型，所以这里和设计原则有些冲突，集成TSystem前要改好
    //可选的方案：将typeRestrictFlag改为【严格同一类型】，同一类型用现在这个valtype，正常的typeRestrictFlag检查交叉类型
    int len; //最大长度，-1为不限

    void clearArray(); //这个暂时调整成私有
public:
    virtual int getType() const {return Arr;}
    virtual void addNode(BasicNode*) {throw addSonExcep(Arr);}
    virtual BasicNode* eval() {return this;}
    virtual ~ArrNode() {this->clearArray();}
    ArrNode(int valtype=-1,int len=-1);
    ArrNode(const ArrNode& n);

    //这两个addElm是给变长数组用的，定长会自动分配内存
    VarNode* addElm(int valtype=-1);
    VarNode* addElm(VarNode* var); //注意，该函数会移交所传递变量的所有权
    void delElm(unsigned int sub);
    VarNode* getElm(unsigned int sub) {return this->allelm.at(sub);} //和访问直接访问allelm没啥区别，为了和楼上对称先留着
    unsigned int getNowLen() const {return this->allelm.size();}
    bool istypeRestrict() const {return this->typeRestrictFlag;}
    int getValType() const {return this->valtype;} //调用前自行检查istypeRestrict
    bool isVLA() const {return this->len==-1;}
    int getLen() const {return this->len;} //最大长度

    vector<VarNode*>allelm;
};
//typedef ArrNode Array; //同上

class VarRefNode : public BasicNode
{
protected:
    BasicNode* val=nullptr;
    bool typeRestrictFlag=false;
    int valtype;
    bool ownershipFlag;

    void setVal(BasicNode* val);
    void setBorrowVal(BasicNode* val);
public:
    virtual int getType() const {return VarRef;}
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
protected:
    vector<bool> isRet;
public:
    virtual nodeType getType() const { return Pro; }
    virtual BasicNode* eval();
    ProNode() {}
    ProNode(const ProNode& n) :BasicNode(n) {}
    //fix:该节点现在可以求值，实际应该做成逗号表达式一类的结构，支持PARTEVAL。但现在pro eval完了都释放，所以没啥用
    //BasicNode* getHeadNode() {return this->sonNode.at(0);}
    BasicNode* getSen(unsigned int sub) { return this->sonNode.at(sub); }
    virtual void addNode(BasicNode* node) { throw addSonExcep(Pro); }
    void addNode(BasicNode* node, bool isRet); //ProNode用这个添加子节点才是对的
    set<nodeType> getRetType();
};


class Function
{
private:
    int parnum; //参数个数
    //bool VLP; //是否不进行参数个数检查
    //关于基础求值
    canBE canBEfun;
    BE BEfun;
    bool iscanBE=false;
    //关于pro求值
    ProNode* body=nullptr;
    vector<VarReference*>argumentList; //形参列表，持有所有权。（warn:用了这种方法将很难并行化，一个函数实体同时只能被一组实参占用）
    
    bool typeRestrictFlag = true; //类型检查标记（fix:相关设施还需要完善一下）

    void unbindArgument();
    void bindArgument(vector<BasicNode*>&sonNode);

public:
    Function(ProNode* body,int parnum=-1):parnum(parnum),body(body){} //普通函数（有函数体）
    Function(canBE canBEfun,BE BEfun,int parnum=-1):
        parnum(parnum),canBEfun(canBEfun),BEfun(BEfun),iscanBE(true){} //调用到函数接口
    ~Function();

    ProNode* getFunBody() {return this->body;}
    int getParnum() {return this->parnum;}
    bool isVLP() {return this->parnum==-1;}
    void addArgument(VarReference* var); //先在外面new好，然后转移所有权进来
    BasicNode* eval(vector<BasicNode *> &sonNode);
    bool istypeRestrict() const { return this->typeRestrictFlag; }

#ifdef READABLEGEN
    string NAME;
#endif
};


class FunNode : public BasicNode
{
protected:
    Function* funEntity; //所有权在scope，不在这里析构
public:
    virtual int getType() const {return Fun;}
    virtual void addNode(BasicNode* node);
    virtual BasicNode* eval();
    FunNode(Function* funEntity):funEntity(funEntity){}
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

class IfNode : public BasicNode
{
public:
    virtual nodeType getType() const { return If; }
    set<nodeType> getRetType();
    virtual void addNode(BasicNode*) { throw addSonExcep(If); }
    virtual BasicNode* eval();
    IfNode(BasicNode* condition, BasicNode* truePro, BasicNode* falsePro);
};

class WhileNode
{
public:
    virtual int getType() const {return While;}
    virtual void addNode(BasicNode*) {throw addSonExcep(While);}
    virtual BasicNode* eval(); //该类的eval不求值（返回NullNode），只通过循环本身产生副作用
    WhileNode(BasicNode* condition, BasicNode* body);
};

class copyHelp
{
public:
    static BasicNode* copyNode(BasicNode* node);
    static BasicNode* copyVal(BasicNode* node);
    static bool isLiteral(const BasicNode* node) { return copyHelp::isLiteral(node->getType()); }
    static bool isLiteral(nodeType type) { return (type == Num || type == String || type == Bool); } //warn:添加新的字面量要进行修改
    static void delTree(BasicNode* n);
    static void delLiteral(BasicNode* n);
};

class evalHelp
{
public:
    static BasicNode* literalCopyEval(BasicNode* node);
    static nodeType typeInfer(BasicNode*& node);
    static set<nodeType> unionTypeInfer(BasicNode*& node);
};