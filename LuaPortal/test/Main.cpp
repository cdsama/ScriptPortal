#include <iostream>
#include <lua.hpp>
#include <luaportal/LuaPortal.h>
using namespace luaportal;

class Foo
{    
public:
    Foo()
    {
    }

    virtual ~Foo()
    {
    }

    static Foo* GetFoo()
    {
        static Foo f;
        return &f;
    }

    void Test()
    {
        std::cout << name << " " << ID << std::endl;
    }

    std::string name;
    int GetID() const
    {
        return ID;
    }

    void SetID(int id)
    {
        ID = id;
    }

private:
    int ID;
};

class Bar : public Foo
{
public:
    Bar() {
        name = "bar";
        SetID(888);
    }
    ~Bar() {}
    static Bar* GetBar()
    {
        static Bar bar;
        return &bar;
    }
};

int main(int argc, char* argv[])
{
    LuaState l;
    l.GlobalContext()
    .BeginNamespace("test")
        .BeginClass<Foo>("Foo")
            .AddData("name",&Foo::name)
            .AddProperty("ID",&Foo::GetID, &Foo::SetID)
            .AddProperty("ConstID", &Foo::GetID)
            .AddStaticFunction("GetFoo",&Foo::GetFoo)
            .AddFunction("Test",&Foo::Test)
        .EndClass()
        .DeriveClass<Bar, Foo>("Bar")
        .AddStaticFunction("GetBar", &Bar::GetBar)
        .EndClass()
    .EndNamespace();

    auto ErrorHandle = [](const std::string& error)
    {
        std::cout << error << std::endl;
    };

    

    l.DoString("f = test.Foo.GetFoo() f.name = 'xiaoming' f:Test()", ErrorHandle);
    l.DoFile("main.lua", ErrorHandle);

    auto add = l.GetGlobal("add"); 
    std::cout <<add(1,2)<<std::endl; 
    auto vbar = l.GetGlobal("vbar");
    
    std::cout << vbar["name"] << vbar["ID"] << std::endl;

    l.DoString("vbar:Test()", ErrorHandle);

    std::cout << "End" << std::endl;

    return 0;
}