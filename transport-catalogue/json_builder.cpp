#include "json_builder.h"

namespace json {

void Builder::InsertValue(Node::Value value) {
    if (status_ == Context::DictValue) {
        auto& dict = std::get<Dict>(node_stack_.back()->GetValue());
        dict[std::move(keys_stack_.back())] = Node(std::move(value));
        keys_stack_.pop_back();
        status_ = Context::DictKey;
    } else if (status_ == Context::Array) {
        auto& array = std::get<Array>(node_stack_.back()->GetValue());
        array.emplace_back(std::move(value));
       
    } else if (status_ == Context::Empty) {
        node_ = Node(std::move(value));
        status_ = Context::Complete;
    } else {
        throw std::logic_error("InsertValue in invalid context");
    }
}
 Builder& Builder::Value(Node::Value value){
    if (status_ != Context::DictValue && status_ != Context::Array && status_ != Context::Empty) {
      throw std::logic_error("Value called in invalid context");
  }
    switch (status_)
    {
    case  Context::Empty:{
        node_ = Node(std::move(value));
        status_ = Context::Complete;
      break;
    }
    case  Context::DictValue:{
        if (node_stack_.empty() || keys_stack_.empty()){
          throw std::logic_error("Value is expected but empty stack has been found");
        }
        auto& dict = std::get<Dict>(node_stack_.back()->GetValue());
        dict[std::move(keys_stack_.back())] = Node(std::move(value));
        keys_stack_.pop_back();
        status_ = Context::DictKey;
      break;
    }
    case  Context::Array:{
        auto& array = std::get<Array>(node_stack_.back()->GetValue());
        array.emplace_back(std::move(value));
      break;
    }
    default:
         throw std::logic_error("Something went wrong with the context");
      break;
    }
    return *this;
  }
KeyItemContext Builder::StartDict() {
    if (status_ != Context::Empty && status_ != Context::Array && status_ != Context::DictValue) {
        throw std::logic_error("StartDict: invalid context");
    }

    Node dict_node = Dict{};
    switch (status_) {
    case Context::Empty:
        node_ = std::move(dict_node);
        node_stack_.push_back(&node_);
        break;
    case Context::Array: {
        auto& array = std::get<Array>(node_stack_.back()->GetValue());
        array.emplace_back(Dict{});
        node_stack_.push_back(&array.back());
        break;
    }
    case Context::DictValue: {
        auto& dict = std::get<Dict>(node_stack_.back()->GetValue());
        std::string key = std::move(keys_stack_.back());
        keys_stack_.pop_back();
        dict[key] = std::move(dict_node);
        node_stack_.push_back(&dict[key]);
        break;
    }
    default:
        throw std::logic_error("StartDict: unexpected context");
    }

    status_ = Context::DictKey;
    return KeyItemContext(*this);
}

ArrayItemContext Builder::StartArray() {
    if (status_ != Context::Empty && status_ != Context::Array && status_ != Context::DictValue) {
        throw std::logic_error("StartArray: invalid context");
    }

    Node array_node = Array{};
    switch (status_) {
    case Context::Empty:
        node_ = std::move(array_node);
        node_stack_.push_back(&node_);
        break;
    case Context::Array: {
        auto& array = std::get<Array>(node_stack_.back()->GetValue());
        array.emplace_back(Array{});
        node_stack_.push_back(&array.back());
        break;
    }
    case Context::DictValue: {
        auto& dict = std::get<Dict>(node_stack_.back()->GetValue());
        std::string key = std::move(keys_stack_.back());
        keys_stack_.pop_back();
        dict[key] = std::move(array_node);
        node_stack_.push_back(&dict[key]);
        break;
    }
    default:
        throw std::logic_error("StartArray: unexpected context");
    }

    status_ = Context::Array;
    return ArrayItemContext(*this);
}



Builder& Builder::Key(std::string key) {
    if (status_ != Context::DictKey) {
        throw std::logic_error("Key: not in DictKey state");
    }
    keys_stack_.push_back(std::move(key));
    status_ = Context::DictValue;
    return *this;
}


Builder& Builder::EndDict() {
        if (status_ != Context::DictKey || node_stack_.empty()) {
        throw std::logic_error("EndDict: invalid");
    }

    node_stack_.pop_back();

    if (node_stack_.empty()) {
        status_ = Context::Complete;
    } else if (std::holds_alternative<Array>(node_stack_.back()->GetValue())) {
        status_ = Context::Array;
    } else {
        status_ = Context::DictKey;
    }

    return *this;
}

Builder& Builder::EndArray() {
    if (status_ != Context::Array || node_stack_.empty()) {
        throw std::logic_error("EndArray: invalid");
    }

    node_stack_.pop_back();

    if (node_stack_.empty()) {
        status_ = Context::Complete;
    } else if (std::holds_alternative<Array>(node_stack_.back()->GetValue())) {
        status_ = Context::Array;
    } else {
        status_ = Context::DictKey;
    }

    return *this;
}
Builder& ArrayItemContext::EndArray() {
    if (builder_.status_ != Context::Array || builder_.node_stack_.empty()) {
        throw std::logic_error("EndArray: invalid");
    }

    builder_.node_stack_.pop_back();

    if (builder_.node_stack_.empty()) {
        builder_.status_ = Context::Complete;
    } else if (std::holds_alternative<Array>(builder_.node_stack_.back()->GetValue())) {
        builder_.status_ = Context::Array;
    } else {
        builder_.status_ = Context::DictKey;
    }

    return builder_;
}

Node Builder::Build() {
    if (status_ != Context::Complete) {
        throw std::logic_error("Build: not complete");
    }
    return node_;
}

// === KeyItemContext ===

ValueAfterKeyContext KeyItemContext::Key(std::string key) {
    if (builder_.status_ != Context::DictKey) {
        throw std::logic_error("Key: not in DictKey state");
    }
    builder_.keys_stack_.push_back(std::move(key));
    builder_.status_ = Context::DictValue;
    return ValueAfterKeyContext(builder_);
}

Builder& KeyItemContext::EndDict() {
    if (builder_.status_ != Context::DictKey || builder_.node_stack_.empty()) {
        throw std::logic_error("EndDict: invalid");
    }
    builder_.node_stack_.pop_back();

    if (builder_.node_stack_.empty()) {
        builder_.status_ = Context::Complete;
    } else if (std::holds_alternative<Array>(builder_.node_stack_.back()->GetValue())) {
        builder_.status_ = Context::Array;
    } else {
        builder_.status_ = Context::DictKey;
    }

    return builder_;
}

// === ValueAfterKeyContext ===

KeyItemContext ValueAfterKeyContext::Value(Node::Value value) {
    builder_.InsertValue(std::move(value));
    return KeyItemContext(builder_);
}

KeyItemContext ValueAfterKeyContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContext ValueAfterKeyContext::StartArray() {
    return builder_.StartArray();
}

// === ArrayItemContext ===

ArrayItemContext ArrayItemContext::Value(Node::Value value) {
    builder_.InsertValue(std::move(value));
    return *this;
}

KeyItemContext ArrayItemContext::StartDict() {
    return builder_.StartDict();
}

ArrayItemContext ArrayItemContext::StartArray() {
    return builder_.StartArray();
}


}  // namespace json
