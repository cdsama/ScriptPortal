/** Unique Lua registry keys for a class.
 
 Each registered class inserts three keys into the registry, whose
 values are the corresponding static, class, and const metatables. This
 allows a quick and reliable lookup for a metatable from a template type.
 */
template <typename T>
class ClassInfo
{
public:
    /** Get the key for the static table.
     
     The static table holds the static data members, static properties, and
     static member functions for a class.
     */
    static void const* getStaticKey ()
    {
        static char value;
        return &value;
    }
    
    /** Get the key for the class table.
     
     The class table holds the data members, properties, and member functions
     of a class. Read-only data and properties, and const member functions are
     also placed here (to save a lookup in the const table).
     */
    static void const* getClassKey ()
    {
        static char value;
        return &value;
    }
    
    /** Get the key for the const table.
     
     The const table holds read-only data members and properties, and const
     member functions of a class.
     */
    static void const* getConstKey ()
    {
        static char value;
        return &value;
    }
};

