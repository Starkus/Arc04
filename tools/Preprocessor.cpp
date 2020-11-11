#include <stdio.h>
#include "General.h"
#include "Maths.h"
#include "Platform.h"
#include "Containers.h"

#include <windows.h>
#include <strsafe.h>
HANDLE g_hStdout;
#include "Win32Common.cpp"

#define LOG(...) Log(__VA_ARGS__)

void *MallocPlus1(u64 size)
{
	return malloc(size + 1);
}

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

struct Type
{
	Token name;
	bool isConst;
	bool isFunction;
	int ptrLevels;
	char *paramsBegin;
	int paramsSize;
};

struct Param
{
	Type type;
	Token name;
	bool isVarArgs;
};

struct Procedure
{
	Type returnType;
	Token name;
	Param params[16];
	int paramCount;
};
DECLARE_DYNAMIC_ARRAY(Procedure);

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
		//LOG("String literal: \"%.*s\"\n", result.size, result.begin);
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
		//LOG("Identifier: %.*s\n", result.size, result.begin);
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
		//LOG("Number: %.*s\n", result.size, result.begin);
	}
	else if (*tokenizer->cursor == '#')
	{
		result.type = TOKEN_PREPROCESSOR_DIRECTIVE;
		result.begin = tokenizer->cursor;
		result.size = EatRestOfLine(tokenizer);
		//LOG("Preprocessor directive: %.*s\n", result.size, result.begin);
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

#if 0
	if (result.type == TOKEN_INVALID)
		LOG("\\%.*s ", result.size, result.begin);
	else if (result.type >= TOKEN_ASCII_BEGIN && result.type < TOKEN_ASCII_END)
		LOG("`%c ", result.type);
	else
		LOG("@%.*s ", result.size, result.begin);
#endif

	return result;
}

void PrintType(char *buffer, const Type *type)
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

DynamicArray_Token ParseFile(const char *filename)
{
	char *fileBuffer;
	u64 fileSize;
	PlatformReadEntireFile(filename, (u8 **)&fileBuffer, &fileSize, MallocPlus1);
	fileBuffer[fileSize] = 0;

	DynamicArray_Token tokens;
	DynamicArrayInit_Token(&tokens, 4096, malloc);
	Tokenizer tokenizer = {};
	tokenizer.cursor = fileBuffer;
	while (true)
	{
		Token newToken = ReadTokenAndAdvance(&tokenizer);
		newToken.file = filename;
		if (newToken.type == TOKEN_END_OF_FILE)
			break;
		tokens[DynamicArrayAdd_Token(&tokens, realloc)] = newToken;
	}

	return tokens;
}

void PrintStructs(DynamicArray_Token &tokens)
{
	for (u32 tokenIdx = 0; tokenIdx < tokens.size; ++tokenIdx)
	{
		Token *token = &tokens[tokenIdx];
		if (token->type == TOKEN_IDENTIFIER)
		{
			if (strncmp(token->begin, "struct", token->size) == 0)
			{
				token = &tokens[++tokenIdx];
				if (token->type == TOKEN_IDENTIFIER)
					LOG("Found struct of name '%.*s'\n", token->size, token->begin);
				else
					LOG("Found annonymous struct\n");
			}
		}
	}
}

int ExtractProcedures(DynamicArray_Token &tokens, DynamicArray_Procedure &procedures)
{
	for (u32 tokenIdx = 0; tokenIdx < tokens.size; ++tokenIdx)
	{
		Token *token = &tokens[tokenIdx];
		if (token->type == TOKEN_IDENTIFIER)
		{
			if (strncmp(token->begin, "PLATFORMPROC", token->size) == 0)
			{
				Procedure newProcedure = {};

				Token *curToken = &tokens[++tokenIdx];
				// Modifier?
				if (strncmp(curToken->begin, "const", curToken->size) == 0)
				{
					newProcedure.returnType.isConst = true;

					curToken = &tokens[++tokenIdx];
					if (curToken->type != TOKEN_IDENTIFIER)
					{
						LOG("ERROR: Parsing platform procedure: expected parameter type! %s:%d\n",
								token->file, token->line);
						return 1;
					}
				}

				if (curToken->type != TOKEN_IDENTIFIER)
				{
					LOG("ERROR: Parsing platform procedure: expected return type! %s:%d\n",
							token->file, token->line);
					return 1;
				}
				newProcedure.returnType.name = *curToken;

				// Pointer?
				curToken = &tokens[++tokenIdx];
				while (curToken->type == '*')
				{
					++newProcedure.returnType.ptrLevels;
					curToken = &tokens[++tokenIdx];
				}

				if (curToken->type != TOKEN_IDENTIFIER)
				{
					LOG("ERROR: Parsing platform procedure: expected procedure name! %s:%d\n",
							token->file, token->line);
					return 1;
				}
				newProcedure.name = *curToken;

				curToken = &tokens[++tokenIdx];
				if (curToken->type != '(')
				{
					LOG("ERROR: Parsing platform procedure: expected '('! %s:%d\n",
							token->file, token->line);
					return 1;
				}

				// Get params
				for (;;)
				{
					Param newParam = {};

					curToken = &tokens[++tokenIdx];
					if (curToken->type == ')')
						break;
					else if (curToken->type == ',' && newProcedure.paramCount)
						continue;

					if (curToken->type == '.')
					{
						for (int i = 0; i < 2; ++i)
						{
							curToken = &tokens[++tokenIdx];
							if (curToken->type != '.')
							{
								LOG("ERROR: Parsing platform procedure: syntax error! %s:%d\n",
										token->file, token->line);
								return 1;
							}
						}
						newParam.isVarArgs = true;

						ASSERT(newProcedure.paramCount < 16);
						newProcedure.params[newProcedure.paramCount++] = newParam;

						break;
					}

					if (curToken->type != TOKEN_IDENTIFIER)
					{
						LOG("ERROR: Parsing platform procedure: expected parameter type! %s:%d\n",
								token->file, token->line);
						return 1;
					}

					// Modifier?
					if (strncmp(curToken->begin, "const", curToken->size) == 0)
					{
						newParam.type.isConst = true;

						curToken = &tokens[++tokenIdx];
						if (curToken->type != TOKEN_IDENTIFIER)
						{
							LOG("ERROR: Parsing platform procedure: expected parameter type! %s:%d\n",
								token->file, token->line);
							return 1;
						}
					}
					newParam.type.name = *curToken;

					// Pointer?
					curToken = &tokens[++tokenIdx];
					while (curToken->type == '*')
					{
						++newParam.type.ptrLevels;
						curToken = &tokens[++tokenIdx];
					}

					// Parenthesis?
					bool openedParenthesis = false;
					if (curToken->type == '(')
					{
						openedParenthesis = true;
						curToken = &tokens[++tokenIdx];
					}

					if (curToken->type == '*')
					{
						newParam.type.isFunction = true;
						curToken = &tokens[++tokenIdx];
					}

					// Name
					if (curToken->type != TOKEN_IDENTIFIER)
					{
						LOG("ERROR: Parsing platform procedure: expected parameter name! %s:%d\n",
								token->file, token->line);
						return 1;
					}
					newParam.name = *curToken;

					if (openedParenthesis)
					{
						curToken = &tokens[++tokenIdx];
						ASSERT(curToken->type == ')');
					}

					if (curToken->type == ')')
					{
						curToken = &tokens[++tokenIdx];
					}

					// Function pointer
					if (newParam.type.isFunction)
					{
						if (curToken->type != '(')
						{
							LOG("ERROR: Parsing platform procedure: expected function pointer parameters! %s:%d\n",
									token->file, token->line);
						}
						curToken = &tokens[++tokenIdx];

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
							curToken = &tokens[++tokenIdx];
						}
					}

					ASSERT(newProcedure.paramCount < 16);
					newProcedure.params[newProcedure.paramCount++] = newParam;
				}

				procedures[DynamicArrayAdd_Procedure(&procedures, realloc)] = newProcedure;
			}
		}
	}
	return 0;
}

void WritePlatformCodeFiles(DynamicArray_Procedure &procedures)
{
#define LOGFILE(file, ...) do { LOG(__VA_ARGS__); PlatformPrintToFile(file, __VA_ARGS__); } while (false)

	LOG("HEADER:\n");
	FileHandle file = PlatformOpenForWrite("gen/PlatformCode.h");

	LOGFILE(file, "struct PlatformCode\n{\n");

	for (u32 procIdx = 0; procIdx < procedures.size; ++procIdx)
	{
		Procedure *proc = &procedures[procIdx];

		char typeStr[256];
		PrintType(typeStr, &proc->returnType);
		LOGFILE(file, "\t%s(*%.*s)(", typeStr, proc->name.size, proc->name.begin);
		for (int paramIdx = 0; paramIdx < proc->paramCount; ++paramIdx)
		{
			if (paramIdx)
				LOGFILE(file, ", ");

			if (proc->params[paramIdx].isVarArgs)
			{
				LOGFILE(file, "...");
				continue;
			}

			Type *paramType = &proc->params[paramIdx].type;
			PrintType(typeStr, paramType);

			Token *name = &proc->params[paramIdx].name;

			if (proc->params[paramIdx].type.isFunction)
			{
				LOGFILE(file, "%s(*%.*s)(%.*s)", typeStr, name->size, name->begin,
						paramType->paramsSize,
						paramType->paramsBegin);
			}
			else
			{
				LOGFILE(file, "%s%.*s", typeStr, name->size, name->begin);
			}
		}
		LOGFILE(file, ");\n");
	}

	LOGFILE(file, "};\n");
	PlatformCloseFile(file);

	LOG("SOURCE:\n");
	file = PlatformOpenForWrite("gen/PlatformCode.cpp");

	LOGFILE(file, "inline void FillPlatformCodeStruct(PlatformCode *platformCode)\n{\n");

	for (u32 procIdx = 0; procIdx < procedures.size; ++procIdx)
	{
		Procedure *proc = &procedures[procIdx];

		LOGFILE(file, "\tplatformCode->%.*s = %.*s;\n", proc->name.size, proc->name.begin,
				proc->name.size, proc->name.begin);
	}

	LOGFILE(file, "};\n");
	PlatformCloseFile(file);

#undef LOGFILE
}

int main(int argc, char **argv)
{
	(void)argc, argv;

	AttachConsole(ATTACH_PARENT_PROCESS);
	g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);


	DynamicArray_Procedure procedures;
	DynamicArrayInit_Procedure(&procedures, 64, malloc);

	const char *folder = "src/";

	PlatformFindData findData;
	PlatformSearchHandle searchHandle;
	bool success = PlatformFindFirstFile(&searchHandle, folder, &findData);
	if (!success)
		return 0;

	do
	{
		const char *curFilename = PlatformGetCurrentFilename(&findData);
		if (strcmp(curFilename, ".") == 0 || strcmp(curFilename, "..") == 0)
			continue;

		char fullname[MAX_PATH];
		sprintf(fullname, "%s%s", folder, curFilename);

		DynamicArray_Token tokens = ParseFile(fullname);
		int error = ExtractProcedures(tokens, procedures);
		if (error)
			return error;
	} while (PlatformFindNextFile(searchHandle, &findData));
	PlatformFindClose(searchHandle);

	WritePlatformCodeFiles(procedures);

	return 0;
}
