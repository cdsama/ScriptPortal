#pragma once
//------------------------------------------------------------------------------
/**
 Container traits.
 
 Unspecialized ContainerTraits has the isNotContainer typedef for SFINAE.
 All user defined containers must supply an appropriate specialization for
 ContinerTraits (without the typedef isNotContainer). The containers that
 come with LuaPortal also come with the appropriate ContainerTraits
 specialization. See the corresponding declaration for details.
 
 A specialization of ContainerTraits for some generic type ContainerType
 looks like this:
 
 template <typename T>
 struct ContainerTraits <ContainerType <T> >
 {
 typedef typename T Type;
 
 static T* get (ContainerType <T> const& c)
 {
 return c.get (); // Implementation-dependent on ContainerType
 }
 };
 */
template <typename T>
struct ContainerTraits
{
    typedef bool isNotContainer;
};

//------------------------------------------------------------------------------
/**
 Type traits.
 
 Specializations return information about a type.
 */
struct TypeTraits
{
    /** Determine if type T is a container.
     
     To be considered a container, there must be a specialization of
     ContainerTraits with the required fields.
     */
    template <typename T>
    class isContainer
    {
    private:
        typedef char yes[1]; // sizeof (yes) == 1
        typedef char no [2]; // sizeof (no)  == 2
        
        template <typename C>
        static no& test (typename C::isNotContainer*);
        
        template <typename>
        static yes& test (...);
        
    public:
        static const bool value = sizeof (test <ContainerTraits <T> >(0)) == sizeof (yes);
    };
    
    /** Determine if T is const qualified.
     */
    /** @{ */
    template <typename T>
    struct isConst
    {
        static bool const value = false;
    };
    
    template <typename T>
    struct isConst <T const>
    {
        static bool const value = true;
    };
    /** @} */
    
    /** Remove the const qualifier from T.
     */
    /** @{ */
    template <typename T>
    struct removeConst
    {
        typedef T Type;
    };
    
    template <typename T>
    struct removeConst <T const>
    {
        typedef T Type;
    };
    /**@}*/
    
    template <typename T>
    static T* getPtr(const T& t) {
        return (T*)&t;
    }
};
