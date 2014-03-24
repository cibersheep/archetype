//
//  Universe.cpp
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#include <iostream>
#include <cassert>

using namespace std;

#include "Universe.h"

namespace archetype {
    Universe* Universe::instance_ = nullptr;
    Universe& Universe::instance() {
        if (not instance_) {
            instance_ = new Universe();
        }
        return *instance_;
    }
    
    void Universe::destroy() {
        delete instance_;
        instance_ = nullptr;
    }
    
    Universe::Universe() {
        // TODO: better sentinel?  This feels fishy.
        // TODO: What if the original context is self:undefined, sender:undefined, message:undefined?
        // TODO: for most programs, self:main, sender:system, message:'START' (good, but maybe not the bottom)
        Context context;
        context.selfObject = ObjectPtr(new Object);
        context.senderObject = ObjectPtr(new Object);
        context.messageId = 0;
        context_.push(context);
        
        output_ = &std::cout;
    }
    
    std::ostream& Universe::output() {
        return *output_;
    }
    
    void Universe::setOutput(std::ostream &out) {
        output_ = &out;
    }
    
    ObjectPtr Universe::getObject(int object_id) const {
        // TODO:  I hate this lookup-and-find pattern.  How about a nice find() for IdIndex?
        if (objects_.hasIndex(object_id)) {
            return objects_.get(object_id);
        } else {
            return nullptr;
        }
    }
    
    ObjectPtr Universe::defineNewObject(int parent_id) {
        ObjectPtr obj(new Object(parent_id));
        int object_id = objects_.index(std::move(obj));
        obj->setId(object_id);
        return objects_.get(object_id);
    }
    
    void Universe::assignObjectIdentifier(const ObjectPtr& object, std::string name) {
        int identifier_id_for_object = Identifiers.index(name);
        assignObjectIdentifier(object, identifier_id_for_object);
    }
    
    void Universe::assignObjectIdentifier(const ObjectPtr& object, int identifier_id) {
        int object_id = object->id();
        ObjectIdentifiers[identifier_id] = object_id;
    }
    
    static ObjectPtr declare_object(TokenStream& t, ObjectPtr obj) {
        while (t.fetch()) {
            if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_END)) {
                return obj;
            }
            if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_METHODS)) {
                break;
            }
            if (t.token().type() != Token::IDENTIFIER) {
                t.expectGeneral("attribute identifier");
                return nullptr;
            }
            int attribute_id = t.token().number();
            t.insistOn(Token(Token::PUNCTUATION, ':'));
            Expression expr = make_expr(t);
            if (not expr) {
                return nullptr;
            }
            obj->setAttribute(attribute_id, std::move(expr));
        }
        if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_METHODS)) {
            while (t.fetch()) {
                if (t.token() == Token(Token::RESERVED_WORD, Keywords::RW_END)) {
                    return obj;
                }
                // TODO: Need to handle 'default' too
                if (t.token().type() != Token::MESSAGE) {
                    t.expectGeneral("message literal");
                    return nullptr;
                }
                int message_id = t.token().number();
                t.insistOn(Token(Token::PUNCTUATION, ':'));
                Statement stmt = make_statement(t);
                if (not stmt) {
                    return nullptr;
                }
                obj->setMethod(message_id, std::move(stmt));
            }
        }
        return nullptr;
    }
    
    static ObjectPtr instantiate(TokenStream& t, ObjectPtr parent = nullptr) {
        if (not t.fetch() or t.token().type() != Token::IDENTIFIER) {
            t.expectGeneral("name of new object");
            return nullptr;
        }
        ObjectPtr obj = Universe::instance().defineNewObject();
        Universe::instance().assignObjectIdentifier(obj, t.token().number());
        if (parent) {
            obj->setParentId(parent->id());
        }
        return declare_object(t, obj);
    }
    
    static ObjectPtr define_type(TokenStream& t) {
        if (not (t.fetch() and t.token().type() == Token::IDENTIFIER)) {
            t.expectGeneral("name of new type");
            return nullptr;
        }
        ObjectPtr obj = Universe::instance().defineNewObject();
        obj->setPrototype(true);
        Universe::instance().assignObjectIdentifier(obj, t.token().number());
        t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_BASED));
        t.insistOn(Token(Token::RESERVED_WORD, Keywords::RW_ON));
        if (not t.fetch()) {
            t.expectGeneral("name of a previously defined type");
            return nullptr;
        }
        if (t.token().type() == Token::IDENTIFIER) {
            auto parent_id_p = Universe::instance().ObjectIdentifiers.find(t.token().number());
            if (parent_id_p == Universe::instance().ObjectIdentifiers.end()) {
                t.expectGeneral("name of a previously defined type");
                return nullptr;
            }
            ObjectPtr parent = Universe::instance().getObject(parent_id_p->second);
            if (not parent->isPrototype()) {
                t.expectGeneral("name of a previously defined type, not the name of an instance");
                return nullptr;
            }
            obj->setParentId(parent->id());
        } else if (t.token() != Token(Token::RESERVED_WORD, Keywords::RW_NULL)) {
            t.expectGeneral("name of a previously defined type");
            return nullptr;
        }
        return declare_object(t, obj);
    }
    
    bool Universe::make(TokenStream& t) {
        while (t.fetch()) {
            if (t.token().type() == Token::RESERVED_WORD) {
                switch (Keywords::Reserved_e(t.token().number())) {
                    case Keywords::RW_TYPE:
                    case Keywords::RW_CLASS:
                        if (not define_type(t)) {
                            return false;
                        } else {
                            break;
                        }
                    case Keywords::RW_NULL:
                        if (not instantiate(t)) {
                            return false;
                        } else {
                            break;
                        }
                    case Keywords::RW_KEYWORD:
                        // TODO: ???
                        break;
                    default:
                        t.expected(Token(Token::RESERVED_WORD, Keywords::RW_TYPE));
                        return false;
                }
            } else if (t.token().type() == Token::IDENTIFIER) {
                int id_number = t.token().number();
                auto which = Universe::instance().ObjectIdentifiers.find(id_number);
                if (which == Universe::instance().ObjectIdentifiers.end()) {
                    t.errorMessage("Require name of defined type");
                    return false;
                } else {
                    ObjectPtr type_object = Universe::instance().getObject(which->second);
                    if (type_object and type_object->isPrototype()) {
                        if (not instantiate(t, type_object)) {
                            return false;
                        }
                    }
                }
            } else {
                t.expectGeneral("Need a type declaration or object instantiation");
                return false;
            }
        }
        return true;
    }
    
    SelfScope::SelfScope(ObjectPtr new_self_obj) {
        Universe::instance().pushContext(Universe::instance().currentContext());
        Universe::instance().currentContext().selfObject = new_self_obj;
    }
    
    SelfScope::~SelfScope() {
        Universe::instance().popContext();
    }
    
    MessageScope::MessageScope(int message_id) {
        Universe::instance().pushContext(Universe::instance().currentContext());
        Universe::instance().currentContext().messageId = message_id;
    }
    
    MessageScope::~MessageScope() {
        Universe::instance().popContext();
    }
}