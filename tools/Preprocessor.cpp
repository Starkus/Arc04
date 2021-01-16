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

#include "Strings.h"
#include "Parsing.cpp"

enum Type
{
	TYPE_INVALID,
	TYPE_VOID,
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
	TYPE_CHAR,
	TYPE_INT,
	TYPE_STRUCT,
	TYPE_ENUM,
	TYPE_COUNT
};

const char *TypeStrings[] =
{
	"TYPE_INVALID",
	"TYPE_VOID",
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
	"TYPE_CHAR",
	"TYPE_INT",
	"TYPE_STRUCT",
	"TYPE_ENUM"
};

const char *basicTypeStrings[] =
{
	"INVALID",
	"void",
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
	"char",
	"int",
	"struct",
	"enum"
};

struct Param;
struct Struct;
struct Enum;
struct TypeInfo
{
	Type type;
	bool isConst;
	bool isStatic;
	bool isFunction;
	bool isFunctionPtr;
	int ptrLevels;
	int count;
	Param *params;
	int paramCount;
	Struct *structInfo;
	Enum *enumInfo;
};

struct Param
{
	TypeInfo typeInfo;
	Token name;
	bool isVarArgs;
};

struct Procedure
{
	TypeInfo returnType;
	Token name;
	Param params[16];
	int paramCount;
	i32 tagCount;
	Token tags[8];
};
DECLARE_ARRAY(Procedure);

struct StructMember
{
	Token name;
	TypeInfo typeInfo;
	i32 tagCount;
	Token tags[8];
};

struct Struct
{
	Token name;
	int memberCount;
	StructMember members[64]; // @Improve: no fixed size array?
	bool isAnonymous;
	Token openingBrace;
	Token closingBrace;
};
DECLARE_ARRAY(Struct);

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
DECLARE_ARRAY(Enum);

struct ParsedFile
{
	Array_Struct structs;
	Array_Enum enums;
	Array_Procedure procedures;
};

void PrintTypeInfo(char *buffer, const TypeInfo *typeInfo)
{
	char stars[8] = {};
	for (int i = 0; i < typeInfo->ptrLevels; ++i)
	{
		stars[i] = '*';
		stars[i + 1] = 0;
	}
	if (typeInfo->type == TYPE_STRUCT)
	{
		sprintf(buffer, "%s%s%.*s %s",
			typeInfo->isStatic ? "static " : "",
			typeInfo->isConst ? "const " : "",
			typeInfo->structInfo->name.size, typeInfo->structInfo->name.begin,
			stars);
	}
	else if (typeInfo->type == TYPE_ENUM)
	{
		sprintf(buffer, "%s%s%.*s %s",
			typeInfo->isStatic ? "static " : "",
			typeInfo->isConst ? "const " : "",
			typeInfo->enumInfo->name.size, typeInfo->enumInfo->name.begin,
			stars);
	}
	else
	{
		sprintf(buffer, "%s%s%s %s",
			typeInfo->isStatic ? "static " : "",
			typeInfo->isConst ? "const " : "",
			basicTypeStrings[typeInfo->type],
			stars);
	}
}

Token *ParseStruct(ParsedFile *context, Token *token, Struct *struct_);

Token *ParseType(ParsedFile *context, Token *token, TypeInfo *typeInfo, Struct *inlineStruct)
{
	*typeInfo = {};
	typeInfo->type = TYPE_INVALID;

	// Modifiers
	while (true)
	{
		if (TokenIsStr(token, "static"))
		{
			typeInfo->isStatic = true;
			++token;
		}
		else if (TokenIsStr(token, "const"))
		{
			typeInfo->isConst = true;
			++token;
		}
		else if (TokenIsStr(token, "inline"))
		{
			// @Hack: implement
			++token;
		}
		else
		{
			break;
		}
	}

	// Inline struct
	if (TokenIsStr(token, "struct") || TokenIsStr(token, "union"))
	{
		typeInfo->type = TYPE_STRUCT;
		token = ParseStruct(context, token, inlineStruct);

		return token;
	}

	// Basic types
	if (TokenIsStr(token, "void"))
	{
		typeInfo->type = TYPE_VOID;
		++token;
	}
	else if (TokenIsStr(token, "u8"))
	{
		typeInfo->type = TYPE_U8;
		++token;
	}
	else if (TokenIsStr(token, "u16"))
	{
		typeInfo->type = TYPE_U16;
		++token;
	}
	else if (TokenIsStr(token, "u32"))
	{
		typeInfo->type = TYPE_U32;
		++token;
	}
	else if (TokenIsStr(token, "u64"))
	{
		typeInfo->type = TYPE_U64;
		++token;
	}

	else if (TokenIsStr(token, "i8"))
	{
		typeInfo->type = TYPE_I8;
		++token;
	}
	else if (TokenIsStr(token, "i16"))
	{
		typeInfo->type = TYPE_I16;
		++token;
	}
	else if (TokenIsStr(token, "i32"))
	{
		typeInfo->type = TYPE_I32;
		++token;
	}
	else if (TokenIsStr(token, "i64") ||
			TokenIsStr(token, "long"))
	{
		typeInfo->type = TYPE_I64;
		++token;
	}

	else if (TokenIsStr(token, "f32") ||
			TokenIsStr(token, "float"))
	{
		typeInfo->type = TYPE_F32;
		++token;
	}
	else if (TokenIsStr(token, "f64") ||
			TokenIsStr(token, "double"))
	{
		typeInfo->type = TYPE_F64;
		++token;
	}

	else if (TokenIsStr(token, "bool"))
	{
		typeInfo->type = TYPE_BOOL;
		++token;
	}
	else if (TokenIsStr(token, "char"))
	{
		typeInfo->type = TYPE_CHAR;
		++token;
	}
	else if (TokenIsStr(token, "int"))
	{
		typeInfo->type = TYPE_INT;
		++token;
	}

	else
	{
		for (u32 i = 0; i < context->structs.size; ++i)
		{
			if (context->structs[i].name.type == TOKEN_IDENTIFIER && // Avoid anonymous structs
					TokenIsEqual(token, &context->structs[i].name))
			{
				typeInfo->type = TYPE_STRUCT;
				typeInfo->structInfo = &context->structs[i];
				++token;
				break;
			}
		}
		if (typeInfo->type == TYPE_INVALID)
		{
			for (u32 i = 0; i < context->enums.size; ++i)
			{
				if (context->enums[i].name.type == TOKEN_IDENTIFIER && // Avoid anonymous enums
						TokenIsEqual(token, &context->enums[i].name))
				{
					typeInfo->type = TYPE_ENUM;
					typeInfo->enumInfo = &context->enums[i];
					++token;
					break;
				}
			}
		}
	}

	return token;
}

Token *ParseVariable(ParsedFile *context, Token *token, TypeInfo *typeInfo, Token *name)
{
	while (token->type == '*' || token->type == '&')
	{
		if (token->type == '*')
			++typeInfo->ptrLevels;
		else
			Log("WARNING: Ignoring reference type, not supported!\n");
		++token;
	}

	if (token->type == '(')
	{
		++token;
		ASSERT(token->type == '*');
		++token;
		typeInfo->isFunctionPtr = true;
	}

	if (token->type == TOKEN_IDENTIFIER)
	{
		//Log("Parsing variable: %.*s\n", token->size, token->begin);
		*name = *token;

		// @Hack: operator is the only identifier here that can contain weird symbols. Just skip
		// until '('.
		if (TokenIsStr(token, "operator"))
		{
			while (token->type != '(')
			{
				++token;
				ASSERT(token->type != TOKEN_END_OF_FILE);
			}
			name->size = (int)(token->begin - name->begin);
		}
		else
		{
			++token;
		}
	}

	if (typeInfo->isFunctionPtr)
	{
		ASSERT(token->type == ')');
		++token;
	}

	if (token->type == '(')
	{
		typeInfo->isFunction = true;
		typeInfo->params = (Param *)malloc(sizeof(Param) * 16);

		++token;
		while (token->type != ')')
		{
			Param param = {};

			if (token->type == '.')
			{
				for (int i = 0; i < 2; ++i)
				{
					++token;
					ASSERT(token->type == '.');
				}
				++token;
				param.isVarArgs = true;
			}
			else
			{
				Struct inlineStruct = {}; // @Hack: ignored!
				token = ParseType(context, token, &param.typeInfo, &inlineStruct);
				token = ParseVariable(context, token, &param.typeInfo, &param.name);
			}

			ASSERT(typeInfo->paramCount < 16);
			typeInfo->params[typeInfo->paramCount++] = param;

			if (token->type == ',')
			{
				++token;
				continue;
			}
		}
		++token;
	}
	else if (token->type == '[')
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
			typeInfo->count = atoi(token->begin);
			++token;
			ASSERT(token->type == ']');
		}
		++token;
	}

	if (token->type == TOKEN_IDENTIFIER && TokenIsStr(token, "const"))
	{
		// Constant member function :)
		++token;
	}

	// @Hack: Ignore default values
	if (token->type == '=')
	{
		++token;
		int defaultValueScopeLevel = 0;
		while (defaultValueScopeLevel ||
				(token->type != ';' && token->type != ',' && token->type != ')' && token->type != '@'))
		{
			if (token->type == '{')
				++defaultValueScopeLevel;
			else if (token->type == '}')
				--defaultValueScopeLevel;
			++token;
		}
	}
	return token;
}

Token *ParseStruct(ParsedFile *context, Token *token, Struct *struct_)
{
	*struct_ = {};

	ASSERT(TokenIsStr(token, "struct") || TokenIsStr(token, "union"));
	++token;

	if (token->type == TOKEN_IDENTIFIER)
	{
		//Log("Parsing struct: %.*s\n", token->size, token->begin);
		struct_->name = *token;
		++token;
	}
	else
	{
		//Log("Parsing anonymous struct\n");
		static u64 anonstructUniqueId = 0;

		char *anonymousTypeName = (char *)malloc(512);
		sprintf(anonymousTypeName, "anonstruct_%llu", anonstructUniqueId++);
		//Log("generated name: %s\n", anonymousTypeName);

		struct_->name.begin = anonymousTypeName;
		struct_->name.size = (int)strlen(anonymousTypeName);

		struct_->isAnonymous = true;
	}

	if (token->type == ';')
	{
		// Just declaration
		return token;
	}

	if (token->type != '{')
	{
		Log("ERROR! Reading struct, expected {\n");
		return token;
	}
	struct_->openingBrace = *token;
	++token;

	// Read struct members
	int scopeLevel = 1;
	while (scopeLevel && token->type != TOKEN_END_OF_FILE)
	{
		StructMember member = {};

		Struct inlineStruct = {};
		token = ParseType(context, token, &member.typeInfo, &inlineStruct);

		// Give anonymous struct a name if exists
		if (inlineStruct.memberCount)
		{
			if (token->type == TOKEN_IDENTIFIER)
			{
				// There are members with this struct as their type.
				Struct *persistentStruct = (Struct *)malloc(sizeof(Struct));
				*persistentStruct = inlineStruct;
				member.typeInfo.structInfo = persistentStruct;
			}
			else
			{
				// 'Headless' struct/union. Just pull the members into this one.
				for (int memIdx = 0; memIdx < inlineStruct.memberCount; ++memIdx)
				{
					struct_->members[struct_->memberCount++] = inlineStruct.members[memIdx];
				}
			}
		}

		while (1)
		{
			member.typeInfo.ptrLevels = 0;
			member.tagCount = 0;

			token = ParseVariable(context, token, &member.typeInfo, &member.name);

			// Tags
			while (token->type == '@')
			{
				++token;
				ASSERT(token->type == TOKEN_IDENTIFIER);
				member.tags[member.tagCount++] = *token;
				//Log("Tag found: @%.*s\n", token->size, token->begin);
				++token;
			}

			if (member.name.type == TOKEN_IDENTIFIER && member.name.size)
			{
				if (!member.typeInfo.isFunction && !member.typeInfo.isStatic)
					struct_->members[struct_->memberCount++] = member;
			}
			else
			{
				Log("WARNING! member with empty name!\n");
			}

			// Ignore function body
			if (token->type == '{')
			{
				++token;
				int functionBodyScopeLevel = 1;
				while (functionBodyScopeLevel)
				{
					if (token->type == '{')
						++functionBodyScopeLevel;
					else if (token->type == '}')
						--functionBodyScopeLevel;
					++token;
				}
			}

			if (token->type == ',')
				++token;
			else
				break;
		}

		if (token->type == ';')
		{
			++token;
		}
		else if (!member.typeInfo.isFunction)
		{
			Log("ERROR! Reading struct, expected ; got 0x%X(%c)\n", token->type, token->type);
		}

		if (token->type == '}')
		{
			--scopeLevel;
			++token;
			if (scopeLevel <= 0)
			{
				struct_->closingBrace = *token;
			}
			else
			{
				ASSERT(token->type == ';');
				++token;
			}
		}
	}
	ASSERT(token->type != TOKEN_END_OF_FILE);
	return token;
}

Token *ParseEnum(Token *token, Enum *enum_)
{
	if (token->type != '{')
	{
		Log("ERROR! Reading enum, expected {\n");
		return token;
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
		*DynamicArrayAdd_EnumValue(&enum_->values, realloc) = enumValue;

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

	return token;
}

ParsedFile ParseFile(DynamicArray_Token &tokens)
{
	ParsedFile parsedFile = {};
	ArrayInit_Struct(&parsedFile.structs, 8192, malloc);
	ArrayInit_Enum(&parsedFile.enums, 8192, malloc);
	ArrayInit_Procedure(&parsedFile.procedures, 8192, malloc);

	for (Token *token = &tokens[0]; token->type != TOKEN_END_OF_FILE; )
	{
		if (token->type == TOKEN_IDENTIFIER)
		{
			if (TokenIsStr(token, "extern"))
			{
				// Skip this garbage
				++token;

				// "C"
				if (token->type == TOKEN_LITERAL_STRING)
				{
					++token;
				}
			}
			else if (TokenIsStr(token, "static_assert") || TokenIsStr(token, "__declspec") ||
					TokenIsStr(token, "__attribute__"))
			{
				++token;
				ASSERT(token->type == '(');
				++token;
				int parenthesisLevels = 1;
				while (parenthesisLevels)
				{
					if (token->type == '(')
						++parenthesisLevels;
					else if (token->type == ')')
						--parenthesisLevels;
					++token;
				}
			}
			else if (TokenIsStr(token, "typedef"))
			{
				// Not implemented
				while (token->type != ';')
					++token;
				++token;
			}
			else if (TokenIsStr(token, "struct") || TokenIsStr(token, "union")) // @Improve
			{
				Struct struct_ = {};

				token = ParseStruct(&parsedFile, token, &struct_);

				if (struct_.name.type == TOKEN_IDENTIFIER && struct_.name.size)
				{
					// Check if struct has already been declared
					bool found = false;
					for (u32 structIdx = 0; structIdx < parsedFile.structs.size; ++structIdx)
					{
						if (TokenIsEqual(&parsedFile.structs[structIdx].name, &struct_.name))
						{
							if (parsedFile.structs[structIdx].memberCount != 0)
							{
								Log("ERROR! Struct redefinition!\n");
								ASSERT(false);
							}
							parsedFile.structs[structIdx] = struct_;
							found = true;
						}
					}
					if (!found)
						*ArrayAdd_Struct(&parsedFile.structs) = struct_;
				}

				ASSERT(token->type == ';');
				++token;
			}
			else if (strncmp(token->begin, "enum", token->size) == 0)
			{
				Enum enum_ = {};
				DynamicArrayInit_EnumValue(&enum_.values, 32, malloc);

				++token;
				if (token->type == TOKEN_IDENTIFIER)
				{
					enum_.name = *token;
					//Log("Found enum of name '%.*s'\n", token->size, token->begin);
					++token;
				}
				else
				{
					//Log("Found anonymous enum\n");
				}

				if (token->type == ';')
				{
					// Just declaration
					++token;
					continue;
				}

				token = ParseEnum(token, &enum_);

				if (enum_.name.type == TOKEN_IDENTIFIER && enum_.name.size)
				{
					*ArrayAdd_Enum(&parsedFile.enums) = enum_;
				}
			}
			else
			{
				// Read variable/procedure

				Struct inlineStruct;
				TypeInfo typeInfo;
				token = ParseType(&parsedFile, token, &typeInfo, &inlineStruct);
				if (typeInfo.type == TYPE_INVALID)
				{
					Log("WARNING! Found unknown type %.*s\n", token->size, token->begin);
					++token;
				}

				if (TokenIsStr(token, "__stdcall") || TokenIsStr(token, "WINAPI") ||
						TokenIsStr(token, "CALLBACK"))
				{
					++token;
				}

				while (true)
				{
					TypeInfo varTypeInfo = typeInfo;
					Token varName;
					token = ParseVariable(&parsedFile, token, &varTypeInfo, &varName);

					if (varTypeInfo.isFunction)
					{
						Procedure procedure = {};
						procedure.returnType = varTypeInfo;
						procedure.name = varName;
						procedure.paramCount = varTypeInfo.paramCount;
						for (int parIdx = 0; parIdx < varTypeInfo.paramCount; ++parIdx)
						{
							procedure.params[parIdx] = varTypeInfo.params[parIdx];
						}

						// @Improve?
						while (token->type == '@')
						{
							++token;
							ASSERT(token->type == TOKEN_IDENTIFIER);
							procedure.tags[procedure.tagCount++] = *token;
							//Log("Tag found: @%.*s\n", token->size, token->begin);
							++token;
						}

						*ArrayAdd_Procedure(&parsedFile.procedures) = procedure;
					}

					if (token->type == '{')
					{
						++token;
						int functionBodyScopeLevel = 1;
						while (functionBodyScopeLevel)
						{
							if (token->type == '{')
								++functionBodyScopeLevel;
							else if (token->type == '}')
								--functionBodyScopeLevel;
							++token;
						}
						break;
					}

					if (token->type == ';')
					{
						++token;
						break;
					}
					else
					{
						ASSERT(token->type == ',');
						++token;
					}
				}
#if 0
				else
				{
					// Assume expression starts with a variable or something
					// Eat up to semicolon

					//Log("Assuming not a type: %.*s\n", token->size, token->begin);

					while (token->type != ';')
					{
						ASSERT(token->type != TOKEN_END_OF_FILE);
						++token;
					}
				}
#endif
			}
		}
		else if (token->type == '@')
		{
			// @Hack: Ignoring these tags completely.
			++token;
			ASSERT(token->type == TOKEN_IDENTIFIER);
			++token;
		}
		else
		{
			++token;
		}
	}
	return parsedFile;
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

		TypeInfo *paramType = &proc->params[paramIdx].typeInfo;
		PrintTypeInfo(typeStr, paramType);

		Token *name = &proc->params[paramIdx].name;

		if (proc->params[paramIdx].typeInfo.isFunction)
		{
			PrintToFile(file, "%s(*%.*s)(", typeStr, name->size, name->begin);
			for (int i = 0; i < paramType->paramCount; ++i)
			{
				TypeInfo *funPtrParam = &paramType->params[i].typeInfo;
				PrintTypeInfo(typeStr, funPtrParam);
				PrintToFile(file, "%s", typeStr);
				if (i != paramType->paramCount - 1)
					PrintToFile(file, ", ", typeStr);
			}
			PrintToFile(file, ")");
		}
		else
		{
			PrintToFile(file, "%s%.*s", typeStr, name->size, name->begin);
		}
	}
	PrintToFile(file, ");\n");
}

void WritePlatformCodeFiles(Array_Procedure &procedures)
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

void WriteStruct(FileHandle file, Struct *struct_, Struct *parentStruct, Token *parentMemberName)
{
	// Tag lists
	for (int memberIdx = 0; memberIdx < struct_->memberCount; ++memberIdx)
	{
		StructMember *member = &struct_->members[memberIdx];
		if (member->tagCount)
		{
			PrintToFile(file, "const char *const _tags_%.*s_%.*s[] { ",
					struct_->name.size, struct_->name.begin,
					member->name.size, member->name.begin);
			for (int tagIdx = 0; tagIdx < member->tagCount; ++tagIdx)
			{
				PrintToFile(file, "\"%.*s\", ", member->tags[tagIdx].size,
						member->tags[tagIdx].begin);
			}
			PrintToFile(file, " };\n");
		}
	}

	// Inner structs
	for (int memberIdx = 0; memberIdx < struct_->memberCount; ++memberIdx)
	{
		StructMember *member = &struct_->members[memberIdx];
		if (member->typeInfo.type == TYPE_STRUCT)
		{
			Struct *innerStruct = member->typeInfo.structInfo;
			if (innerStruct->isAnonymous)
			{
				// Avoid duplicates
				bool alreadyWritten = false;
				for (int i = 0; i < memberIdx; ++i)
				{
					if (struct_->members[i].typeInfo.structInfo == innerStruct)
					{
						alreadyWritten = true;
						break;
					}
				}
				if (!alreadyWritten)
					WriteStruct(file, innerStruct, struct_, &member->name);
			}
		}
	}

	if (struct_->memberCount)
	{
		PrintToFile(file, "const StructMember _typeInfoMembers_%.*s[] =\n{\n", struct_->name.size,
				struct_->name.begin);
		for (int memberIdx = 0; memberIdx < struct_->memberCount; ++memberIdx)
		{
			StructMember *member = &struct_->members[memberIdx];
			PrintToFile(file, "\t{ \"%.*s\", ", member->name.size, member->name.begin);
			PrintToFile(file, "%s, ", TypeStrings[member->typeInfo.type]);
			PrintToFile(file, "%d, ", member->typeInfo.ptrLevels);
			PrintToFile(file, "%d, ", member->typeInfo.count);

			// Offset of
			if (struct_->isAnonymous)
			{
				PrintToFile(file, "sizeof(%.*s::%.*s.%.*s), ",
						parentStruct->name.size, parentStruct->name.begin,
						parentMemberName->size, parentMemberName->begin,
						member->name.size, member->name.begin);

				PrintToFile(file, "offsetof(%.*s, %.*s.%.*s) - offsetof(%.*s, %.*s)",
						parentStruct->name.size, parentStruct->name.begin,
						parentMemberName->size, parentMemberName->begin,
						member->name.size, member->name.begin,
						parentStruct->name.size, parentStruct->name.begin,
						parentMemberName->size, parentMemberName->begin);
			}
			else
			{
				PrintToFile(file, "sizeof(%.*s::%.*s), ", struct_->name.size,
						struct_->name.begin, member->name.size, member->name.begin);

				PrintToFile(file, "offsetof(%.*s, %.*s)", struct_->name.size,
						struct_->name.begin, member->name.size, member->name.begin);
			}

			bool typeInfoWritten = false;
			if (member->typeInfo.type == TYPE_STRUCT)
			{
				Token *nameToken = &member->typeInfo.structInfo->name;
				if (nameToken->begin)
				{
					PrintToFile(file, ", &typeInfo_%.*s", nameToken->size,
							nameToken->begin);
					typeInfoWritten = true;
				}
			}
			else if (member->typeInfo.type == TYPE_ENUM)
			{
				Token *nameToken = &member->typeInfo.enumInfo->name;
				if (nameToken->begin)
				{
					PrintToFile(file, ", &enumInfo_%.*s", nameToken->size,
							nameToken->begin);
					typeInfoWritten = true;
				}
			}
			if (!typeInfoWritten)
				PrintToFile(file, ", nullptr");

			if (member->tagCount)
			{
				PrintToFile(file, ", %d, _tags_%.*s_%.*s", member->tagCount,
						struct_->name.size, struct_->name.begin,
						member->name.size, member->name.begin);
			}

			PrintToFile(file, " },\n");
		}
		PrintToFile(file, "};\n");
	}

	PrintToFile(file, "const StructInfo typeInfo_%.*s = { ", struct_->name.size,
			struct_->name.begin);

	// Name
	PrintToFile(file, "\"%.*s\", ", struct_->name.size, struct_->name.begin);

	// Size
	if (struct_->memberCount)
	{
		if (!struct_->isAnonymous)
			PrintToFile(file, "sizeof(%.*s), ", struct_->name.size, struct_->name.begin);
		else
			PrintToFile(file, "sizeof(%.*s::%.*s), ", parentStruct->name.size,
					parentStruct->name.begin, parentMemberName->size, parentMemberName->begin);
	}
	else
		PrintToFile(file, "0, ");

	// Member count
	PrintToFile(file, "%d, ", struct_->memberCount);

	// Array of members
	if (struct_->memberCount)
		PrintToFile(file, "_typeInfoMembers_%.*s ", struct_->name.size, struct_->name.begin);
	else
		PrintToFile(file, "nullptr ");

	PrintToFile(file, "};\n");
}

void WriteTypeInfoFiles(Array_Struct &structs, Array_Enum &enums)
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
	PrintToFile(file,   "\tu32 tagCount;\n");
	PrintToFile(file,   "\tconst char *const *tags;\n");
	PrintToFile(file, "};\n\n");

	PrintToFile(file, "struct StructInfo\n{\n");
	PrintToFile(file,   "\tconst char *name;\n");
	PrintToFile(file,   "\tu64 size;\n");
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
		WriteStruct(file, struct_, nullptr, nullptr);
	}

	PrintToFile(file, "#endif\n");

	PlatformCloseFile(file);
}

void PostProcessSourceFile(ParsedFile *context, DynamicArray_Token &tokens, const char *outputFile)
{
	FileHandle file = PlatformOpenForWrite(outputFile);

	Token *token = &tokens[0];
	const char *begin = token->begin;
	while (token->type != TOKEN_END_OF_FILE)
	{
		if (token->type == '@')
		{
			Token *tagStr = token + 1;
			ASSERT(tagStr->type == TOKEN_IDENTIFIER);
			if (TokenIsStr(tagStr, "Using"))
			{
				// Alright so here's the thing. We need to stick this whole struct member inside a
				// union, and also throw the whole struct definition inside that union.

				// @Improve: instead of doing this, detect usings during parsing and save enough
				// information to replace the text at this stage.

				// @Incomplete: default value? modifiers?
				// @Incomplete: @Using tag NOT at end of statement?
				Token *typeToken = token - 2;
				ASSERT(typeToken->type == TOKEN_IDENTIFIER);
				Token *nameToken = token - 1;
				ASSERT(nameToken->type == TOKEN_IDENTIFIER);

				Token *semicolonToken = token + 2;
				// Skip other tags
				while (semicolonToken->type == '@')
					semicolonToken += 2;
				ASSERT(semicolonToken->type == ';');

				Struct *struct_ = nullptr;
				for (u32 structIdx = 0; structIdx < context->structs.size; ++structIdx)
				{
					if (TokenIsEqual(&context->structs[structIdx].name, typeToken))
					{
						struct_ = &context->structs[structIdx];
						break;
					}
				}
				if (!struct_)
				{
					Log("ERROR! Struct not found: %.*s\n", typeToken->size, typeToken->begin);
					ASSERT(false);
				}

				// Write up to this statement
				const char *end = typeToken->begin;
				PlatformWriteToFile(file, begin, end - begin);

				// Preamble
				PrintToFile(file, "union { ");

				// Print original statement
				begin = typeToken->begin;
				end = token->begin;
				PlatformWriteToFile(file, begin, end - begin);

				PrintToFile(file, "; struct");

				begin = struct_->openingBrace.begin;
				end = struct_->closingBrace.begin + 1;

				// Strip new lines
				// @Improve: Properly keep track of line numbers so we can just add #line directives
				u64 bufferSize = (u64)(end - begin);
				char *buffer = (char *)malloc(bufferSize);
				char *dst = buffer;
				bool lastWasWhitespace = false;
				for (const char *src = begin; src < end;)
				{
					if (IsWhitespace(*src))
					{
						if (!lastWasWhitespace)
							*dst++ = ' ';
						else
							--bufferSize;
						lastWasWhitespace = true;
						++src;
					}
					// @Hack: remove tags
					else if (*src == '@')
					{
						++src;
						--bufferSize;
						while (IsAlpha(*src) || IsNumeric(*src) || *src == '_')
						{
							++src;
							--bufferSize;
						}
					}
					else
					{
						*dst++ = *src++;
						lastWasWhitespace = false;
					}
				}
				PlatformWriteToFile(file, buffer, bufferSize);

				PrintToFile(file, " }");

				++token;
				begin = token->begin + token->size;
			}
			else
			{
				//Just remove tag
				const char *end = token->begin;
				PlatformWriteToFile(file, begin, end - begin);

				++token;
				if (token->type != TOKEN_IDENTIFIER)
				{
					Log("ERROR! No identifier token after @\n");
				}
				begin = token->begin + token->size;
			}
		}
		++token;
	}
	const char *end = token->begin;
	PlatformWriteToFile(file, begin, end - begin);

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

	bool isGame = false;
	bool isPlatform = false;
	const char *srcFile = nullptr;
	for (int argi = 0; argi < argc; ++argi)
	{
		Log("Arg %d: %s\n", argi, argv[argi]);
		if (strcmp(argv[argi], "-game") == 0)
		{
			isGame = true;
		}
		else if (strcmp(argv[argi], "-platform") == 0)
		{
			isPlatform = true;
		}
		else
		{
			srcFile = argv[argi];
		}
	}
	if (isGame == isPlatform)
	{
		Log("ERROR! Specify exactly one of -game or -platform\n");
		return 1;
	}
	if (srcFile == nullptr)
	{
		Log("ERROR! Specify a source file to preprocess\n");
		return 1;
	}

	u8 *fileBuffer;
	u64 fileSize;
	PlatformReadEntireFile(srcFile, &fileBuffer, &fileSize, malloc);

	DynamicArray_Token tokens;
	DynamicArrayInit_Token(&tokens, 4096, malloc);
	TokenizeFile((char *)fileBuffer, fileSize, tokens);

	ParsedFile parsedFile = ParseFile(tokens);

	if (isGame)
	{
		WriteTypeInfoFiles(parsedFile.structs, parsedFile.enums);
	}

	else if (isPlatform)
	{
		Array_Procedure platformProcedures;
		ArrayInit_Procedure(&platformProcedures, 512, malloc);

		for (u32 procIdx = 0; procIdx < parsedFile.procedures.size; ++procIdx)
		{
			Procedure *proc = &parsedFile.procedures[procIdx];

			for (int tagIdx = 0; tagIdx < proc->tagCount; ++tagIdx)
			{
				if (TokenIsStr(&proc->tags[tagIdx], "PlatformProc"))
				{
					*ArrayAdd_Procedure(&platformProcedures) = *proc;
					break;
				}
			}
		}

		WritePlatformCodeFiles(platformProcedures);
	}

	char outputFile[MAX_PATH];
	strcpy(outputFile, srcFile);
	ChangeExtension(outputFile, "cpp");
	PostProcessSourceFile(&parsedFile, tokens, outputFile);

	return 0;
}
