#include <stdio.h>
#include "General.h"
#include "Maths.h"
#include "Platform.h"
#include "Containers.h"

#if TARGET_WINDOWS
#include <windows.h>
#include <strsafe.h>
#include "Win32Common.cpp"
#else
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/limits.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include "LinuxCommon.cpp"
#endif

enum //TokenType
{
	TOKEN_INVALID,
	TOKEN_IDENTIFIER,
	TOKEN_LITERAL_NUMBER,
	TOKEN_LITERAL_STRING,
	TOKEN_PREPROCESSOR_DIRECTIVE,
	TOKEN_ASCII_BEGIN = '!',
	TOKEN_ASCII_END = '~' + 1,
	TOKEN_END_OF_FILE
};

struct Token
{
	int type;
	char *begin;
	int size;

	const char *file;
	int line;
};
DECLARE_DYNAMIC_ARRAY(Token);

struct Tokenizer
{
	char *cursor;
	int line;
};

enum Type
{
	TYPE_INVALID,
	TYPE_U8,
	TYPE_U16,
	TYPE_U32,
	TYPE_U64,
	TYPE_I8,
	TYPE_I16,
	TYPE_I32,
	TYPE_I64,
	TYPE_F32,
	TYPE_F64,
	TYPE_BOOL,
	TYPE_STRUCT,
	TYPE_COUNT
};

const char *TypeStrings[] =
{
	"TYPE_INVALID",
	"TYPE_U8",
	"TYPE_U16",
	"TYPE_U32",
	"TYPE_U64",
	"TYPE_I8",
	"TYPE_I16",
	"TYPE_I32",
	"TYPE_I64",
	"TYPE_F32",
	"TYPE_F64",
	"TYPE_BOOL",
	"TYPE_STRUCT"
};

const char *basicTypeStrings[] =
{
	"INVALID",
	"u8",
	"u16",
	"u32",
	"u64",
	"i8",
	"i16",
	"i32",
	"i64",
	"f32",
	"f64",
	"bool",
	"struct"
};

struct TypeInfo
{
	Token name;
	bool isConst;
	bool isStatic;
	bool isFunction;
	int ptrLevels;
	int count;
	char *paramsBegin;
	int paramsSize;
};

struct Param
{
	TypeInfo type;
	Token name;
	bool isVarArgs;
};

struct Procedure
{
	TypeInfo returnType;
	Token name;
	Param params[16];
	int paramCount;
};
DECLARE_DYNAMIC_ARRAY(Procedure);

struct StructMember
{
	Token name;
	Type type;
	TypeInfo typeInfo;
};

struct Struct
{
	Token name;
	int memberCount;
	StructMember members[64];
};
DECLARE_DYNAMIC_ARRAY(Struct);

inline bool IsAlpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

inline bool IsNumeric(char c)
{
	return c >= '0' && c <= '9';
}

inline bool IsNumericHex(char c)
{
	return (c >= '0' && c <= '9') ||
		(c >= 'a' && c <= 'f') ||
		(c >= 'A' && c <= 'F');
}

inline bool IsWhitespace(char c)
{
	return c == ' ' ||
		c == '\t' ||
		c == '\n' ||
		c == '\r';
}

int EatWhitespace(Tokenizer *tokenizer)
{
	int nAdvanced = 0;
	while (*tokenizer->cursor && IsWhitespace(*tokenizer->cursor))
	{
		if (*tokenizer->cursor == '\n')
			++tokenizer->line;
		++tokenizer->cursor;
		++nAdvanced;
	}
	return nAdvanced;
}

int EatRestOfLine(Tokenizer *tokenizer)
{
	int nAdvanced = 0;
	for (;; ++tokenizer->cursor)
	{
		char c = *tokenizer->cursor;
		if (!c)
			break;

		if (c == '\\')
		{
			++tokenizer->cursor;
		}
		else if (c == '\n')
		{
			++tokenizer->cursor;
			++tokenizer->line;
			break;
		}
		++nAdvanced;
	}
	return nAdvanced;
}

void ProcessCppComment(Tokenizer *tokenizer)
{
	tokenizer->cursor += 2; // Skip both '/'
	EatRestOfLine(tokenizer);
}

void ProcessCComment(Tokenizer *tokenizer)
{
	tokenizer->cursor += 2;
	for (;; ++tokenizer->cursor)
	{
		if (!*tokenizer->cursor)
			break;

		if (*tokenizer->cursor == '*' && *(tokenizer->cursor + 1) == '/')
		{
			tokenizer->cursor += 2;
			break;
		}
	}
}

Token ReadTokenAndAdvance(Tokenizer *tokenizer)
{
	Token result = {};
	result.line = tokenizer->line;

	EatWhitespace(tokenizer);

	if (*tokenizer->cursor == '/')
	{
		if (*(tokenizer->cursor + 1) == '/')
			ProcessCppComment(tokenizer);
		else if (*(tokenizer->cursor + 1) == '*')
			ProcessCComment(tokenizer);
	}

	if (!*tokenizer->cursor)
	{
		result.type = TOKEN_END_OF_FILE;
		return result;
	}

	if (*tokenizer->cursor == '"')
	{
		result.type = TOKEN_LITERAL_STRING;
		++result.size;
		++tokenizer->cursor;
		result.begin = tokenizer->cursor;

		while (*tokenizer->cursor && *tokenizer->cursor != '"')
		{
			if (*tokenizer->cursor == '\\')
			{
				++result.size;
				++tokenizer->cursor;
			}
			++result.size;
			++tokenizer->cursor;
		}
		++tokenizer->cursor;
		//Log("String literal: \"%.*s\"\n", result.size, result.begin);
	}
	else if (IsAlpha(*tokenizer->cursor))
	{
		result.type = TOKEN_IDENTIFIER;
		result.begin = tokenizer->cursor;
		while (IsAlpha(*tokenizer->cursor) || IsNumeric(*tokenizer->cursor) || *tokenizer->cursor == '_')
		{
			++result.size;
			++tokenizer->cursor;
		}
		//Log("Identifier: %.*s\n", result.size, result.begin);
	}
	else if (IsNumeric(*tokenizer->cursor))
	{
		result.type = TOKEN_LITERAL_NUMBER;
		result.begin = tokenizer->cursor;
		bool done = false;
		if (*tokenizer->cursor == '0')
		{
			++result.size;
			++tokenizer->cursor;
			ASSERT(!IsNumeric(*tokenizer->cursor));
			if (*tokenizer->cursor != 'x' &&
					*tokenizer->cursor != 'X' &&
					*tokenizer->cursor != 'b' &&
					*tokenizer->cursor != '.')
			{
				done = true;
			}
			++result.size;
			++tokenizer->cursor;
		}
		if (!done)
		{
			while (IsNumericHex(*tokenizer->cursor) || *tokenizer->cursor == '.')
			{
				++result.size;
				++tokenizer->cursor;
			}
			if (*tokenizer->cursor == 'f')
			{
				++result.size;
				++tokenizer->cursor;
			}
		}
		//Log("Number: %.*s\n", result.size, result.begin);
	}
	else if (*tokenizer->cursor == '#')
	{
		result.type = TOKEN_PREPROCESSOR_DIRECTIVE;
		result.begin = tokenizer->cursor;
		result.size = EatRestOfLine(tokenizer);
		//Log("Preprocessor directive: %.*s\n", result.size, result.begin);
	}
	else if (*tokenizer->cursor >= TOKEN_ASCII_BEGIN && *tokenizer->cursor < TOKEN_ASCII_END)
	{
		result.type = *tokenizer->cursor;
		++tokenizer->cursor;
	}
	else
	{
		result.type = TOKEN_INVALID;
		result.begin = tokenizer->cursor;
		result.size = 1;
		++tokenizer->cursor;
	}

	return result;
}

void PrintTypeInfo(char *buffer, const TypeInfo *type)
{
	char stars[8] = {};
	for (int i = 0; i < type->ptrLevels; ++i)
	{
		stars[i] = '*';
		stars[i + 1] = 0;
	}
	sprintf(buffer, "%s%.*s %s\0",
			type->isConst ? "const " : "",
			type->name.size, type->name.begin,
			stars);
}

void ParseFile(const char *filename, DynamicArray_Token &tokens)
{
	auto MallocPlus1 = [](u64 size) { return malloc(size + 1); };
	char *fileBuffer;
	u64 fileSize;
	PlatformReadEntireFile(filename, (u8 **)&fileBuffer, &fileSize, MallocPlus1);
	fileBuffer[fileSize] = 0;

	Tokenizer tokenizer = {};
	tokenizer.cursor = fileBuffer;
	while (true)
	{
		Token newToken = ReadTokenAndAdvance(&tokenizer);
		newToken.file = filename;
		if (newToken.type == TOKEN_END_OF_FILE)
			break;
		else if (newToken.type == TOKEN_PREPROCESSOR_DIRECTIVE)
		{
			if (strncmp(newToken.begin, "#include", 8) == 0)
			{
				char *fileBegin = 0;
				char *fileEnd = 0;
				for (char *scan = newToken.begin; scan < newToken.begin + newToken.size; ++scan)
				{
					if (*scan == '"')
					{
						if (!fileBegin)
						{
							fileBegin = scan + 1;
						}
						else
						{
							fileEnd = scan;
							break;
						}
					}
				}

				char fullname[MAX_PATH];
				sprintf(fullname, "src/%.*s", (int)(fileEnd - fileBegin), fileBegin);
				if (PlatformFileExists(fullname))
				{
					ParseFile(fullname, tokens);
				}
			}
		}
		else
			tokens[DynamicArrayAdd_Token(&tokens, realloc)] = newToken;
	}
}

Type ParseType(Token token, DynamicArray_Struct &structs)
{
	Type result = TYPE_INVALID;

	if (strncmp("u8", token.begin, token.size) == 0)
	{
		result = TYPE_U8;
	}
	else if (strncmp("u16", token.begin, token.size) == 0)
	{
		result = TYPE_U16;
	}
	else if (strncmp("u32", token.begin, token.size) == 0)
	{
		result = TYPE_U32;
	}
	else if (strncmp("u64", token.begin, token.size) == 0)
	{
		result = TYPE_U64;
	}

	else if (strncmp("i8", token.begin, token.size) == 0)
	{
		result = TYPE_I8;
	}
	else if (strncmp("i16", token.begin, token.size) == 0)
	{
		result = TYPE_I16;
	}
	else if (strncmp("i32", token.begin, token.size) == 0)
	{
		result = TYPE_I32;
	}
	else if (strncmp("i64", token.begin, token.size) == 0 ||
			strncmp("int", token.begin, token.size) == 0)
	{
		result = TYPE_I64;
	}

	else if (strncmp("f32", token.begin, token.size) == 0)
	{
		result = TYPE_F32;
	}
	else if (strncmp("f64", token.begin, token.size) == 0)
	{
		result = TYPE_F64;
	}

	else if (strncmp("bool", token.begin, token.size) == 0)
	{
		result = TYPE_BOOL;
	}

	else
	{
		for (u32 i = 0; i < structs.size; ++i)
		{
			if (structs[i].name.type == TOKEN_IDENTIFIER && // Avoid annonymous structs
					strncmp(structs[i].name.begin, token.begin, Min(structs[i].name.size, token.size)) == 0)
			{
				result = TYPE_STRUCT;
				break;
			}
		}
	}

	return result;
}

void PrintStructs(DynamicArray_Struct &structs)
{
	for (u32 structIdx = 0; structIdx < structs.size; ++structIdx)
	{
		Struct *struct_ = &structs[structIdx];

		Log("Struct '%.*s'\n", struct_->name.size, struct_->name.begin);

		for (int memberIdx = 0; memberIdx < struct_->memberCount; ++memberIdx)
		{
			StructMember *member = &struct_->members[memberIdx];
			Log("    Member: %s ", basicTypeStrings[member->type],
					member->typeInfo.name.size, member->typeInfo.name.begin,
					member->name.size, member->name.begin);

			char buffer[256];
			PrintTypeInfo(buffer, &member->typeInfo);
			Log("%s", buffer);

			Log("%.*s\n", member->name.size, member->name.begin);
		}
	}
}

void ReadStructs(DynamicArray_Token &tokens, DynamicArray_Struct &structs)
{
	DynamicArrayInit_Struct(&structs, 256, malloc);

	for (u32 tokenIdx = 0; tokenIdx < tokens.size; ++tokenIdx)
	{
		Token *token = &tokens[tokenIdx];
		if (token->type == TOKEN_IDENTIFIER)
		{
			if (strncmp(token->begin, "struct", token->size) == 0 ||
				strncmp(token->begin, "union", token->size) == 0) // @Improve
			{
				Struct struct_ = {};

				token = &tokens[++tokenIdx];
				if (token->type == TOKEN_IDENTIFIER)
				{
					struct_.name = *token;
					Log("Found struct of name '%.*s'\n", token->size, token->begin);
					token = &tokens[++tokenIdx];
				}
				else
				{
					Log("Found annonymous struct\n");
				}

				if (token->type == ';')
				{
					// Just declaration
					token = &tokens[++tokenIdx];
					continue;
				}

				if (token->type != '{')
				{
					Log("ERROR! Reading struct, expected {\n");
					return;
				}
				token = &tokens[++tokenIdx];

				int scopeLevel = 1;
				while (scopeLevel && token->type != TOKEN_END_OF_FILE)
				{
					StructMember member = {};

					while (token->type == TOKEN_PREPROCESSOR_DIRECTIVE)
						token = &tokens[++tokenIdx];

					if (token->type == TOKEN_IDENTIFIER &&
							strncmp(token->begin, "static", token->size) == 0)
					{
						member.typeInfo.isStatic = true;
						token = &tokens[++tokenIdx];
					}

					if (token->type == TOKEN_IDENTIFIER &&
							strncmp(token->begin, "const", token->size) == 0)
					{
						member.typeInfo.isConst = true;
						token = &tokens[++tokenIdx];
					}

					bool annonymousType = false;
					if (token->type == TOKEN_IDENTIFIER && (
							strncmp(token->begin, "struct", token->size) == 0 ||
							strncmp(token->begin, "union", token->size) == 0))
					{
						// @Hack
						if ((token + 1)->type == '{')
						{
							// Check if it's unnamed union/struct
							Token *t = token + 1;
							while (t->type != '}' && t->type != TOKEN_END_OF_FILE)
								++t;
							++t;
							if (t->type != TOKEN_IDENTIFIER)
							{
								// Unnamed union/struct
								++scopeLevel;
								token = &tokens[++tokenIdx];
								token = &tokens[++tokenIdx];
							}
							else
							{
								// Named union/struct
								annonymousType = true;

								while (token->type != '}' && token->type != TOKEN_END_OF_FILE)
									token = &tokens[++tokenIdx];
								token = &tokens[++tokenIdx];
							}
						}
					}

					if (!annonymousType && token->type == TOKEN_IDENTIFIER)
					{
						member.type = ParseType(*token, structs);
						member.typeInfo.name = *token;
						token = &tokens[++tokenIdx];
					}

					while (1)
					{
						member.typeInfo.ptrLevels = 0;

						while (token->type == '*')
						{
							++member.typeInfo.ptrLevels;
							token = &tokens[++tokenIdx];
						}

						if (token->type == TOKEN_IDENTIFIER)
						{
							member.name = *token;
							token = &tokens[++tokenIdx];
						}

						if (token->type == '[')
						{
							token = &tokens[++tokenIdx];
							if (token->type != TOKEN_LITERAL_NUMBER)
							{
								Log("ERROR! Non-literal array sizes not supported yet!\n");
								// @Incomplete: parse array members!
								while (token->type != ']')
								{
									token = &tokens[++tokenIdx];
								}
							}
							else
							{
								member.typeInfo.count = atoi(token->begin);
								token = &tokens[++tokenIdx];
								ASSERT(token->type == ']');
							}
							token = &tokens[++tokenIdx];
						}

						// @Hack: Ignore default values
						if (token->type == '=')
						{
							token = &tokens[++tokenIdx];
							while (token->type != ';' && token->type != ',')
								token = &tokens[++tokenIdx];
						}

						if (member.name.type == TOKEN_IDENTIFIER && member.name.size)
						{
							if (!member.typeInfo.isStatic)
								struct_.members[struct_.memberCount++] = member;
						}
						else
						{
							Log("WARNING! member with empty name!\n");
						}

						if (token->type == ',')
							token = &tokens[++tokenIdx];
						else
							break;
					}

					while (token->type == TOKEN_PREPROCESSOR_DIRECTIVE)
						token = &tokens[++tokenIdx];

					if (token->type != ';')
					{
						Log("ERROR! Reading struct, expected ; got %c\n", token->type);
					}
					token = &tokens[++tokenIdx];

					if (token->type == '}')
					{
						--scopeLevel;
						token = &tokens[++tokenIdx];
					}
				}

				if (struct_.name.type == TOKEN_IDENTIFIER && struct_.name.size)
				{
					structs[DynamicArrayAdd_Struct(&structs, realloc)] = struct_;
				}
			}
		}
	}
}

int ReadProcedure(Token *curToken, Procedure *newProcedure)
{
	*newProcedure = {};

	// Modifier?
	if (strncmp(curToken->begin, "const", curToken->size) == 0)
	{
		newProcedure->returnType.isConst = true;

		++curToken;
		if (curToken->type != TOKEN_IDENTIFIER)
		{
			Log("ERROR: Parsing platform procedure: expected parameter type! %s:%d\n",
					curToken->file, curToken->line);
			return 1;
		}
	}

	if (curToken->type != TOKEN_IDENTIFIER)
	{
		Log("ERROR: Parsing platform procedure: expected return type! %s:%d\n",
				curToken->file, curToken->line);
		return 1;
	}
	newProcedure->returnType.name = *curToken;

	// Pointer?
	++curToken;
	while (curToken->type == '*')
	{
		++newProcedure->returnType.ptrLevels;
		++curToken;
	}

	if (curToken->type != TOKEN_IDENTIFIER)
	{
		Log("ERROR: Parsing platform procedure: expected procedure name! %s:%d\n",
				curToken->file, curToken->line);
		return 1;
	}
	newProcedure->name = *curToken;

	++curToken;
	if (curToken->type != '(')
	{
		Log("ERROR: Parsing platform procedure: expected '('! %s:%d\n",
				curToken->file, curToken->line);
		return 1;
	}

	// Get params
	for (;;)
	{
		Param newParam = {};

		++curToken;
		if (curToken->type == ')')
			break;
		else if (curToken->type == ',' && newProcedure->paramCount)
			continue;

		if (curToken->type == '.')
		{
			for (int i = 0; i < 2; ++i)
			{
				++curToken;
				if (curToken->type != '.')
				{
					Log("ERROR: Parsing platform procedure: syntax error! %s:%d\n",
							curToken->file, curToken->line);
					return 1;
				}
			}
			newParam.isVarArgs = true;

			ASSERT(newProcedure->paramCount < 16);
			newProcedure->params[newProcedure->paramCount++] = newParam;

			break;
		}

		if (curToken->type != TOKEN_IDENTIFIER)
		{
			Log("ERROR: Parsing platform procedure: expected parameter type! %s:%d\n",
					curToken->file, curToken->line);
			return 1;
		}

		// Modifier?
		if (strncmp(curToken->begin, "const", curToken->size) == 0)
		{
			newParam.type.isConst = true;

			++curToken;
			if (curToken->type != TOKEN_IDENTIFIER)
			{
				Log("ERROR: Parsing platform procedure: expected parameter type! %s:%d\n",
					curToken->file, curToken->line);
				return 1;
			}
		}
		newParam.type.name = *curToken;

		// Pointer?
		++curToken;
		while (curToken->type == '*')
		{
			++newParam.type.ptrLevels;
			++curToken;
		}

		// Parenthesis?
		bool openedParenthesis = false;
		if (curToken->type == '(')
		{
			openedParenthesis = true;
			++curToken;
		}

		if (curToken->type == '*')
		{
			newParam.type.isFunction = true;
			++curToken;
		}

		// Name
		if (curToken->type != TOKEN_IDENTIFIER)
		{
			Log("ERROR: Parsing platform procedure: expected parameter name! %s:%d\n",
					curToken->file, curToken->line);
			return 1;
		}
		newParam.name = *curToken;

		if (openedParenthesis)
		{
			++curToken;
			ASSERT(curToken->type == ')');
		}

		if (curToken->type == ')')
		{
			++curToken;
		}

		// Function pointer
		if (newParam.type.isFunction)
		{
			if (curToken->type != '(')
			{
				Log("ERROR: Parsing platform procedure: expected function pointer parameters! %s:%d\n",
						curToken->file, curToken->line);
			}
			++curToken;

			newParam.type.paramsBegin = curToken->begin;

			while (curToken->type != ')')
			{
				if (curToken->type == TOKEN_IDENTIFIER)
				{
					newParam.type.paramsSize += curToken->size;
				}
				else if (curToken->type >= TOKEN_ASCII_BEGIN &&
						curToken->type < TOKEN_ASCII_END)
				{
					++newParam.type.paramsSize;
				}
				else
				{
					ASSERT(false);
				}
				++curToken;
			}
		}

		ASSERT(newProcedure->paramCount < 16);
		newProcedure->params[newProcedure->paramCount++] = newParam;
	}

	return 0;
}

int ExtractPlatformProcedures(DynamicArray_Token &tokens, DynamicArray_Procedure &procedures)
{
	for (u32 tokenIdx = 0; tokenIdx < tokens.size; ++tokenIdx)
	{
		Token *token = &tokens[tokenIdx];
		if (token->type == TOKEN_IDENTIFIER)
		{
			if (strncmp(token->begin, "PLATFORMPROC", token->size) == 0)
			{
				Token *curToken = &tokens[++tokenIdx];
				Procedure newProcedure;
				int error = ReadProcedure(curToken, &newProcedure);
				if (error)
					return error;
				procedures[DynamicArrayAdd_Procedure(&procedures, realloc)] = newProcedure;
			}
		}
	}
	return 0;
}

u64 PrintToFile(FileHandle file, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	char buffer[2048];
	vsprintf(buffer, format, args);
	va_end(args);

	return PlatformWriteToFile(file, buffer, strlen(buffer));
}

void PrintProcedureToFile(FileHandle file, Procedure *proc)
{
	char typeStr[256];
	PrintTypeInfo(typeStr, &proc->returnType);
	PrintToFile(file, "%s(*%.*s)(", typeStr, proc->name.size, proc->name.begin);
	for (int paramIdx = 0; paramIdx < proc->paramCount; ++paramIdx)
	{
		if (paramIdx)
			PrintToFile(file, ", ");

		if (proc->params[paramIdx].isVarArgs)
		{
			PrintToFile(file, "...");
			continue;
		}

		TypeInfo *paramType = &proc->params[paramIdx].type;
		PrintTypeInfo(typeStr, paramType);

		Token *name = &proc->params[paramIdx].name;

		if (proc->params[paramIdx].type.isFunction)
		{
			PrintToFile(file, "%s(*%.*s)(%.*s)", typeStr, name->size, name->begin,
					paramType->paramsSize,
					paramType->paramsBegin);
		}
		else
		{
			PrintToFile(file, "%s%.*s", typeStr, name->size, name->begin);
		}
	}
	PrintToFile(file, ");\n");
}

void WritePlatformCodeFiles(DynamicArray_Procedure &procedures)
{
	// HEADER
	FileHandle file = PlatformOpenForWrite("gen/PlatformCode.h");

	PrintToFile(file, "struct PlatformCode\n{\n");

	for (u32 procIdx = 0; procIdx < procedures.size; ++procIdx)
	{
		Procedure *proc = &procedures[procIdx];
		PrintToFile(file, "\t");
		PrintProcedureToFile(file, proc);
	}

	PrintToFile(file, "};\n");
	PlatformCloseFile(file);

	// PLATFORM SOURCE
	file = PlatformOpenForWrite("gen/PlatformCode.cpp");

	PrintToFile(file, "inline void FillPlatformCodeStruct(PlatformCode *platformCode)\n{\n");

	for (u32 procIdx = 0; procIdx < procedures.size; ++procIdx)
	{
		Procedure *proc = &procedures[procIdx];

		PrintToFile(file, "\tplatformCode->%.*s = %.*s;\n", proc->name.size, proc->name.begin,
				proc->name.size, proc->name.begin);
	}

	PrintToFile(file, "};\n");
	PlatformCloseFile(file);

	// GAME SOURCE
	file = PlatformOpenForWrite("gen/PlatformCodeLoad.cpp");

	PrintToFile(file, "// Global function pointers //\n");
	for (u32 procIdx = 0; procIdx < procedures.size; ++procIdx)
	{
		Procedure *proc = &procedures[procIdx];
		PrintProcedureToFile(file, proc);
	}

	PrintToFile(file, "//////////////////////////////////////////\n\n");

	PrintToFile(file, "inline void ImportPlatformCodeFromStruct(PlatformCode *platformCode)\n{\n");

	for (u32 procIdx = 0; procIdx < procedures.size; ++procIdx)
	{
		Procedure *proc = &procedures[procIdx];

		PrintToFile(file, "\t%.*s = platformCode->%.*s;\n", proc->name.size, proc->name.begin,
				proc->name.size, proc->name.begin);
	}

	PrintToFile(file, "};\n");
	PlatformCloseFile(file);
}

void WriteReflectionFile(DynamicArray_Struct &structs)
{
	FileHandle file = PlatformOpenForWrite("gen/Reflection.h");

	PrintToFile(file, "enum Type\n{\n");
	for (int i = 0; i < TYPE_COUNT; ++i)
		PrintToFile(file, "\t%s,\n", TypeStrings[i]);
	PrintToFile(file, "};\n\n");

	PrintToFile(file, "struct StructInfo;\n");

	PrintToFile(file, "struct StructMember\n{\n");
	PrintToFile(file,   "\tconst char *name;\n");
	PrintToFile(file,   "\tType type;\n");
	PrintToFile(file,   "\tu32 pointerLevels;\n");
	PrintToFile(file,   "\tu32 arrayCount;\n");
	PrintToFile(file,   "\tu64 offset;\n");
	PrintToFile(file,   "\tconst StructInfo *structInfo;\n");
	PrintToFile(file, "};\n\n");

	PrintToFile(file, "struct StructInfo\n{\n");
	PrintToFile(file,   "\tconst char *name;\n");
	PrintToFile(file,   "\tu32 memberCount;\n");
	PrintToFile(file,   "\tconst StructMember *members;\n");
	PrintToFile(file, "};\n\n");

	for (u32 structIdx = 0; structIdx < structs.size; ++structIdx)
	{
		Struct *struct_ = &structs[structIdx];

		PrintToFile(file, "const StructMember _typeInfoMembers_%.*s[] =\n{\n", struct_->name.size,
				struct_->name.begin);
		for (int memberIdx = 0; memberIdx < struct_->memberCount; ++memberIdx)
		{
			StructMember *member = &struct_->members[memberIdx];
			PrintToFile(file, "\t{ \"%.*s\", ", member->name.size, member->name.begin);
			PrintToFile(file, "%s, ", TypeStrings[member->type]);
			PrintToFile(file, "%d, ", member->typeInfo.ptrLevels);
			PrintToFile(file, "%d, ", member->typeInfo.count);
			PrintToFile(file, "offsetof(%.*s, %.*s)", struct_->name.size,
					struct_->name.begin, member->name.size, member->name.begin);
			if (member->type == TYPE_STRUCT)
			{
				PrintToFile(file, ", &typeInfo_%.*s", member->typeInfo.name.size,
						member->typeInfo.name.begin);
			}
			PrintToFile(file, " },\n", struct_->name.size,
					struct_->name.begin, member->name.size, member->name.begin);
		}
		PrintToFile(file, "};\n");

		PrintToFile(file, "const StructInfo typeInfo_%.*s = { ", struct_->name.size,
				struct_->name.begin);

		PrintToFile(file, "\"%.*s\", ", struct_->name.size, struct_->name.begin);
		PrintToFile(file, "%d, ", struct_->memberCount);
		PrintToFile(file, "_typeInfoMembers_%.*s ", struct_->name.size, struct_->name.begin);

		PrintToFile(file, "};\n\n");
	}

	PlatformCloseFile(file);
}

int main(int argc, char **argv)
{
	if (argc == 1)
		return 1;

#if TARGET_WINDOWS
	AttachConsole(ATTACH_PARENT_PROCESS);
	g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	Log("%s - %s\n", argv[0], argv[1]);

	DynamicArray_Procedure procedures;
	DynamicArrayInit_Procedure(&procedures, 64, malloc);

	const char *gameFile = "src/Game.cpp";
	DynamicArray_Token gameTokens;
	DynamicArrayInit_Token(&gameTokens, 4096, malloc);
	ParseFile(gameFile, gameTokens);

	DynamicArray_Struct structs;
	ReadStructs(gameTokens, structs);
	PrintStructs(structs);

	WriteReflectionFile(structs);

	const char *platformFile = argv[1];

	DynamicArray_Token tokens;
	DynamicArrayInit_Token(&tokens, 4096, malloc);
	ParseFile(platformFile, tokens);

	int error = ExtractPlatformProcedures(tokens, procedures);
	if (error)
		return error;

	WritePlatformCodeFiles(procedures);

	return 0;
}
