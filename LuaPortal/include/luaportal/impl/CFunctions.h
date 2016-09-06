// We use a structure so we can define everything in the header.
//
struct CFunc
{
    //----------------------------------------------------------------------------
    /**
     __index metamethod for a namespace or class static members.
     
     This handles:
     Retrieving functions and class static methods, stored in the metatable.
     Reading global and class static data, stored in the __propget table.
     Reading global and class properties, stored in the __propget table.
     */
    static int IndexMetaMethod(lua_State* L)
    {
        int result = 0;
        lua_getmetatable(L, 1);                // push metatable of arg1
        for(;;)
        {
            lua_pushvalue(L, 2);                 // push key arg2
            lua_rawget(L, -2);                   // lookup key in metatable
            if(lua_isnil(L, -1))                // not found
            {
                lua_pop(L, 1);                     // discard nil
                rawgetfield(L, -1, "__propget");   // lookup __propget in metatable
                lua_pushvalue(L, 2);               // push key arg2
                lua_rawget(L, -2);                 // lookup key in __propget
                lua_remove(L, -2);                 // discard __propget
                if(lua_iscfunction(L, -1))
                {
                    lua_remove(L, -2);               // discard metatable
                    lua_pushvalue(L, 1);             // push arg1
                    lua_call(L, 1, 1);               // call cfunction
                    result = 1;
                    break;
                }
                else
                {
                    assert(lua_isnil(L, -1));
                    lua_pop(L, 1);                   // discard nil and fall through
                }
            }
            else
            {
                assert(lua_istable(L, -1) || lua_iscfunction(L, -1));
                lua_remove(L, -2);
                result = 1;
                break;
            }
            
            rawgetfield(L, -1, "__parent");
            if(lua_istable(L, -1))
            {
                // Remove metatable and repeat the search in __parent.
                lua_remove(L, -2);
            }
            else
            {
                // Discard metatable and return nil.
                assert(lua_isnil(L, -1));
                lua_remove(L, -2);
                result = 1;
                break;
            }
        }
        
        return result;
    }
    
    //----------------------------------------------------------------------------
    /**
     __newindex metamethod for a namespace or class static members.
     
     The __propset table stores proxy functions for assignment to:
     Global and class static data.
     Global and class properties.
     */
    static int NewIndexMetaMethod(lua_State* L)
    {
        int result = 0;
        lua_getmetatable(L, 1);                // push metatable of arg1
        for(;;)
        {
            rawgetfield(L, -1, "__propset");     // lookup __propset in metatable
            assert(lua_istable(L, -1));
            lua_pushvalue(L, 2);                 // push key arg2
            lua_rawget(L, -2);                   // lookup key in __propset
            lua_remove(L, -2);                   // discard __propset
            if(lua_iscfunction(L, -1))          // ensure value is a cfunction
            {
                lua_remove(L, -2);                 // discard metatable
                lua_pushvalue(L, 3);               // push new value arg3
                lua_call(L, 1, 0);                 // call cfunction
                result = 0;
                break;
            }
            else
            {
                assert(lua_isnil(L, -1));
                lua_pop(L, 1);
            }
            
            rawgetfield(L, -1, "__parent");
            if(lua_istable(L, -1))
            {
                // Remove metatable and repeat the search in __parent.
                lua_remove(L, -2);
            }
            else
            {
                assert(lua_isnil(L, -1));
                lua_pop(L, 2);
                result = luaL_error(L,"no writable variable '%s'", lua_tostring(L, 2));
            }
        }
        
        return result;
    }
    
    //----------------------------------------------------------------------------
    /**
     lua_CFunction to report an error writing to a read-only value.
     
     The name of the variable is in the first upvalue.
     */
    static int ReadOnlyError(lua_State* L)
    {
        std::string s;
        
        s = s + "'" + lua_tostring(L, lua_upvalueindex(1)) + "' is read-only";
        
        return luaL_error(L, s.c_str());
    }
    
    //----------------------------------------------------------------------------
    /**
     lua_CFunction to Get a variable.
     
     This is used for global variables or class static data members.
     
     The pointer to the data is in the first upvalue.
     */
    template<typename T>
    static int GetVariable(lua_State* L)
    {
        assert(lua_islightuserdata(L, lua_upvalueindex(1)));
        T const* ptr = static_cast<T const*>(lua_touserdata(L, lua_upvalueindex(1)));
        assert(ptr != 0);
        Stack<T>::Push(L, *ptr);
        return 1;
    }
    
    //----------------------------------------------------------------------------
    /**
     lua_CFunction to set a variable.
     
     This is used for global variables or class static data members.
     
     The pointer to the data is in the first upvalue.
     */
    template<typename T>
    static int SetVariable(lua_State* L)
    {
        assert(lua_islightuserdata(L, lua_upvalueindex(1)));
        T* ptr = static_cast<T*>(lua_touserdata(L, lua_upvalueindex(1)));
        assert(ptr != 0);
        *ptr = Stack<T>::Get(L, 1);
        return 0;
    }
    
    //----------------------------------------------------------------------------
    /**
     lua_CFunction to call a function with a return value.
     
     This is used for global functions, global properties, class static methods,
     and class static properties.
     
     The function pointer is in the first upvalue.
     */
    template<typename FnPtr, typename ReturnType = typename FuncTraits<FnPtr>::ReturnType>
    struct Call
    {
        //        typedef typename FuncTraits<FnPtr>::Params Params;
        static int GeneratedFunction(lua_State* L)
        {
            assert(isfulluserdata(L, lua_upvalueindex(1)));
            FnPtr const& fnptr = *static_cast<FnPtr const*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(fnptr != 0);
            //            ArgList<Params> args(L);
            Stack<typename FuncTraits<FnPtr>::ReturnType>::Push(L, FuncTraits<FnPtr>::Call(fnptr, L));
            return 1;
        }
    };
    
    //----------------------------------------------------------------------------
    /**
     lua_CFunction to call a function with no return value.
     
     This is used for global functions, global properties, class static methods,
     and class static properties.
     
     The function pointer is in the first upvalue.
     */
    template<typename FnPtr>
    struct Call<FnPtr, void>
    {
        static int GeneratedFunction(lua_State* L)
        {
            assert(isfulluserdata(L, lua_upvalueindex(1)));
            FnPtr const& fnptr = *static_cast<FnPtr const*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(fnptr != 0);
            FuncTraits<FnPtr>::Call(fnptr, L);
            return 0;
        }
    };
    
    //----------------------------------------------------------------------------
    /**
     lua_CFunction to call a class member function with a return value.
     
     The member function pointer is in the first upvalue.
     The class userdata object is at the top of the Lua stack.
     */
    template<typename MemFnPtr, typename ReturnType = typename FuncTraits<MemFnPtr>::ReturnType>
    struct CallMember
    {
        typedef typename FuncTraits<MemFnPtr>::ClassType T;
        
        static int GeneratedFunction(lua_State* L)
        {
            assert(isfulluserdata(L, lua_upvalueindex(1)));
            T* const t = Userdata::Get<T>(L, 1, false);
            MemFnPtr const& fnptr = *static_cast<MemFnPtr const*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(fnptr != 0);
            Stack<ReturnType>::Push(L, FuncTraits<MemFnPtr>::Call(t, fnptr, L));
            return 1;
        }
    };
    
    template<typename MemFnPtr,
    class ReturnType = typename FuncTraits<MemFnPtr>::ReturnType>
    struct CallConstMember
    {
        typedef typename FuncTraits<MemFnPtr>::ClassType T;        
        static int GeneratedFunction(lua_State* L)
        {
            assert(isfulluserdata(L, lua_upvalueindex(1)));
            T const* const t = Userdata::Get<T>(L, 1, true);
            MemFnPtr const& fnptr = *static_cast<MemFnPtr const*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(fnptr != 0);
            Stack<ReturnType>::Push(L, FuncTraits<MemFnPtr>::Call(t, fnptr, L));
            return 1;
        }
    };
    
    //----------------------------------------------------------------------------
    /**
     lua_CFunction to call a class member function with no return value.
     
     The member function pointer is in the first upvalue.
     The class userdata object is at the top of the Lua stack.
     */
    template<typename MemFnPtr>
    struct CallMember<MemFnPtr, void>
    {
        typedef typename FuncTraits<MemFnPtr>::ClassType T;
        
        static int GeneratedFunction(lua_State* L)
        {
            assert(isfulluserdata(L, lua_upvalueindex(1)));
            T* const t = Userdata::Get<T>(L, 1, false);
            MemFnPtr const& fnptr = *static_cast<MemFnPtr const*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(fnptr != 0);
            FuncTraits<MemFnPtr>::Call(t, fnptr, L);
            return 0;
        }
    };
    
    template<typename MemFnPtr>
    struct CallConstMember<MemFnPtr, void>
    {
        typedef typename FuncTraits<MemFnPtr>::ClassType T;
        
        static int GeneratedFunction(lua_State* L)
        {
            assert(isfulluserdata(L, lua_upvalueindex(1)));
            T const* const t = Userdata::Get<T>(L, 1, true);
            MemFnPtr const& fnptr = *static_cast<MemFnPtr const*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(fnptr != 0);
            FuncTraits<MemFnPtr>::CallConst(t, fnptr, L);
            return 0;
        }
    };
    
    //--------------------------------------------------------------------------
    /**
     lua_CFunction to call a class member lua_CFunction.
     
     The member function pointer is in the first upvalue.
     The class userdata object is at the top of the Lua stack.
     */
    template<typename T>
    struct CallMemberCFunction
    {
        static int GeneratedFunction(lua_State* L)
        {
            assert(isfulluserdata(L, lua_upvalueindex(1)));
            typedef int(T::*MFP)(lua_State* L);
            T* const t = Userdata::Get<T>(L, 1, false);
            MFP const& fnptr = *static_cast<MFP const*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(fnptr != 0);
            return(t->*fnptr)(L);
        }
    };
    
    template<typename T>
    struct CallConstMemberCFunction
    {
        static int GeneratedFunction(lua_State* L)
        {
            assert(isfulluserdata(L, lua_upvalueindex(1)));
            typedef int(T::*MFP)(lua_State* L);
            T const* const t = Userdata::Get<T>(L, 1, true);
            MFP const& fnptr = *static_cast<MFP const*>(lua_touserdata(L, lua_upvalueindex(1)));
            assert(fnptr != 0);
            return(t->*fnptr)(L);
        }
    };
    
    //--------------------------------------------------------------------------
    
    // SFINAE Helpers
    
    template<typename MemFnPtr, bool IsConst>
    struct CallMemberFunctionHelper
    {
        static void AddFunction(lua_State* L, char const* name, MemFnPtr mf)
        {
            new(lua_newuserdata(L, sizeof(MemFnPtr))) MemFnPtr(mf);
            lua_pushcclosure(L, &CallConstMember<MemFnPtr>::GeneratedFunction, 1);
            lua_pushvalue(L, -1);
            rawsetfield(L, -5, name); // const table
            rawsetfield(L, -3, name); // class table
        }
    };
    
    template<typename MemFnPtr>
    struct CallMemberFunctionHelper<MemFnPtr, false>
    {
        static void AddFunction(lua_State* L, char const* name, MemFnPtr mf)
        {
            new(lua_newuserdata(L, sizeof(MemFnPtr))) MemFnPtr(mf);
            lua_pushcclosure(L, &CallMember<MemFnPtr>::GeneratedFunction, 1);
            rawsetfield(L, -3, name); // class table
        }
    };
    
    //--------------------------------------------------------------------------
    /**
     __gc metamethod for a class.
     */
    template<typename C>
    static int GCMetaMethod(lua_State* L)
    {
        Userdata* const ud = Userdata::GetExact<C>(L, 1);
        ud->~Userdata();
        return 0;
    }
    
    //--------------------------------------------------------------------------
    /**
     lua_CFunction to Get a class data member.
     
     The pointer-to-member is in the first upvalue.
     The class userdata object is at the top of the Lua stack.
     */
    template<typename C, typename T>
    static int GetProperty(lua_State* L)
    {
        C const* const c = Userdata::Get<C>(L, 1, true);
        T C::** mp = static_cast<T C::**>(lua_touserdata(L, lua_upvalueindex(1)));
        Stack<T>::Push(L, c->**mp);
        return 1;
    }
    
    //--------------------------------------------------------------------------
    /**
     lua_CFunction to set a class data member.
     
     The pointer-to-member is in the first upvalue.
     The class userdata object is at the top of the Lua stack.
     */
    template<typename C, typename T>
    static int SetProperty(lua_State* L)
    {
        C* const c = Userdata::Get<C>(L, 1, false);
        T C::** mp = static_cast<T C::**>(lua_touserdata(L, lua_upvalueindex(1)));
        c->**mp = Stack<T>::Get(L, 2);
        return 0;
    }
};
