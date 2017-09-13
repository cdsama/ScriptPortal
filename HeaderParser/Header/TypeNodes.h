#pragma once

#include <vector>
#include <memory>

namespace hp
{
    struct TypeNode
    {
        enum class Type
        {
            Pointer,
            Reference,
            LReference,
            Literal,
            Template,
            Function
        };

        TypeNode(Type t) :
            type(t) {}

        bool isConst = false;
        bool isVolatile = false;
        bool isMutable = false;
        bool isStatic = false;

        Type type;
    };

    struct PointerNode : public TypeNode
    {
        PointerNode(std::unique_ptr<TypeNode> && b) :
            TypeNode(TypeNode::Type::Pointer),
            base(std::forward<std::unique_ptr<TypeNode>>(b)) {}

        std::unique_ptr<TypeNode> base;
    };

    struct ReferenceNode : public TypeNode
    {
        ReferenceNode(std::unique_ptr<TypeNode> && b) :
            TypeNode(TypeNode::Type::Reference),
            base(std::forward<std::unique_ptr<TypeNode>>(b)) {}

        std::unique_ptr<TypeNode> base;
    };

    struct LReferenceNode : public TypeNode
    {
        LReferenceNode(std::unique_ptr<TypeNode> && b) :
            TypeNode(TypeNode::Type::LReference),
            base(std::forward<std::unique_ptr<TypeNode>>(b)) {}

        std::unique_ptr<TypeNode> base;
    };

    struct TemplateNode : public TypeNode
    {
        TemplateNode(const std::string& n) :
            TypeNode(TypeNode::Type::Template),
            name(n) {}

        std::string name;
        std::vector<std::unique_ptr<TypeNode>> parameters;
    };

    struct LiteralNode : public TypeNode
    {
        LiteralNode(const std::string& ref) :
            TypeNode(TypeNode::Type::Literal),
            name(ref) {}

        std::string name;
    };

    struct FunctionNode : public TypeNode
    {
        FunctionNode() : TypeNode(TypeNode::Type::Function) {}

        struct Parameter
        {
            std::string name;
            std::unique_ptr<TypeNode> type;
        };


        std::unique_ptr<TypeNode> returns;
        std::vector<std::unique_ptr<Parameter>> parameters;
    };

    struct ITypeNodeVisitor
    {
        virtual void VisitNode(TypeNode &node) = 0;
        virtual void Visit(PointerNode& node) = 0;
        virtual void Visit(ReferenceNode& node) = 0;
        virtual void Visit(LReferenceNode& node) = 0;
        virtual void Visit(LiteralNode& node) = 0;
        virtual void Visit(TemplateNode& node) = 0;
        virtual void Visit(FunctionNode& node) = 0;
    };

    struct TypeNodeVisitor : public ITypeNodeVisitor
    {
        void VisitNode(TypeNode &node) override
        {
            switch (node.type)
            {
            case TypeNode::Type::Pointer:
                Visit(reinterpret_cast<PointerNode&>(node));
                break;
            case TypeNode::Type::Reference:
                Visit(reinterpret_cast<ReferenceNode&>(node));
                break;
            case TypeNode::Type::LReference:
                Visit(reinterpret_cast<LReferenceNode&>(node));
                break;
            case TypeNode::Type::Literal:
                Visit(reinterpret_cast<LiteralNode&>(node));
                break;
            case TypeNode::Type::Template:
                Visit(reinterpret_cast<TemplateNode&>(node));
                break;
            case TypeNode::Type::Function:
                Visit(reinterpret_cast<FunctionNode&>(node));
                break;
            }
        }
    };
}