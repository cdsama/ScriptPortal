/** Unique Lua registry keys for a class.
 
 Each registered class inserts three keys into the registry, whose
 Values are the corresponding static, class, and const metatables. This
 allows a quick and reliable lookup for a metatable from a template type.
 */
template<typename T>
class ClassInfo
{
public:
    /** Get the key for the static table.
     
     The static table holds the static data members, static properties, and
     static member functions for a class.
     */
    static void const* GetStaticKey()
    {
        static char Value;
        return &Value;
    }
    
    /** Get the key for the class table.
     
     The class table holds the data members, properties, and member functions
     of a class. Read-only data and properties, and const member functions are
     also placed here(to save a lookup in the const table).
     */
    static void const* GetClassKey()
    {
        static char Value;
        return &Value;
    }
    
    /** Get the key for the const table.
     
     The const table holds read-only data members and properties, and const
     member functions of a class.
     */
    static void const* GetConstKey()
    {
        static char Value;
        return &Value;
    }
};

