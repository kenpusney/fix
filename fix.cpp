#include <map>
#include <functional>
#include <string>
#include <stack>
#include <iostream>
#include <cstdlib>

using namespace std;

struct FixEnv{
    static map<string,function<void(FixEnv&)>> env;
    static void regist(string name, function<void(FixEnv&)> fn){
        env[name] = fn;
    }
    stack<int> stk;
    void push(string s){
        env[s](*this);
    }
    void push(int i){
        stk.push(i);
    }
    static void init_base();
};

map<string,function<void(FixEnv&)>> FixEnv::env{};

void FixEnv::init_base(){
    regist("fix",[](FixEnv& fx){
        cout << fx.stk.top() << endl;
    });
    regist("add",[](FixEnv& fx){
        auto x = fx.stk.top();fx.stk.pop();
        auto y = fx.stk.top();fx.stk.pop();
        fx.stk.push(x+y);
    });
    regist("sub",[](FixEnv& fx){
        auto x = fx.stk.top();fx.stk.pop();
        auto y = fx.stk.top();fx.stk.pop();
        fx.stk.push(x-y);
    });
    regist("mul",[](FixEnv& fx){
        auto x = fx.stk.top();fx.stk.pop();
        auto y = fx.stk.top();fx.stk.pop();
        fx.stk.push(x*y);
    });
    regist("fix",[](FixEnv& fx){
        cout << fx.stk.top() << endl;
    });
}

int main(){
    FixEnv fx;
    FixEnv::init_base();
    string s;
    while(cin >> s){
        if(s[0] >= '0' && s[0] <= '9'){
            fx.push(atoi(s.c_str()));
        }else{
            fx.push(s);
        }
    }
    return 0;
}
