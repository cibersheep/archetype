//
//  ConsoleInput.h
//  archetype
//
//  Created by Derek Jones on 9/3/14.
//  Copyright (c) 2014 Derek Jones. All rights reserved.
//

#ifndef __archetype__ConsoleInput__
#define __archetype__ConsoleInput__

#include <iostream>

#include "UserInput.h"

namespace archetype {
    class ConsoleInput : public IUserInput {
    public:
        virtual ~ConsoleInput() { }
        virtual char getKey() override {
            // TODO:  Totally not good enough.
            char key;
            std::cin >> key;
            return key;
        }
        virtual std::string getLine() override {
            std::string line;
            if (std::getline(std::cin, line)) {
                // TODO:  Uh oh.  This prevents "animal" from working correctly
                // TODO:  But without it, the REPL can't detect end-of-file
                // TODO:  I think IUserInput needs an EOF inspector
                return line + "\n";
            } else {
                return line;
            }
        }
    };
}

#endif /* defined(__archetype__ConsoleInput__) */
