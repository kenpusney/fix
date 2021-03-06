#include <map>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <memory>
#include <utility>

using namespace std;

struct FixEnv;
struct FixHandle;
struct FixObject
{
    virtual bool isHandle()
    {
        return false;
    }
    
    virtual ~FixObject() = 0;
};

FixObject::~FixObject() {}

struct FixNum:FixObject
{
    int value;
    FixNum(int i):value(i){}
};

using FixParam = vector<shared_ptr<FixObject>>;

struct FixStub:FixObject
{
    size_t params;
    function<void(FixEnv&, FixParam&, FixHandle&)> fn;
    FixStub(function<void(FixEnv&, FixParam&, FixHandle&)> fn_, size_t params_):params(params_), fn(fn_){}
    FixStub(){};
    virtual ~FixStub() {};
};

struct FixHandle:FixObject
{
    size_t params;
    FixStub* stub;
    vector<shared_ptr<FixObject>> slots;

    virtual bool isHandle() override
    {
        return true;
    }
    FixHandle(FixStub& stub_):params(stub_.params), stub(&stub_), slots(){}
    
    virtual void invoke(FixEnv& env)
    {
        this->stub->fn(env, slots, *this);
    }
};
//// TODO
struct FixBlock:FixStub
{
    vector<string> cmds;
    FixBlock();
    virtual ~FixBlock() {};
};

struct FixEnv
{
    static void regist(string name, size_t params, function<void(FixEnv&,FixParam&,FixHandle&)> fn)
    {
        env[name] = make_shared<FixStub>(fn, params);
    }
    
    FixEnv(): stk{}, handle(nullptr), block_name(), is_def_mode(false) {}
    
    void eval(string s)
    {
        if(is_def_mode) {
            if(s == "def")
            {
                env[block_name] = shared_ptr<FixStub>(handle);
                handle = nullptr;
                block_name = "";
                is_def_mode = false;
            }
            else
            {
                handle->cmds.push_back(s);
            }
            return;
        }
        if(s[0] == ':')
        {
            string name = {begin(s)+1, end(s)};
            handle = new FixBlock();
            block_name = name;
            is_def_mode = true;
            return;
        }
        if(s[0] == '$')
        {
            string name = {begin(s)+1, end(s)};
            if(env.find(name) == env.end()){ cout << "Error: Cannot found op `"<< s << "`!" <<endl; return;}
            auto handle = make_shared<FixHandle>(*env[name]);
            for (int i = 1; i < handle -> params; i++ )
            {
                handle->slots.push_back(stk.top());
                stk.pop();
            }
            if (handle->params == 0)
                handle->invoke(*this);
            else
                stk.push(handle);
            return;
        }
        if(env.find(s) == env.end()){ cout << "Error: Cannot found op `"<< s << "`!" <<endl; return;}
        auto handle = make_shared<FixHandle>(*env[s]);
        while(!stk.empty() && handle->params)
        {
            handle->slots.push_back(stk.top());
            handle->params--;
            stk.pop();
        }
        if(!handle->params)
            handle->invoke(*this);
        else
            stk.push(handle);
    }
    
    void eval(int i)
    {
        if(!stk.empty() && stk.top()->isHandle())
        {
            auto handle = dynamic_pointer_cast<FixHandle>(stk.top());
            handle->slots.push_back(make_shared<FixNum>(i));
            handle->params--;
            if(!handle->params)
            {
                stk.pop();
                handle->invoke(*this);
            }
        }else{
            stk.push(make_shared<FixNum>(i));
        }
    }
    
    void ret(shared_ptr<FixObject> ptr)
    {
        stk.push(ptr);
    }
    
    static void init_base();
    
private:

    static map<string,shared_ptr<FixStub>> env;

    stack<shared_ptr<FixObject>> stk;
    FixBlock* handle;
    string block_name;
public:
    bool is_def_mode;
};

FixBlock::FixBlock(): FixStub()
{
    this->fn = [this](FixEnv& fx, FixParam& $, FixHandle& self)
    {
        for(auto cmd : cmds)
        {
            if(cmd[0] >= '0' && cmd[0] <= '9')
                fx.eval(atoi(cmd.c_str()));
            else
                fx.eval(cmd);
        }
    };
}

map<string,shared_ptr<FixStub>> FixEnv::env{};

void FixEnv::init_base()
{
    
    regist("fix", 1,
           [](FixEnv& fx,FixParam& $, FixHandle& self)
    {
        if (dynamic_pointer_cast<FixNum>($[0]))
            cout <<"=> "<< dynamic_pointer_cast<FixNum>($[0])->value << endl;
        fx.ret($[0]);
    });
    
    regist("+", 2,
           [](FixEnv& fx,FixParam& $, FixHandle& self)
    {
        fx.ret(
               make_shared<FixNum>(
                                   dynamic_pointer_cast<FixNum>($[0])->value +
                                   dynamic_pointer_cast<FixNum>($[1])->value
                                   ));
    });
    
    regist("-", 2,
           [](FixEnv& fx,FixParam& $, FixHandle& self)
    {
        fx.ret(
               make_shared<FixNum>(
                                   dynamic_pointer_cast<FixNum>($[0])->value -
                                   dynamic_pointer_cast<FixNum>($[1])->value
                                   ));
    });
    
    regist("*", 2,
           [](FixEnv& fx,FixParam& $, FixHandle& self)
    {
        fx.ret(
               make_shared<FixNum>(
                                   dynamic_pointer_cast<FixNum>($[0])->value *
                                   dynamic_pointer_cast<FixNum>($[1])->value
                                   ));
    });
    
    regist("/", 2,
           [](FixEnv& fx,FixParam& $, FixHandle& self)
    {
        fx.ret(
               make_shared<FixNum>(
                                   dynamic_pointer_cast<FixNum>($[0])->value /
                                   dynamic_pointer_cast<FixNum>($[1])->value
                                   ));
    });
    
    regist("times", 3,
           [](FixEnv& fx, FixParam& $, FixHandle& self)
    {
        auto handle = dynamic_pointer_cast<FixHandle>($[0]);
        auto time = dynamic_pointer_cast<FixNum>($[1])->value;
        handle->slots.push_back($[2]);
        while(time --)
        {
            handle->invoke(fx);
            if (time != 0) {
                auto it = end(handle->slots) - 1;
                it = handle->slots.erase(it);
                handle->slots.insert(it, fx.stk.top());
                fx.stk.pop();
            }
        };
    });
    
    regist("consume", 0,
           [](FixEnv& fx, FixParam& $, FixHandle& self)
    {
        while(!fx.stk.empty())
        {
            if(fx.stk.top()->isHandle()) cout << "[Fix]" << "  ";
            else cout << dynamic_pointer_cast<FixNum>(fx.stk.top())->value << "  ";
            fx.stk.pop();
        }
        cout << endl;
    });
    
    regist("dup", 1,
           [](FixEnv& fx, FixParam& $, FixHandle& self)
    {
        fx.ret($[0]);
        fx.ret($[0]);
    });
    
    regist("swap", 2,
           [](FixEnv& fx, FixParam& $, FixHandle& self)
    {
        fx.ret($[0]);
        fx.ret($[1]);
    });
    
    regist("exit", 0, [](FixEnv& fx, FixParam& $, FixHandle& self)
    {
        exit(0);
    });
}

int main(){
    FixEnv fx;
    FixEnv::init_base();
    string s;
    while(cin >> s)
    {
        if(!s.size()) continue;
        if(s[0] >= '0' && s[0] <= '9' && !fx.is_def_mode)
            fx.eval(atoi(s.c_str()));
        else
            fx.eval(s);
    }
    return 0;
}
