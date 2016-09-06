#ifndef _LUA_PORTAL_H_
#define _LUA_PORTAL_H_
// All #include dependencies are listed here
// instead of in the individual header files.
//
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <functional>
#include <memory>
#include <type_traits>

namespace luaportal
{
    
    // Forward declaration
    template <typename T>
    struct Stack;
    
#include "impl/LuaHelpers.h"
#include "impl/TypeTraits.h"
#include "impl/FuncTraits.h"
#include "impl/CallableTraits.h"
#include "impl/ClassInfo.h"
#include "impl/Userdata.h"
#include "impl/Constructor.h"
#include "impl/Stack.h"
    
    class LuaRef;
    
#include "impl/LuaRef.h"
#include "impl/Iterator.h"
    
    //------------------------------------------------------------------------------
    /**
     security options.
     */
    class Security
    {
    public:
        static bool hideMetatables ()
        {
            return getSettings().hideMetatables;
        }
        
        static void setHideMetatables (bool shouldHide)
        {
            getSettings().hideMetatables = shouldHide;
        }
        
    private:
        struct Settings
        {
            Settings () : hideMetatables (true)
            {
            }
            
            bool hideMetatables;
        };
        
        static Settings& getSettings ()
        {
            static Settings settings;
            return settings;
        }
    };
    
#include "impl/CFunctions.h"
#include "impl/Namespace.h"
#include "impl/LuaState.h"
    
    //------------------------------------------------------------------------------
    /**
     Change whether or not metatables are hidden (on by default).
     */
    inline void setHideMetatables (bool shouldHide)
    {
        Security::setHideMetatables (shouldHide);
    }
    
}

#endif
