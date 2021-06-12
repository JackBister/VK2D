// This file is a bit of a mess right now. It combines header and implementation. It uses old style include guards
// because of this. This is because I want to link it directly in both core and DLLs.

#ifndef REFLECT_H
#define REFLECT_H

#include <memory>
#include <sstream>
#include <string>
#include <variant>
#include <vector>

#include <ThirdParty/glm/glm/glm.hpp>
#include <ThirdParty/glm/glm/gtc/quaternion.hpp>

class EditorNode
{
public:
    enum Flags { READONLY = 1 };

    enum Type { NULLPTR, TREE, BOOL, FLOAT, DOUBLE, INT, STRING };

    struct NullPtr {
        std::string label;
    };

    struct Tree {
        std::string label;
        std::vector<std::unique_ptr<EditorNode>> children;
    };

    struct Bool {
        std::string label;
        bool * v;
    };

    struct Float {
        std::string label;
        float * v;
        int extra_flags;
    };

    struct Double {
        std::string label;
        double * v;
        int extra_flags;
    };

    struct Int {
        std::string label;
        int * v;
        int extra_flags;
    };

    struct String {
        std::string label;
        std::string * v;
    };

    EditorNode(std::variant<NullPtr, Tree, Bool, Float, Double, Int, String> && node)
        : type(node.index()), node(std::move(node))
    {
    }

    size_t type;
    std::variant<NullPtr, Tree, Bool, Float, Double, Int, String> node;
};

namespace reflect
{

//--------------------------------------------------------
// Base class of all type descriptors
//--------------------------------------------------------

struct TypeDescriptor {
    const char * name;
    size_t size;

    TypeDescriptor(const char * name, size_t size) : name{name}, size{size} {}
    virtual ~TypeDescriptor() {}
    virtual std::string getFullName() const { return name; }
    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const = 0;
};

//--------------------------------------------------------
// Finding type descriptors
//--------------------------------------------------------

// Declare the function template that handles primitive types such as int, std::string, etc.:
template <typename T>
TypeDescriptor * getPrimitiveDescriptor();

// A helper class to find TypeDescriptors in different ways:
struct DefaultResolver {
    template <typename T>
    static char func(decltype(&T::Reflection));
    template <typename T>
    static int func(...);
    template <typename T>
    struct IsReflected {
        enum { value = (sizeof(func<T>(nullptr)) == sizeof(char)) };
    };

    template <typename T>
    static char f(decltype(&T::GetReflection));
    template <typename T>
    static int f(...);
    template <typename T>
    struct IsInheritanceReflected {
        enum { value = (sizeof(f<T>(nullptr)) == sizeof(char)) };
    };

    // This version is called if T has a static member named "Reflection":
    template <typename T, typename std::enable_if<IsReflected<T>::value, int>::type = 0>
    static TypeDescriptor * get()
    {
        return &T::Reflection;
    }

    // This version is called otherwise:
    template <typename T, typename std::enable_if<!IsReflected<T>::value, int>::type = 0>
    static TypeDescriptor * get()
    {
        return getPrimitiveDescriptor<T>();
    }

    template <typename T, typename std::enable_if<IsInheritanceReflected<T>::value, int>::type = 0>
    static TypeDescriptor * get(T * v)
    {
        return v->GetReflection();
    }

    template <typename T, typename std::enable_if<!IsInheritanceReflected<T>::value, int>::type = 0>
    static TypeDescriptor * get(T * v)
    {
        return get<T>();
    }
};

// This is the primary class template for finding all TypeDescriptors:
template <typename T>
struct TypeResolver {
    static TypeDescriptor * get() { return DefaultResolver::get<T>(); }

    static TypeDescriptor * get(T * v) { return DefaultResolver::get<T>(v); }
};

//--------------------------------------------------------
// Type descriptors for user-defined structs/classes
//--------------------------------------------------------
struct TypeDescriptor_Struct : TypeDescriptor {
    struct Member {
        const char * name;
        size_t offset;
        TypeDescriptor * type;
    };

    std::vector<Member> members;

    TypeDescriptor_Struct(void (*init)(TypeDescriptor_Struct *)) : TypeDescriptor{nullptr, 0} { init(this); }
    TypeDescriptor_Struct(const char * name, size_t size, const std::initializer_list<Member> & init)
        : TypeDescriptor{name, size}, members{init}
    {
    }
    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const override
    {
        EditorNode::Tree ret;
        ret.label = name;
        ret.label.append(" <").append(this->name).append(">");
        for (auto const & member : members) {
            ret.children.push_back(
                std::move(member.type->DrawEditorGui(member.name, (char *)obj + member.offset, isLocked)));
        }
        return std::make_unique<EditorNode>(std::move(ret));
    }
};

//--------------------------------------------------------
// Type descriptors for std::unique_ptr
//--------------------------------------------------------
template <typename TargetType>
struct TypeDescriptor_StdUniquePtr : TypeDescriptor {
    TypeDescriptor * targetType;
    const void * (*getTarget)(const void *);

    // Template constructor:
    TypeDescriptor_StdUniquePtr(TargetType * /* dummy argument */)
        : TypeDescriptor{"std::unique_ptr<>", sizeof(std::unique_ptr<TargetType>)}, targetType{
                                                                                        TypeResolver<TargetType>::get()}
    {
        getTarget = [](const void * uniquePtrPtr) -> const void * {
            const auto & uniquePtr = *(const std::unique_ptr<TargetType> *)uniquePtrPtr;
            return uniquePtr.get();
        };
    }

    virtual std::string getFullName() const override
    {
        return std::string("std::unique_ptr<") + targetType->getFullName() + ">";
    }

    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const override
    {
        void const * target = getTarget(obj);
        if (target == nullptr) {
            return std::make_unique<EditorNode>(EditorNode::NullPtr{name});
        }
        auto derived = TypeResolver<TargetType>::get((TargetType *)target);
        return derived->DrawEditorGui(name, target, isLocked);
    }
};

template <typename T>
class TypeResolver<std::unique_ptr<T>>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_StdUniquePtr<T> typeDesc{(T *)nullptr};
        return &typeDesc;
    }

    static TypeDescriptor * get(std::unique_ptr<T> &)
    {
        static TypeDescriptor_StdUniquePtr<T> typeDesc{(T *)nullptr};
        return &typeDesc;
    }
};

//--------------------------------------------------------
// Type descriptors for T *
//--------------------------------------------------------
template <typename TargetType>
struct TypeDescriptor_GenericPointer : TypeDescriptor {
    TypeDescriptor * targetType;
    const void * (*getTarget)(const void *);

    // Template constructor:
    TypeDescriptor_GenericPointer(TargetType * /* dummy argument */)
        : TypeDescriptor{"*", sizeof(TargetType *)}, targetType{TypeResolver<TargetType>::get()}
    {
        getTarget = [](const void * ptrPtr) -> const void * {
            const auto & ptr = *(const TargetType **)ptrPtr;
            return ptr;
        };
    }

    virtual std::string getFullName() const override { return targetType->getFullName() + " *"; }

    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const override
    {
        void const * target = getTarget(obj);
        if (target == nullptr) {
            return std::make_unique<EditorNode>(EditorNode::NullPtr{name});
        }
        auto derived = TypeResolver<TargetType>::get((TargetType *)target);
        return derived->DrawEditorGui(name, target, isLocked);
    }
};

template <typename T>
class TypeResolver<T *>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_GenericPointer<T> typeDesc{(T *)nullptr};
        return &typeDesc;
    }

    static TypeDescriptor * get(T *)
    {
        static TypeDescriptor_GenericPointer<T> typeDesc{(T *)nullptr};
        return &typeDesc;
    }
};

//--------------------------------------------------------
// Type descriptors for std::string
//--------------------------------------------------------
struct TypeDescriptor_StdString : TypeDescriptor {

    // Template constructor:
    TypeDescriptor_StdString() : TypeDescriptor{"std::string", sizeof(std::string)} {}

    virtual std::string getFullName() const override { return "std::string"; }

    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const override
    {
        return std::make_unique<EditorNode>(EditorNode::String{name, (std::string *)obj});
    }
};

template <>
class TypeResolver<std::string>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_StdString typeDesc;
        return &typeDesc;
    }
};

//--------------------------------------------------------
// Type descriptors for std::vector
//--------------------------------------------------------
template <typename ItemType>
struct TypeDescriptor_StdVector : TypeDescriptor {
    TypeDescriptor * itemType;
    size_t (*getSize)(const void *);
    const void * (*getItem)(const void *, size_t);

    TypeDescriptor_StdVector(ItemType *)
        : TypeDescriptor{"std::vector<>", sizeof(std::vector<ItemType>)}, itemType{TypeResolver<ItemType>::get()}
    {
        getSize = [](const void * vecPtr) -> size_t {
            const auto & vec = *(const std::vector<ItemType> *)vecPtr;
            return vec.size();
        };
        getItem = [](const void * vecPtr, size_t index) -> const void * {
            const auto & vec = *(const std::vector<ItemType> *)vecPtr;
            return &vec[index];
        };
    }
    virtual std::string getFullName() const override
    {
        return std::string("std::vector<") + itemType->getFullName() + ">";
    }

    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const override
    {
        EditorNode::Tree ret;
        ret.label = name;
        auto numItems = getSize(obj);
        for (size_t i = 0; i < numItems; ++i) {
            std::stringstream ss;
            ss << '[' << i << ']';
            auto derived = TypeResolver<ItemType>::get(*(ItemType *)getItem(obj, i));
            ret.children.push_back(std::move(derived->DrawEditorGui(ss.str().c_str(), getItem(obj, i), isLocked)));
        }
        return std::make_unique<EditorNode>(std::move(ret));
    }
};

// Partially specialize TypeResolver<> for std::vectors:
template <typename T>
class TypeResolver<std::vector<T>>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_StdVector<T> typeDesc{(T *)nullptr};
        return &typeDesc;
    }
};

//--------------------------------------------------------
// Type descriptors for glm classes
//--------------------------------------------------------
template <>
class TypeResolver<glm::ivec2>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_Struct typeDesc{"glm::ivec2",
                                              sizeof(glm::ivec2),
                                              {{"x", offsetof(glm::ivec2, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::ivec2, y), TypeResolver<float>::get()}}};
        return &typeDesc;
    }

    static TypeDescriptor * get(glm::ivec2 &)
    {
        static TypeDescriptor_Struct typeDesc{"glm::ivec2",
                                              sizeof(glm::ivec2),
                                              {{"x", offsetof(glm::ivec2, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::ivec2, y), TypeResolver<float>::get()}}};
        return &typeDesc;
    }
};

template <>
class TypeResolver<glm::vec2>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_Struct typeDesc{"glm::vec2",
                                              sizeof(glm::vec2),
                                              {{"x", offsetof(glm::vec2, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::vec2, y), TypeResolver<float>::get()}}};
        return &typeDesc;
    }

    static TypeDescriptor * get(glm::vec2 &)
    {
        static TypeDescriptor_Struct typeDesc{"glm::vec2",
                                              sizeof(glm::vec2),
                                              {{"x", offsetof(glm::vec2, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::vec2, y), TypeResolver<float>::get()}}};
        return &typeDesc;
    }
};

template <>
class TypeResolver<glm::vec3>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_Struct typeDesc{"glm::vec3",
                                              sizeof(glm::vec3),
                                              {{"x", offsetof(glm::vec3, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::vec3, y), TypeResolver<float>::get()},
                                               {"z", offsetof(glm::vec3, z), TypeResolver<float>::get()}}};
        return &typeDesc;
    }

    static TypeDescriptor * get(glm::vec3 &)
    {
        static TypeDescriptor_Struct typeDesc{"glm::vec3",
                                              sizeof(glm::vec3),
                                              {{"x", offsetof(glm::vec3, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::vec3, y), TypeResolver<float>::get()},
                                               {"z", offsetof(glm::vec3, z), TypeResolver<float>::get()}}};
        return &typeDesc;
    }
};

template <>
class TypeResolver<glm::vec4>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_Struct typeDesc{"glm::vec4",
                                              sizeof(glm::vec4),
                                              {{"x", offsetof(glm::vec4, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::vec4, y), TypeResolver<float>::get()},
                                               {"z", offsetof(glm::vec4, y), TypeResolver<float>::get()},
                                               {"w", offsetof(glm::vec4, w), TypeResolver<float>::get()}}};
        return &typeDesc;
    }

    static TypeDescriptor * get(glm::vec4 &)
    {
        static TypeDescriptor_Struct typeDesc{"glm::vec4",
                                              sizeof(glm::vec4),
                                              {{"x", offsetof(glm::vec4, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::vec4, y), TypeResolver<float>::get()},
                                               {"z", offsetof(glm::vec4, y), TypeResolver<float>::get()},
                                               {"w", offsetof(glm::vec4, w), TypeResolver<float>::get()}}};
        return &typeDesc;
    }
};

template <>
class TypeResolver<glm::quat>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_Struct typeDesc{"glm::quat",
                                              sizeof(glm::quat),
                                              {{"x", offsetof(glm::quat, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::quat, y), TypeResolver<float>::get()},
                                               {"z", offsetof(glm::quat, y), TypeResolver<float>::get()},
                                               {"w", offsetof(glm::quat, w), TypeResolver<float>::get()}}};
        return &typeDesc;
    }

    static TypeDescriptor * get(glm::quat &)
    {
        static TypeDescriptor_Struct typeDesc{"glm::quat",
                                              sizeof(glm::quat),
                                              {{"x", offsetof(glm::quat, x), TypeResolver<float>::get()},
                                               {"y", offsetof(glm::quat, y), TypeResolver<float>::get()},
                                               {"z", offsetof(glm::quat, y), TypeResolver<float>::get()},
                                               {"w", offsetof(glm::quat, w), TypeResolver<float>::get()}}};
        return &typeDesc;
    }
};

template <>
class TypeResolver<glm::mat4>
{
public:
    static TypeDescriptor * get()
    {
        static TypeDescriptor_Struct typeDesc{"glm::mat4",
                                              sizeof(glm::mat4),
                                              {// TODO: Dangerous hardcoded offsets
                                               {"[0]", 0, TypeResolver<glm::vec4>::get()},
                                               {"[1]", 16, TypeResolver<glm::vec4>::get()},
                                               {"[2]", 32, TypeResolver<glm::vec4>::get()},
                                               {"[3]", 48, TypeResolver<glm::vec4>::get()}}};
        return &typeDesc;
    }

    static TypeDescriptor * get(glm::mat4 &)
    {
        static TypeDescriptor_Struct typeDesc{"glm::mat4",
                                              sizeof(glm::mat4),
                                              {// TODO: Dangerous hardcoded offsets
                                               {"[0]", 0, TypeResolver<glm::vec4>::get()},
                                               {"[1]", 16, TypeResolver<glm::vec4>::get()},
                                               {"[2]", 32, TypeResolver<glm::vec4>::get()},
                                               {"[3]", 48, TypeResolver<glm::vec4>::get()}}};
        return &typeDesc;
    }
};

#define REFLECT_INHERITANCE()                                                                                          \
    virtual reflect::TypeDescriptor_Struct * GetReflection() { return &Reflection; }

#define REFLECT()                                                                                                      \
    friend struct reflect::DefaultResolver;                                                                            \
    static reflect::TypeDescriptor_Struct Reflection;                                                                  \
    static void initReflection(reflect::TypeDescriptor_Struct *);

#define REFLECT_STRUCT_BEGIN(type)                                                                                     \
    reflect::TypeDescriptor_Struct type::Reflection{type::initReflection};                                             \
    void type::initReflection(reflect::TypeDescriptor_Struct * typeDesc)                                               \
    {                                                                                                                  \
        using T = type;                                                                                                \
        typeDesc->name = #type;                                                                                        \
        typeDesc->size = sizeof(T);                                                                                    \
        typeDesc->members = {

#define REFLECT_STRUCT_MEMBER(name) {#name, offsetof(T, name), reflect::TypeResolver<decltype(T::name)>::get()},

#define REFLECT_STRUCT_END()                                                                                           \
    }                                                                                                                  \
    ;                                                                                                                  \
    }
}; // namespace reflect

#endif // REFLECT_H

#ifdef REFLECT_IMPL

namespace reflect
{
//--------------------------------------------------------
// Type descriptors for primitive types
//--------------------------------------------------------
struct TypeDescriptor_Bool : TypeDescriptor {
    TypeDescriptor_Bool() : TypeDescriptor{"bool", sizeof(bool)} {}

    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const override
    {
        return std::make_unique<EditorNode>(EditorNode::Bool{name, (bool *)obj});
    }
};

template <>
TypeDescriptor * getPrimitiveDescriptor<bool>()
{
    static TypeDescriptor_Bool typeDesc;
    return &typeDesc;
}

struct TypeDescriptor_Float : TypeDescriptor {
    TypeDescriptor_Float() : TypeDescriptor{"float", sizeof(float)} {}

    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const override
    {
        return std::make_unique<EditorNode>(
            EditorNode::Float{name, (float *)obj, isLocked ? EditorNode::Flags::READONLY : 0});
    }
};

template <>
TypeDescriptor * getPrimitiveDescriptor<float>()
{
    static TypeDescriptor_Float typeDesc;
    return &typeDesc;
}

struct TypeDescriptor_Double : TypeDescriptor {
    TypeDescriptor_Double() : TypeDescriptor{"double", sizeof(double)} {}

    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const override
    {
        return std::make_unique<EditorNode>(
            EditorNode::Double{name, (double *)obj, isLocked ? EditorNode::Flags::READONLY : 0});
    }
};

template <>
TypeDescriptor * getPrimitiveDescriptor<double>()
{
    static TypeDescriptor_Double typeDesc;
    return &typeDesc;
}

struct TypeDescriptor_Int : TypeDescriptor {
    TypeDescriptor_Int() : TypeDescriptor{"int", sizeof(int)} {}

    virtual std::unique_ptr<EditorNode> DrawEditorGui(char const * name, void const * obj, bool isLocked) const override
    {
        return std::make_unique<EditorNode>(
            EditorNode::Int{name, (int *)obj, isLocked ? EditorNode::Flags::READONLY : 0});
    }
};

template <>
TypeDescriptor * getPrimitiveDescriptor<int>()
{
    static TypeDescriptor_Int typeDesc;
    return &typeDesc;
}

}

#endif
