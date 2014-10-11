//
//  Universe.h
//  archetype
//
//  Created by Derek Jones on 2/10/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__Universe__
#define __archetype__Universe__

#include <string>
#include <map>
#include <memory>
#include <stack>
#include <stdexcept>

#include "IdIndex.h"
#include "StringIdIndex.h"
#include "Object.h"
#include "Value.h"
#include "TokenStream.h"
#include "Serialization.h"
#include "UserInput.h"
#include "UserOutput.h"

namespace archetype {

    class QuitGame : public std::runtime_error {
    public:
        QuitGame(): runtime_error("Exiting.") { }
    };

    typedef IdIndex<ObjectPtr> ObjectIndex;
    typedef std::map<int, int> IdentifierMap;

    class Universe {
    public:
        static const int NullObjectId = 0;
        static const int SystemObjectId = 1;
        static const int UserObjectsBeginAt = 2;

        struct Context {
            ObjectPtr selfObject;
            ObjectPtr senderObject;
            Value messageValue;
            ObjectPtr eachObject;

            Context();
            Context(const Context& c);
            Context& operator=(const Context& c);
            ~Context();
        };

        StringIdIndex Messages;
        StringIdIndex TextLiterals;
        StringIdIndex Identifiers;

        IdentifierMap ObjectIdentifiers;

        Context& currentContext() { return context_.top(); }
        void pushContext(const Context& context) { context_.push(context); }
        void popContext() { context_.pop(); }

        UserInput input() const { return input_; }
        void setInput(UserInput input) { input_ = input; }
        UserOutput output() const { return output_; }
        void setOutput(UserOutput output) { output_ = output; }

        int objectCount() const;
        ObjectPtr getObject(int object_id) const;
        ObjectPtr getObject(std::string identifier) const;
        ObjectPtr defineNewObject(int parent_id = 0);

        // The post-condition is that the referenced object is gone, and all existing
        // references to it will be UNDEFINED.
        void destroyObject(int object_id);

        void assignObjectIdentifier(const ObjectPtr& object, std::string identifier);
        void assignObjectIdentifier(const ObjectPtr& object, int identifier_id);

        bool identifierIsAssignedAs(int identifier_id, int object_id) const;

        bool make(TokenStream& t);

        static Universe& instance();
        static void destroy();

    private:
        ObjectIndex objects_;
        ObjectPtr   nullObject_;
        ObjectPtr   systemObject_;
        std::stack<Context> context_;
        UserInput  input_;
        UserOutput output_;

        static Universe* instance_;

        Universe();
        Universe(const Universe&) = delete;
        Universe& operator=(const Universe&) = delete;

        friend Storage& operator<<(Storage& out, const Universe& u);
        friend Storage& operator>>(Storage& in, Universe& u);
    };

    class ContextScope {
    public:
        ContextScope();
        ContextScope(const ContextScope&) = delete;
        ContextScope& operator=(const ContextScope&) = delete;
        ~ContextScope();

        Universe::Context* operator->()
        { return &(Universe::instance().currentContext()); }
    };

    Storage& operator<<(Storage& out, const IdentifierMap& m);
    Storage& operator>>(Storage&in, IdentifierMap& m);

    Storage& operator<<(Storage& out, const Universe& u);
    Storage& operator>>(Storage& in, Universe& u);

}

#endif /* defined(__archetype__Universe__) */
