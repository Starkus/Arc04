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
	TYPE_ENUM,
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
	"TYPE_STRUCT",
	"TYPE_ENUM"
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
	"struct",
	"enum"
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
	StructMember members[64]; // @Improve: no fixed size array?
};
DECLARE_DYNAMIC_ARRAY(Struct);

struct EnumValue
{
	Token name;
};
DECLARE_DYNAMIC_ARRAY(EnumValue);
struct Enum
{
	Token name;
	DynamicArray_EnumValue values;
};
DECLARE_DYNAMIC_ARRAY(Enum);

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

	while (*tokenizer->cursor == '/')
	{
		if (*(tokenizer->cursor + 1) == '/')
		{
			ProcessCppComment(tokenizer);
			EatWhitespace(tokenizer);
		}
		else if (*(tokenizer->cursor + 1) == '*')
		{
			ProcessCComment(tokenizer);
			EatWhitespace(tokenizer);
		}
		else
			break;
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

inline bool TokenIsStr(Token *token, const char *str)
{
	return token->type == TOKEN_IDENTIFIER &&
		token->size == strlen(str) &&
		strncmp(token->begin, str, token->size) == 0;
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
		if (newToken.type == TOKEN_PREPROCESSOR_DIRECTIVE)
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

				char *fullname = (char *)malloc(MAX_PATH);
				sprintf(fullname, "src/%.*s", (int)(fileEnd - fileBegin), fileBegin);
				if (PlatformFileExists(fullname))
				{
					ParseFile(fullname, tokens);
					// Remove EOF token
					ASSERT(tokens[tokens.size - 1].type == TOKEN_END_OF_FILE);
					--tokens.size;
				}
			}
		}
		else
			*DynamicArrayAdd_Token(&tokens, realloc) = newToken;

		if (newToken.type == TOKEN_END_OF_FILE)
			break;
	}
}

Type ParseType(Token token, DynamicArray_Struct &structs, DynamicArray_Enum &enums)
{
	Type result = TYPE_INVALID;

	if (TokenIsStr(&token, "u8") ||
			TokenIsStr(&token, "char"))
	{
		result = TYPE_U8;
	}
	else if (TokenIsStr(&token, "u16"))
	{
		result = TYPE_U16;
	}
	else if (TokenIsStr(&token, "u32"))
	{
		result = TYPE_U32;
	}
	else if (TokenIsStr(&token, "u64"))
	{
		result = TYPE_U64;
	}

	else if (TokenIsStr(&token, "i8"))
	{
		result = TYPE_I8;
	}
	else if (TokenIsStr(&token, "i16"))
	{
		result = TYPE_I16;
	}
	else if (TokenIsStr(&token, "i32"))
	{
		result = TYPE_I32;
	}
	else if (TokenIsStr(&token, "i64") ||
			TokenIsStr(&token, "int") ||
			TokenIsStr(&token, "long"))
	{
		result = TYPE_I64;
	}

	else if (TokenIsStr(&token, "f32") ||
			TokenIsStr(&token, "float"))
	{
		result = TYPE_F32;
	}
	else if (TokenIsStr(&token, "f64") ||
			TokenIsStr(&token, "double"))
	{
		result = TYPE_F64;
	}

	else if (TokenIsStr(&token, "bool"))
	{
		result = TYPE_BOOL;
	}

	else
	{
		for (u32 i = 0; i < structs.size; ++i)
		{
			if (structs[i].name.type == TOKEN_IDENTIFIER && // Avoid anonymous structs
					strncmp(structs[i].name.begin, token.begin, Min(structs[i].name.size, token.size)) == 0)
			{
				result = TYPE_STRUCT;
				return result;
			}
		}
		for (u32 i = 0; i < enums.size; ++i)
		{
			if (enums[i].name.type == TOKEN_IDENTIFIER && // Avoid anonymous enums
					strncmp(enums[i].name.begin, token.begin, Min(enums[i].name.size, token.size)) == 0)
			{
				result = TYPE_ENUM;
				return result;
			}
		}
	}

	return result;
}

Token *ReadStruct(Token *token, Struct *struct_, DynamicArray_Struct &structs, DynamicArray_Enum
		&enums)
{
	if (token->type != '{')
	{
		Log("ERROR! Reading struct, expected {\n");
		return token;
	}
	++token;

	int scopeLevel = 1;
	while (scopeLevel && token->type != TOKEN_END_OF_FILE)
	{
		StructMember member = {};

		while (token->type == TOKEN_PREPROCESSOR_DIRECTIVE)
			++token;

		if (TokenIsStr(token, "static"))
		{
			member.typeInfo.isStatic = true;
			++token;
		}

		if (TokenIsStr(token, "const"))
		{
			member.typeInfo.isConst = true;
			++token;
		}

		bool anonymousType = false;
		char *anonymousTypeName = nullptr;
		while (TokenIsStr(token, "struct") || TokenIsStr(token, "union"))
		{
			++token;
			if (token->type != '{')
			{
				Log("ERROR! Parsing struct, expected {\n");
				break;
			}

			// Check if it's unnamed union/struct
			Token *lookAheadToken = token + 1;
			int lookAheadScopeLevel = 1;
			while (lookAheadScopeLevel && lookAheadToken->type != TOKEN_END_OF_FILE)
			{
				if (lookAheadToken->type == '{')
					++lookAheadScopeLevel;
				else if (lookAheadToken->type == '}')
					--lookAheadScopeLevel;
				++lookAheadToken;
			}

			if (lookAheadToken->type != TOKEN_IDENTIFIER)
			{
				// Unnamed union/struct
				// We ignore anonymous, name-less strucs/unions. They only group things together
				// but since we don't care about the offset of things we don't care about them
				// at all!
				++scopeLevel;
				++token;
			}
			else
			{
				// Named union/struct
				anonymousType = true;

				anonymousTypeName = (char *)malloc(512);
				sprintf(anonymousTypeName, "anonstruct_%.*s_%.*s",
						struct_->name.size, struct_->name.begin,
						lookAheadToken->size, lookAheadToken->begin);
				Log("generated name: %s\n", anonymousTypeName);
				break;
			}
		}

		if (anonymousType)
		{
			member.type = TYPE_STRUCT;
			Token nameToken = {};
			nameToken.type = TOKEN_IDENTIFIER;
			nameToken.begin = anonymousTypeName;
			nameToken.size = (int)strlen(anonymousTypeName);
			member.typeInfo.name = nameToken;

			Struct anonstruct = {};
			anonstruct.name = nameToken;
			token = ReadStruct(token, &anonstruct, structs, enums);
			*DynamicArrayAdd_Struct(&structs, realloc) = anonstruct;
		}
		else if (token->type == TOKEN_IDENTIFIER)
		{
			member.type = ParseType(*token, structs, enums);
			member.typeInfo.name = *token;
			++token;
		}

		while (1)
		{
			member.typeInfo.ptrLevels = 0;

			while (token->type == '*')
			{
				++member.typeInfo.ptrLevels;
				++token;
			}

			if (token->type == TOKEN_IDENTIFIER)
			{
				member.name = *token;
				++token;
			}

			if (token->type == '[')
			{
				++token;
				if (token->type != TOKEN_LITERAL_NUMBER)
				{
					Log("ERROR! Non-literal array sizes not supported yet!\n");
					// @Incomplete: parse array members!
					while (token->type != ']')
					{
						++token;
					}
				}
				else
				{
					member.typeInfo.count = atoi(token->begin);
					++token;
					ASSERT(token->type == ']');
				}
				++token;
			}

			// @Hack: Ignore default values
			if (token->type == '=')
			{
				++token;
				Log("Ignoring default value!\n");
				int defaultValueScopeLevel = 0;
				while (defaultValueScopeLevel || (token->type != ';' && token->type != ','))
				{
					if (token->type == '{')
						++defaultValueScopeLevel;
					else if (token->type == '}')
						--defaultValueScopeLevel;
					++token;
				}
			}

			if (member.name.type == TOKEN_IDENTIFIER && member.name.size)
			{
				if (!member.typeInfo.isStatic)
					struct_->members[struct_->memberCount++] = member;
			}
			else
			{
				Log("WARNING! member with empty name!\n");
			}

			if (token->type == ',')
				++token;
			else
				break;
		}

		while (token->type == TOKEN_PREPROCESSOR_DIRECTIVE)
			++token;

		if (token->type != ';')
		{
			Log("ERROR! Reading struct, expected ; got 0x%X(%c)\n", token->type, token->type);
		}
		++token;

		if (token->type == '}')
		{
			--scopeLevel;
			++token;
		}
	}
	ASSERT(token->type != TOKEN_END_OF_FILE);
	return token;
}

void ReadStructsAndEnums(DynamicArray_Token &tokens, DynamicArray_Struct &structs, DynamicArray_Enum &enums)
{
	DynamicArrayInit_Struct(&structs, 256, malloc);
	DynamicArrayInit_Enum(&enums, 256, malloc);

	for (Token *token = &tokens[0]; token->type != TOKEN_END_OF_FILE; )
	{
		if (token->type == TOKEN_IDENTIFIER)
		{
			if (TokenIsStr(token, "struct") || TokenIsStr(token, "union")) // @Improve
			{
				Struct struct_ = {};

				++token;
				if (token->type == TOKEN_IDENTIFIER)
				{
					struct_.name = *token;
					Log("Found struct of name '%.*s'\n", token->size, token->begin);
					++token;
				}
				else
				{
					Log("Found anonymous struct\n");
				}

				if (token->type == ';')
				{
					// Just declaration
					++token;
					continue;
				}

				token = ReadStruct(token, &struct_, structs, enums);

				if (struct_.name.type == TOKEN_IDENTIFIER && struct_.name.size)
				{
					*DynamicArrayAdd_Struct(&structs, realloc) = struct_;
				}
			}
			else if (strncmp(token->begin, "enum", token->size) == 0)
			{
				Enum enum_ = {};
				DynamicArrayInit_EnumValue(&enum_.values, 32, malloc);

				++token;
				if (token->type == TOKEN_IDENTIFIER)
				{
					enum_.name = *token;
					Log("Found enum of name '%.*s'\n", token->size, token->begin);
					++token;
				}
				else
				{
					Log("Found anonymous enum\n");
				}

				if (token->type == ';')
				{
					// Just declaration
					++token;
					continue;
				}

				if (token->type != '{')
				{
					Log("ERROR! Reading enum, expected {\n");
					return;
				}
				++token;

				while (token->type != TOKEN_END_OF_FILE)
				{
					EnumValue enumValue = {};

					while (token->type == TOKEN_PREPROCESSOR_DIRECTIVE)
						++token;

					if (token->type == TOKEN_IDENTIFIER)
					{
						enumValue.name = *token;
						++token;
					}

					// Ignore values, we can offload them to the compiler
					if (token->type == '=')
					{
						++token;
						while (token->type != ',' && token->type != '}')
							++token;
					}

					//Log("%.*s.%.*s\n", enum_.name.size, enum_.name.begin, enumValue.name.size,
							//enumValue.name.begin);
					*DynamicArrayAdd_EnumValue(&enum_.values, realloc) = enumValue;

					if (token->type == ',')
					{
						++token;
						continue;
					}
					else if (token->type == '}')
					{
						++token;
						break;
					}
					else
					{
						Log("ERROR! Reading enum, expected ,/} got %c(0x%X)\n", token->type,
								token->type);
						break;
					}
				}
				ASSERT(token->type != TOKEN_END_OF_FILE);

				if (enum_.name.type == TOKEN_IDENTIFIER && enum_.name.size)
				{
					*DynamicArrayAdd_Enum(&enums, realloc) = enum_;
				}
			}
			else
			{
				++token;
			}
		}
		else
		{
			++token;
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
				*DynamicArrayAdd_Procedure(&procedures, realloc) = newProcedure;
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

void WriteTypeInfoFiles(DynamicArray_Struct &structs, DynamicArray_Enum &enums)
{
	FileHandle file = PlatformOpenForWrite("gen/TypeInfo.h");

	PrintToFile(file, "#if DEBUG_BUILD\n");

	PrintToFile(file, "enum Type\n{\n");
	for (int i = 0; i < TYPE_COUNT; ++i)
		PrintToFile(file, "\t%s,\n", TypeStrings[i]);
	PrintToFile(file, "};\n\n");

	PrintToFile(file, "struct EnumValue\n{\n");
	PrintToFile(file,   "\tconst char *name;\n");
	PrintToFile(file,   "\ti64 value;\n");
	PrintToFile(file, "};\n\n");

	PrintToFile(file, "struct EnumInfo\n{\n");
	PrintToFile(file,   "\tconst char *name;\n");
	PrintToFile(file,   "\tu32 valueCount;\n");
	PrintToFile(file,   "\tconst EnumValue *values;\n");
	PrintToFile(file, "};\n\n");

	//PrintToFile(file, "struct StructInfo;\n");

	PrintToFile(file, "struct StructMember\n{\n");
	PrintToFile(file,   "\tconst char *name;\n");
	PrintToFile(file,   "\tType type;\n");
	PrintToFile(file,   "\tu32 pointerLevels;\n");
	PrintToFile(file,   "\tu32 arrayCount;\n");
	PrintToFile(file,   "\tu64 size;\n");
	PrintToFile(file,   "\tu64 offset;\n");
	PrintToFile(file,   "\tconst void *typeInfo;\n");
	PrintToFile(file, "};\n\n");

	PrintToFile(file, "struct StructInfo\n{\n");
	PrintToFile(file,   "\tconst char *name;\n");
	PrintToFile(file,   "\tu32 memberCount;\n");
	PrintToFile(file,   "\tconst StructMember *members;\n");
	PrintToFile(file, "};\n\n");

	// ENUMS
	for (u32 enumIdx = 0; enumIdx < enums.size; ++enumIdx)
	{
		Enum *enum_ = &enums[enumIdx];

		PrintToFile(file, "extern const EnumInfo enumInfo_%.*s;\n", enum_->name.size,
				enum_->name.begin);
	}

	// STRUCTS
	for (u32 structIdx = 0; structIdx < structs.size; ++structIdx)
	{
		Struct *struct_ = &structs[structIdx];

		PrintToFile(file, "extern const StructInfo typeInfo_%.*s;\n", struct_->name.size,
				struct_->name.begin);
	}

	PrintToFile(file, "#endif\n");
	PlatformCloseFile(file);

	file = PlatformOpenForWrite("gen/TypeInfo.cpp");
	PrintToFile(file, "#if DEBUG_BUILD\n");

	// ENUMS
	for (u32 enumIdx = 0; enumIdx < enums.size; ++enumIdx)
	{
		Enum *enum_ = &enums[enumIdx];

		PrintToFile(file, "const EnumValue _enumInfoValues_%.*s[] =\n{\n", enum_->name.size,
				enum_->name.begin);
		for (u32 valueIdx = 0; valueIdx < enum_->values.size; ++valueIdx)
		{
			EnumValue *value = &enum_->values[valueIdx];
			PrintToFile(file, "\t{ \"%.*s\", ", value->name.size, value->name.begin);
			PrintToFile(file, "%.*s", value->name.size, value->name.begin);
			PrintToFile(file, " },\n");
		}
		PrintToFile(file, "};\n");

		PrintToFile(file, "const EnumInfo enumInfo_%.*s = { ", enum_->name.size,
				enum_->name.begin);

		PrintToFile(file, "\"%.*s\", ", enum_->name.size, enum_->name.begin);
		PrintToFile(file, "%d, ", enum_->values.size);
		PrintToFile(file, "_enumInfoValues_%.*s ", enum_->name.size, enum_->name.begin);

		PrintToFile(file, "};\n\n");
	}

	// STRUCTS
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

			// Offset of
			const char *anonstruct = "anonstruct_";
			bool isAnonstruct = strncmp(struct_->name.begin, anonstruct, strlen(anonstruct)) == 0;
			if (isAnonstruct)
			{
				// name is of the form "anonstruct_ParentStruct_Member"
				const char *parentStruct = struct_->name.begin + strlen(anonstruct);

				const char *memberName = parentStruct;
				while (*memberName != '_')
					++memberName;
				++memberName;

				const i64 parentStructSize = memberName - parentStruct - 1; // -1 for the _
				const i64 memberNameSize = struct_->name.begin + struct_->name.size - memberName;

				PrintToFile(file, "sizeof(%.*s::%.*s.%.*s), ",
						parentStructSize, parentStruct,
						memberNameSize, memberName,
						member->name.size, member->name.begin);

				PrintToFile(file, "offsetof(%.*s, %.*s.%.*s) - offsetof(%.*s, %.*s)",
						parentStructSize, parentStruct,
						memberNameSize, memberName,
						member->name.size, member->name.begin,
						parentStructSize, parentStruct,
						memberNameSize, memberName);
			}
			else
			{
				PrintToFile(file, "sizeof(%.*s::%.*s), ", struct_->name.size,
						struct_->name.begin, member->name.size, member->name.begin);

				PrintToFile(file, "offsetof(%.*s, %.*s)", struct_->name.size,
						struct_->name.begin, member->name.size, member->name.begin);
			}

			if (member->type == TYPE_STRUCT)
			{
				PrintToFile(file, ", &typeInfo_%.*s", member->typeInfo.name.size,
						member->typeInfo.name.begin);
			}
			else if (member->type == TYPE_ENUM)
			{
				PrintToFile(file, ", &enumInfo_%.*s", member->typeInfo.name.size,
						member->typeInfo.name.begin);
			}
			PrintToFile(file, " },\n");
		}
		PrintToFile(file, "};\n");

		PrintToFile(file, "const StructInfo typeInfo_%.*s = { ", struct_->name.size,
				struct_->name.begin);

		PrintToFile(file, "\"%.*s\", ", struct_->name.size, struct_->name.begin);
		PrintToFile(file, "%d, ", struct_->memberCount);
		PrintToFile(file, "_typeInfoMembers_%.*s ", struct_->name.size, struct_->name.begin);

		PrintToFile(file, "};\n");
	}

	PrintToFile(file, "#endif\n");

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

#if 1
	const char *gameFile = "src/Game.cpp";
	DynamicArray_Token gameTokens;
	DynamicArrayInit_Token(&gameTokens, 4096, malloc);
	ParseFile(gameFile, gameTokens);

	DynamicArray_Struct structs;
	DynamicArray_Enum enums;
	ReadStructsAndEnums(gameTokens, structs, enums);

	WriteTypeInfoFiles(structs, enums);
#endif

	DynamicArray_Procedure procedures;
	DynamicArrayInit_Procedure(&procedures, 64, malloc);

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
