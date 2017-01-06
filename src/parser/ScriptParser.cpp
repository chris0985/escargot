#include "Escargot.h"
#include "ScriptParser.h"
#include "runtime/Context.h"
#include "interpreter/ByteCode.h"
#include "parser/esprima_cpp/esprima.h"
#include "parser/ast/AST.h"
#include "parser/CodeBlock.h"

namespace Escargot {

ScriptParser::ScriptParser(Context* c)
    : m_context(c)
{
}

CodeBlock* ScriptParser::generateCodeBlockTreeFromASTWalker(Context* ctx, StringView source, Script* script, ASTScopeContext* scopeCtx, CodeBlock* parentCodeBlock)
{
    CodeBlock* codeBlock;
    if (parentCodeBlock == nullptr) {
        // globalBlock
        codeBlock = new CodeBlock(ctx, script, source, scopeCtx->m_isStrict, scopeCtx->m_locStart, scopeCtx->m_names,
                                  (CodeBlock::CodeBlockInitFlag)((scopeCtx->m_hasEval ? CodeBlock::CodeBlockHasEval : 0)
                                                                 | (scopeCtx->m_hasWith ? CodeBlock::CodeBlockHasWith : 0)
                                                                 | (scopeCtx->m_hasCatch ? CodeBlock::CodeBlockHasCatch : 0)
                                                                 | (scopeCtx->m_hasYield ? CodeBlock::CodeBlockHasYield : 0)));
    } else {
        codeBlock = new CodeBlock(ctx, script, StringView(source, scopeCtx->m_locStart.index, scopeCtx->m_locEnd.index),
                                  scopeCtx->m_locStart,
                                  scopeCtx->m_isStrict, scopeCtx->m_nodeStartIndex,
                                  scopeCtx->m_functionName, scopeCtx->m_parameters, scopeCtx->m_names, parentCodeBlock,
                                  (CodeBlock::CodeBlockInitFlag)((scopeCtx->m_hasEval ? CodeBlock::CodeBlockHasEval : 0)
                                                                 | (scopeCtx->m_hasWith ? CodeBlock::CodeBlockHasWith : 0)
                                                                 | (scopeCtx->m_hasCatch ? CodeBlock::CodeBlockHasCatch : 0)
                                                                 | (scopeCtx->m_hasYield ? CodeBlock::CodeBlockHasYield : 0)
                                                                 | (scopeCtx->m_associateNode->type() == FunctionExpression ? CodeBlock::CodeBlockIsFunctionExpression : 0)
                                                                 | (scopeCtx->m_associateNode->type() == FunctionDeclaration ? CodeBlock::CodeBlockIsFunctionDeclaration : 0)));
    }

#ifndef NDEBUG
    codeBlock->m_locStart = scopeCtx->m_locStart;
    codeBlock->m_locEnd = scopeCtx->m_locEnd;
    codeBlock->m_scopeContext = scopeCtx;
#endif

    if (parentCodeBlock) {
        if (codeBlock->hasEvalWithCatchYield()) {
            CodeBlock* c = codeBlock;
            while (c) {
                c->notifySelfOrChildHasEvalWithCatchYield();
                c = c->parentCodeBlock();
            }
        }

        bool hasCapturedIdentifier = false;
        AtomicString arguments = ctx->staticStrings().arguments;
        for (size_t i = 0; i < scopeCtx->m_usingNames.size(); i++) {
            AtomicString uname = scopeCtx->m_usingNames[i];
            if (uname == arguments) {
                codeBlock->m_usesArgumentsObject = true;
                if (!codeBlock->hasName(arguments)) {
                    CodeBlock::IdentifierInfo info;
                    info.m_indexForIndexedStorage = SIZE_MAX;
                    info.m_name = arguments;
                    info.m_needToAllocateOnStack = false;
                    codeBlock->m_identifierInfos.pushBack(info);
                } else {
                    codeBlock->m_hasArgumentsBinding = true;
                }
                CodeBlock* b = codeBlock;
                while (b) {
                    b->notifySelfOrChildHasEvalWithCatchYield();
                    b = b->parentCodeBlock();
                }
                continue;
            }
            if (!codeBlock->hasName(uname)) {
                CodeBlock* c = codeBlock->parentCodeBlock();
                while (c) {
                    if (c->tryCaptureIdentifiersFromChildCodeBlock(uname)) {
                        hasCapturedIdentifier = true;
                        break;
                    }
                    c = c->parentCodeBlock();
                }
            }
        }

        if (hasCapturedIdentifier) {
            CodeBlock* c = codeBlock->parentCodeBlock();
            while (c && c->m_canAllocateEnvironmentOnStack) {
                c->m_canAllocateEnvironmentOnStack = false;
                c = c->parentCodeBlock();
            }
        }
    }

    for (size_t i = 0; i < scopeCtx->m_childScopes.size(); i++) {
        codeBlock->appendChildBlock(generateCodeBlockTreeFromASTWalker(ctx, source, script, scopeCtx->m_childScopes[i], codeBlock));
    }

    return codeBlock;
}

// generate code blocks from AST
CodeBlock* ScriptParser::generateCodeBlockTreeFromAST(Context* ctx, StringView source, Script* script, ProgramNode* program)
{
    return generateCodeBlockTreeFromASTWalker(ctx, source, script, program->scopeContext(), nullptr);
}

void ScriptParser::generateCodeBlockTreeFromASTWalkerPostProcess(CodeBlock* cb)
{
    for (size_t i = 0; i < cb->m_childBlocks.size(); i++) {
        generateCodeBlockTreeFromASTWalkerPostProcess(cb->m_childBlocks[i]);
    }
    cb->computeVariables();
}

ScriptParser::ScriptParserResult ScriptParser::parse(StringView scriptSource, String* fileName, CodeBlock* parentCodeBlock, bool strictFromOutside)
{
    Script* script = nullptr;
    ScriptParseError* error = nullptr;
    try {
        ProgramNode* program = esprima::parseProgram(m_context, scriptSource, nullptr, strictFromOutside);

        script = new Script(fileName);
        CodeBlock* topCodeBlock;
        if (parentCodeBlock) {
            program->scopeContext()->m_hasEval = parentCodeBlock->hasEval();
            program->scopeContext()->m_hasWith = parentCodeBlock->hasWith();
            program->scopeContext()->m_hasCatch = parentCodeBlock->hasCatch();
            program->scopeContext()->m_hasYield = parentCodeBlock->hasYield();
            topCodeBlock = generateCodeBlockTreeFromASTWalker(m_context, scriptSource, script, program->scopeContext(), parentCodeBlock);
        } else {
            topCodeBlock = generateCodeBlockTreeFromAST(m_context, scriptSource, script, program);
        }

        generateCodeBlockTreeFromASTWalkerPostProcess(topCodeBlock);

        topCodeBlock->m_cachedASTNode = program;
        script->m_topCodeBlock = topCodeBlock;

// dump Code Block
#ifndef NDEBUG
        if (getenv("DUMP_CODEBLOCK_TREE") && strlen(getenv("DUMP_CODEBLOCK_TREE"))) {
            std::function<void(CodeBlock*, size_t depth)> fn = [&fn](CodeBlock* cb, size_t depth) {

#define PRINT_TAB()                      \
    for (size_t i = 0; i < depth; i++) { \
        printf("  ");                    \
    }

                PRINT_TAB()
                printf("CodeBlock %s (%d:%d -> %d:%d)(%s, %s) (E:%d, W:%d, C:%d, Y:%d) Name:%d\n", cb->m_functionName.string()->toUTF8StringData().data(),
                       (int)cb->m_locStart.line,
                       (int)cb->m_locStart.column,
                       (int)cb->m_locEnd.line,
                       (int)cb->m_locEnd.column,
                       cb->m_canAllocateEnvironmentOnStack ? "Stack" : "Heap",
                       cb->m_canUseIndexedVariableStorage ? "Indexed" : "Named",
                       (int)cb->m_hasEval, (int)cb->m_hasWith, (int)cb->m_hasCatch, (int)cb->m_hasYield, (int)cb->m_functionNameIndex);

                PRINT_TAB()
                printf("Names: ");
                for (size_t i = 0; i < cb->m_identifierInfos.size(); i++) {
                    printf("%s(%s, %d), ", cb->m_identifierInfos[i].m_name.string()->toUTF8StringData().data(),
                           cb->m_identifierInfos[i].m_needToAllocateOnStack ? "Stack" : "Heap", (int)cb->m_identifierInfos[i].m_indexForIndexedStorage);
                }
                puts("");

                PRINT_TAB()
                printf("Using Names: ");
                for (size_t i = 0; i < cb->m_scopeContext->m_usingNames.size(); i++) {
                    printf("%s, ", cb->m_scopeContext->m_usingNames[i].string()->toUTF8StringData().data());
                }
                puts("");

                for (size_t i = 0; i < cb->m_childBlocks.size(); i++) {
                    fn(cb->m_childBlocks[i], depth + 1);
                }

            };
            fn(topCodeBlock, 0);
        }
#endif

    } catch (esprima::Error* orgError) {
        script = nullptr;
        error = new ScriptParseError();
        error->column = orgError->column;
        error->description = orgError->description;
        error->index = orgError->index;
        error->lineNumber = orgError->lineNumber;
        error->message = orgError->message;
        error->name = orgError->name;
        error->errorCode = orgError->errorCode;
    }

    ScriptParser::ScriptParserResult result(script, error);
    return result;
}

Node* ScriptParser::parseFunction(CodeBlock* codeBlock)
{
    try {
        Node* body = esprima::parseSingleFunction(m_context, codeBlock);
        return body;
    } catch (esprima::Error* orgError) {
        //
        RELEASE_ASSERT_NOT_REACHED();
    }
}
}
