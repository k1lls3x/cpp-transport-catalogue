#pragma once

#include "json.h"

#include <string>
#include <vector>
#include <stdexcept>
#include <utility>

namespace json {

class Builder;

// === Базовый шаблонный класс для контекстов ===

template <typename Derived>
class ContextBase {
public:
    Builder& builder_;
    explicit ContextBase(Builder& builder) : builder_(builder) {}
};

// === Основной класс Builder и контексты ===

class Builder {
public:
    class KeyItemContext;
    class ValueAfterKeyContext;
    class ArrayItemContext;

    Builder() = default;

    KeyItemContext StartDict();
    ArrayItemContext StartArray();
    Builder& Value(Node::Value value);
    Builder& Key(const std::string& key);
    Builder& EndDict();
    Builder& EndArray();
    Node Build();

    class KeyItemContext : public ContextBase<KeyItemContext> {
    public:
        using ContextBase::ContextBase;
        ValueAfterKeyContext Key(const std::string& );
        Builder& EndDict();

        // запреты
        Builder& EndArray() = delete;
        KeyItemContext StartDict() = delete;
        ArrayItemContext StartArray() = delete;
        KeyItemContext Value(Node::Value) = delete;
    };

    class ValueAfterKeyContext : public ContextBase<ValueAfterKeyContext> {
    public:
        using ContextBase::ContextBase;
        KeyItemContext Value(Node::Value value);
        KeyItemContext StartDict();
        ArrayItemContext StartArray();

        // запреты
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
        ValueAfterKeyContext Key(std::string) = delete;
    };

    class ArrayItemContext : public ContextBase<ArrayItemContext> {
    public:
        using ContextBase::ContextBase;
        ArrayItemContext Value(Node::Value value);
        KeyItemContext StartDict();
        ArrayItemContext StartArray();
        Builder& EndArray();

        // запреты
        Builder& EndDict() = delete;
        ArrayItemContext Key(std::string) = delete;
    };

private:
    Node node_;
    std::vector<Node*> node_stack_;
    std::vector<std::string> keys_stack_;
    enum class Context {
        Empty,
        DictKey,
        DictValue,
        Array,
        Complete
    };
    Context status_ = Context::Empty;

    void InsertValue(Node::Value value);
};

}  // namespace json
