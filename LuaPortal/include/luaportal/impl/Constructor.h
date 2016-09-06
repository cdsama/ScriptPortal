#pragma once
/** Constructor generators.
 
 These templates call operator new with the contents of a type/value
 list passed to the Constructor . Two versions of Call() are provided. 
 One performs a regular new, the other performs a placement new.
 */


template<typename... Ps>
struct Constructor {};

template<typename T, typename... P>
struct NewData {
    static T* Call(P... args)
    {
        return new T(args...);
    }
};

template<typename T, typename... P>
struct NewData<T,void *,P...> {
    
    static T* Call(void* mem, P... args)
    {
        return new(mem) T(args...);
    }
};

template<typename T, typename... P>
struct RecursiveNewData;

template<typename T>
struct RecursiveNewData<T>{
    template<typename... U>
    static T* Call(lua_State *L, int start, U... u)
    {
        return NewData<T, U...>::Call(u...);
    }
};

template<typename T,typename H, typename... P>
struct RecursiveNewData<T, H, P...>{
    template<typename... U>
    static T* Call(lua_State *L,int start, U... u)
    {
        const int index = static_cast<int>(start + sizeof...(u));
        H h = Stack<H>::Get(L, index);
        return RecursiveNewData<T, P...>::Call(L,start, u..., h);
    }
};

template<typename C, typename... P>
struct ConstructorFunc {
    static int placementProxy(lua_State* L) {
        auto place = UserdataValue<C>::place(L);
        RecursiveNewData<C, P...>::Call(L, 1, place->GetVoidPointer());
        place->markConstructed();
        return 1;
    }
    
    static int containerProxy(lua_State* L) {
        typedef typename ContainerTraits<C>::Type T;
        T* const p = RecursiveNewData<T, P...>::Call(L, 2);
        UserdataSharedHelper<C, false>::Push(L, p);
        return 1;
    }
};
