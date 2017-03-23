/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef __EscargotContext__
#define __EscargotContext__

#include "runtime/AtomicString.h"
#include "runtime/Context.h"
#include "runtime/GlobalObject.h"
#include "runtime/RegExpObject.h"
#include "runtime/StaticStrings.h"
#include "runtime/String.h"

namespace WTF {
class BumpPointerAllocator;
}

namespace Escargot {

class VMInstance;
class ScriptParser;
class ObjectStructure;
class ControlFlowRecord;
class SandBox;
class JobQueue;
class ByteCodeBlock;

class Context : public gc {
    friend class AtomicString;
    friend class SandBox;
    friend class ByteCodeInterpreter;
    friend class OpcodeTable;

public:
    Context(VMInstance* instance);

    VMInstance* vmInstance()
    {
        return m_instance;
    }

    const StaticStrings& staticStrings()
    {
        return m_staticStrings;
    }

    AtomicStringMap* atomicStringMap()
    {
        return m_atomicStringMap;
    }

    ScriptParser& scriptParser()
    {
        return *m_scriptParser;
    }

    RegExpCacheMap* regexpCache()
    {
        return m_regexpCache;
    }

    WTF::BumpPointerAllocator* bumpPointerAllocator()
    {
        return m_bumpPointerAllocator;
    }

    ObjectStructure* defaultStructureForObject()
    {
        return m_defaultStructureForObject;
    }

    ObjectStructure* defaultStructureForFunctionObject()
    {
        return m_defaultStructureForFunctionObject;
    }

    ObjectStructure* defaultStructureForNotConstructorFunctionObject()
    {
        return m_defaultStructureForNotConstructorFunctionObject;
    }

    ObjectStructure* defaultStructureForFunctionObjectInStrictMode()
    {
        return m_defaultStructureForFunctionObjectInStrictMode;
    }

    ObjectStructure* defaultStructureForNotConstructorFunctionObjectInStrictMode()
    {
        return m_defaultStructureForNotConstructorFunctionObjectInStrictMode;
    }

    ObjectStructure* defaultStructureForBuiltinFunctionObject()
    {
        return m_defaultStructureForBuiltinFunctionObject;
    }

    ObjectStructure* defaultStructureForFunctionPrototypeObject()
    {
        return m_defaultStructureForFunctionPrototypeObject;
    }

    ObjectStructure* defaultStructureForArrayObject()
    {
        return m_defaultStructureForArrayObject;
    }

    ObjectStructure* defaultStructureForStringObject()
    {
        return m_defaultStructureForStringObject;
    }

    ObjectStructure* defaultStructureForRegExpObject()
    {
        return m_defaultStructureForRegExpObject;
    }

    ObjectStructure* defaultStructureForArgumentsObject()
    {
        return m_defaultStructureForArgumentsObject;
    }

    ObjectStructure* defaultStructureForArgumentsObjectInStrictMode()
    {
        return m_defaultStructureForArgumentsObjectInStrictMode;
    }

    GlobalObject* globalObject()
    {
        return m_globalObject;
    }

    void throwException(ExecutionState& state, const Value& exception);

#if ESCARGOT_ENABLE_PROMISE
    JobQueue* jobQueue()
    {
        return m_jobQueue;
    }
#endif

    Vector<CodeBlock*, GCUtil::gc_malloc_ignore_off_page_allocator<CodeBlock*>>& compiledCodeBlocks()
    {
        return m_compiledCodeBlocks;
    }

protected:
    VMInstance* m_instance;

    // these data actually store in VMInstance
    // we can store pointers of these data for reducing memory dereference to VMInstance
    AtomicStringMap* m_atomicStringMap;
    StaticStrings& m_staticStrings;
    GlobalObject* m_globalObject;
    ScriptParser* m_scriptParser;
    Vector<CodeBlock*, GCUtil::gc_malloc_ignore_off_page_allocator<CodeBlock*>>& m_compiledCodeBlocks;
    WTF::BumpPointerAllocator* m_bumpPointerAllocator;
    RegExpCacheMap* m_regexpCache;
    ObjectStructure* m_defaultStructureForObject;
    ObjectStructure* m_defaultStructureForFunctionObject;
    ObjectStructure* m_defaultStructureForNotConstructorFunctionObject;
    ObjectStructure* m_defaultStructureForFunctionObjectInStrictMode;
    ObjectStructure* m_defaultStructureForNotConstructorFunctionObjectInStrictMode;
    ObjectStructure* m_defaultStructureForBuiltinFunctionObject;
    ObjectStructure* m_defaultStructureForFunctionPrototypeObject;
    ObjectStructure* m_defaultStructureForArrayObject;
    ObjectStructure* m_defaultStructureForStringObject;
    ObjectStructure* m_defaultStructureForRegExpObject;
    ObjectStructure* m_defaultStructureForArgumentsObject;
    ObjectStructure* m_defaultStructureForArgumentsObjectInStrictMode;
    Vector<SandBox*, GCUtil::gc_malloc_allocator<SandBox*>>& m_sandBoxStack;
#if ESCARGOT_ENABLE_PROMISE
    JobQueue* m_jobQueue;
#endif
};
}

#endif
