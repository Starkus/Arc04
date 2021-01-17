void Indent(StringStream *stream, i32 indentLevel)
{
	for (int i = 0; i < indentLevel; ++i)
		StreamWrite(stream, "  ");
}

void SerializeStruct(StringStream *stream, const void *ptr, const StructInfo *structInfo, i32 indentLevel = 0);
void SerializeStructMember(StringStream *stream, const void *ptr, const StructMember *memberInfo, i32 indentLevel = 0)
{
	u8 *memberPtr = (u8 *)ptr;

	// Check for NoSerialize tag
	for (u32 tagIdx = 0; tagIdx < memberInfo->tagCount; ++tagIdx)
	{
		if (strcmp(memberInfo->tags[tagIdx], "NoSerialize") == 0)
		{
			return;
		}
	}

	Indent(stream, indentLevel);
	StreamWrite(stream, "%s = ", memberInfo->name);

	// Pointer
	if (memberInfo->pointerLevels != 0)
	{
		if (memberInfo->typeInfo == &typeInfo_Resource)
		{
			const Resource *resourcePtr = *(const Resource**)memberPtr;
			if (resourcePtr)
			{
				ResourceVoucher voucher;
				voucher.type = resourcePtr->type;
				voucher.filename = { resourcePtr->filename, strlen(resourcePtr->filename) };
				StreamWrite(stream, "{\n");
				SerializeStruct(stream, &voucher, &typeInfo_ResourceVoucher, indentLevel + 1);
				Indent(stream, indentLevel); StreamWrite(stream, "}");
			}
		}
		else
		{
			StreamWrite(stream, "0x%llX", *(u64 *)memberPtr);
		}
	}
	else switch (memberInfo->type)
	{
	case TYPE_U8:
		StreamWrite(stream, "%hhu", *(u8 *)memberPtr); break;
	case TYPE_U16:
		StreamWrite(stream, "%hu", *(u16 *)memberPtr); break;
	case TYPE_U32:
		StreamWrite(stream, "%u", *(u32 *)memberPtr); break;
	case TYPE_U64:
		StreamWrite(stream, "%llu", *(u64 *)memberPtr); break;
	case TYPE_I8:
		StreamWrite(stream, "%hhd", *(i8 *)memberPtr); break;
	case TYPE_I16:
		StreamWrite(stream, "%hd", *(i16 *)memberPtr); break;
	case TYPE_I32:
		StreamWrite(stream, "%d", *(i32 *)memberPtr); break;
	case TYPE_I64:
		StreamWrite(stream, "%lld", *(i64 *)memberPtr); break;
	case TYPE_F32:
		StreamWrite(stream, "%f", *(f32 *)memberPtr); break;
	case TYPE_F64:
		StreamWrite(stream, "%Lf", *(f64 *)memberPtr); break;
	case TYPE_BOOL:
		StreamWrite(stream, "%s", *(i32 *)memberPtr == 0 ? "false" : "true"); break;
	case TYPE_STRUCT:
	{
		if (memberInfo->typeInfo == &typeInfo_String)
		{
			String *string = (String *)memberPtr;
			StreamWrite(stream, "\"%.*s\"", string->size, string->data);
		}
		else if (memberInfo->typeInfo == &typeInfo_v2)
		{
			v2 *vec = (v2 *)memberPtr;
			StreamWrite(stream, "{ %f, %f }", vec->x, vec->y);
		}
		else if (memberInfo->typeInfo == &typeInfo_v3)
		{
			v3 *vec = (v3 *)memberPtr;
			StreamWrite(stream, "{ %f, %f, %f }", vec->x, vec->y, vec->z);
		}
		else if (memberInfo->typeInfo == &typeInfo_v4)
		{
			v4 *vec = (v4 *)memberPtr;
			StreamWrite(stream, "{ %f, %f, %f, %f }", vec->x, vec->y, vec->z, vec->w);
		}
		else
		{
			StreamWrite(stream, "{\n");
			const StructInfo *memberStructInfo = (StructInfo *)memberInfo->typeInfo;
			SerializeStruct(stream, memberPtr, memberStructInfo, indentLevel + 1);
			Indent(stream, indentLevel); StreamWrite(stream, "}");
		}
	} break;
	case TYPE_ENUM:
	{
		const EnumInfo *memberEnumInfo = (EnumInfo *)memberInfo->typeInfo;
		i64 value;
		switch (memberInfo->size)
		{
			case 1:  value = *(i8  *)memberPtr; break;
			case 2:  value = *(i16 *)memberPtr; break;
			case 4:  value = *(i32 *)memberPtr; break;
			default: value = *(i64 *)memberPtr; break;
		}
		const char *enumValueName = nullptr;
		for (u32 i = 0; i < memberEnumInfo->valueCount; ++i)
		{
			if (value == memberEnumInfo->values[i].value)
			{
				enumValueName = memberEnumInfo->values[i].name;
			}
		}
		StreamWrite(stream, "%s", enumValueName);
	} break;
	}

	StreamWrite(stream, "\n");
}

void SerializeStruct(StringStream *stream, const void *ptr, const StructInfo *structInfo, i32 indentLevel)
{
	u8 *structPtr = (u8 *)ptr;

	for (u32 i = 0; i < structInfo->memberCount; ++i)
	{
		const StructMember *memberInfo = &structInfo->members[i];
		void *memberPtr = structPtr + memberInfo->offset;

		SerializeStructMember(stream, memberPtr, memberInfo, indentLevel);
	}
}

u64 ReadUint(Token **cursor)
{
	Token *token = *cursor;

	ASSERT(token->type == TOKEN_LITERAL_NUMBER);
	u64 n = atoi(token->begin);
	++token;

	*cursor = token;

	return n;
}

i64 ReadInt(Token **cursor)
{
	Token *token = *cursor;

	bool neg = false;
	if (token->type == '-')
	{
		neg = true;
		++token;
	}
	ASSERT(token->type == TOKEN_LITERAL_NUMBER);
	i64 n = atoi(token->begin);
	++token;

	if (neg) n = -n;

	*cursor = token;

	return n;
}

f64 ReadFloat(Token **cursor)
{
	Token *token = *cursor;

	bool neg = false;
	if (token->type == '-')
	{
		neg = true;
		++token;
	}
	ASSERT(token->type == TOKEN_LITERAL_NUMBER);
	f64 n = atof(token->begin);
	++token;

	if (neg) n = -n;

	*cursor = token;

	return n;
}

Token *DeserializeStruct(void *ptr, Token *token, const StructInfo *structInfo);
Token *DeserializeStructMember(void *memberPtr, Token *token, const StructMember *memberInfo)
{
	// Pointer
	if (memberInfo->pointerLevels != 0)
	{
		if (memberInfo->typeInfo == &typeInfo_Resource && memberInfo->pointerLevels == 1)
		{
			// Read resource name string and load/get resource
			ResourceVoucher voucher;
			token = DeserializeStruct(&voucher, token, &typeInfo_ResourceVoucher);
			ASSERT(token->type == '}'); // ?
			++token;

			const char *cstr = StringToCStr(&voucher.filename, FrameAlloc);
			const Resource *res = GetResource(cstr);
			if (!res)
				res = LoadResource(voucher.type, cstr);

			*(const Resource **)memberPtr = res;
		}
		else
		{
			*(u64 *)memberPtr = (u64)ReadUint(&token);
		}
	}
	else switch (memberInfo->type)
	{
	case TYPE_U8:
		// @Improve: check for over/underflows
		*(u8  *)memberPtr =  (u8)ReadUint(&token); break;
	case TYPE_U16:
		*(u16 *)memberPtr = (u16)ReadUint(&token); break;
	case TYPE_U32:
		*(u32 *)memberPtr = (u32)ReadUint(&token); break;
	case TYPE_U64:
		*(u64 *)memberPtr = (u64)ReadUint(&token); break;
	case TYPE_I8:
		*(i8  *)memberPtr =  (i8)ReadInt(&token); break;
	case TYPE_I16:
		*(i16 *)memberPtr = (i16)ReadInt(&token); break;
	case TYPE_I32:
		*(i32 *)memberPtr = (i32)ReadInt(&token); break;
	case TYPE_I64:
		*(i64 *)memberPtr = (i64)ReadInt(&token); break;
	case TYPE_F32:
		*(f32 *)memberPtr = (f32)ReadFloat(&token); break;
	case TYPE_F64:
		*(f64 *)memberPtr = (f64)ReadFloat(&token); break;
	case TYPE_BOOL:
		if (TokenIsStr(token, "true"))
			*(bool *)memberPtr = true;
		else if (TokenIsStr(token, "false"))
			*(bool *)memberPtr = false;
		else
			ASSERT(false);
		break;
	case TYPE_STRUCT:
	{
		if (memberInfo->typeInfo == &typeInfo_String)
		{
			String *string = (String *)memberPtr;
			ASSERT(token->type == TOKEN_LITERAL_STRING);
			string->data = token->begin;
			string->size = token->size;
			++token;
		}
		else if (memberInfo->typeInfo == &typeInfo_v2)
		{
			ASSERT(token->type == '{');
			++token;

			v2 *vec = (v2 *)memberPtr;
			vec->x = (f32)ReadFloat(&token);
			ASSERT(token->type == ',');
			++token;
			vec->y = (f32)ReadFloat(&token);

			ASSERT(token->type == '}');
			++token;
		}
		else if (memberInfo->typeInfo == &typeInfo_v3)
		{
			ASSERT(token->type == '{');
			++token;

			v3 *vec = (v3 *)memberPtr;
			vec->x = (f32)ReadFloat(&token);
			ASSERT(token->type == ',');
			++token;
			vec->y = (f32)ReadFloat(&token);
			ASSERT(token->type == ',');
			++token;
			vec->z = (f32)ReadFloat(&token);

			ASSERT(token->type == '}');
			++token;
		}
		else if (memberInfo->typeInfo == &typeInfo_v4)
		{
			ASSERT(token->type == '{');
			++token;

			v4 *vec = (v4 *)memberPtr;
			vec->x = (f32)ReadFloat(&token);
			ASSERT(token->type == ',');
			++token;
			vec->y = (f32)ReadFloat(&token);
			ASSERT(token->type == ',');
			++token;
			vec->z = (f32)ReadFloat(&token);
			ASSERT(token->type == ',');
			++token;
			vec->w = (f32)ReadFloat(&token);

			ASSERT(token->type == '}');
			++token;
		}
		else
		{
			const StructInfo *memberStructInfo = (StructInfo *)memberInfo->typeInfo;
			token = DeserializeStruct(memberPtr, token, memberStructInfo);

			ASSERT(token->type == '}');
			++token;
		}
	} break;
	case TYPE_ENUM:
	{
		const EnumInfo *memberEnumInfo = (EnumInfo *)memberInfo->typeInfo;
		i64 value = I64_MIN;
		for (u32 i = 0; i < memberEnumInfo->valueCount; ++i)
		{
			if (TokenIsStr(token, memberEnumInfo->values[i].name))
			{
				value = memberEnumInfo->values[i].value;
			}
		}
		switch (memberInfo->size)
		{
			case 1:  *(i8  *)memberPtr =  (i8)value; break;
			case 2:  *(i16 *)memberPtr = (i16)value; break;
			case 4:  *(i32 *)memberPtr = (i32)value; break;
			default: *(i64 *)memberPtr = (i64)value; break;
		}
		++token;
	} break;
	}

	return token;
}

Token *DeserializeStruct(void *ptr, Token *token, const StructInfo *structInfo)
{
	u8 *structPtr = (u8 *)ptr;

	ASSERT(token->type == '{');
	++token;
	while (token->type != '}')
	{
		Token *nameToken = token;
		++token;

		ASSERT(token->type == '=');
		++token;

		const StructMember *memberInfo = nullptr;
		for (u32 i = 0; i < structInfo->memberCount; ++i)
		{
			const StructMember *curMemberInfo = &structInfo->members[i];
			if (TokenIsStr(nameToken, curMemberInfo->name))
			{
				memberInfo = curMemberInfo;
				break;
			}
		}

		ASSERT(memberInfo);
		void *memberPtr = structPtr + memberInfo->offset;

		token = DeserializeStructMember(memberPtr, token, memberInfo);
	}
	return token;
}

void SerializeEntities(GameState *gameState, StringStream *stream)
{
	i32 indentLevel = 0;

	u32 entityCount = gameState->transforms.size;
	for (u32 entityIdx = 0; entityIdx < entityCount; ++entityIdx)
	{
		EntityHandle handle = EntityHandleFromTransformIndex(gameState, entityIdx); // @Improve

		// @Improve: don't skip player! Do something else like keep IDs not to break other people's
		// entity handles.
		if (handle.id == gameState->player.entityHandle.id)
			continue;

		Indent(stream, indentLevel); StreamWrite(stream, "Entity {\n");
		++indentLevel;

		//Indent(stream, indentLevel); StreamWrite(stream, "ID %d\n", handle.id);

		Transform *transform = GetEntityTransform(gameState, handle);
		Indent(stream, indentLevel); StreamWrite(stream, "Transform {\n");
		SerializeStruct(stream, transform, &typeInfo_Transform, indentLevel + 1);
		Indent(stream, indentLevel); StreamWrite(stream, "}\n");

		MeshInstance *mesh = GetEntityMesh(gameState, handle);
		if (mesh)
		{
			Indent(stream, indentLevel); StreamWrite(stream, "Mesh {\n");
			SerializeStruct(stream, mesh, &typeInfo_MeshInstance, indentLevel + 1);
			Indent(stream, indentLevel); StreamWrite(stream, "}\n");
		}

		SkinnedMeshInstance *skinnedMesh = GetEntitySkinnedMesh(gameState, handle);
		if (skinnedMesh)
		{
			Indent(stream, indentLevel); StreamWrite(stream, "SkinnedMesh {\n");
			SerializeStruct(stream, skinnedMesh, &typeInfo_SkinnedMeshInstance, indentLevel + 1);
			Indent(stream, indentLevel); StreamWrite(stream, "}\n");
		}

		Collider *collider = GetEntityCollider(gameState, handle);
		if (collider)
		{
			Indent(stream, indentLevel); StreamWrite(stream, "Collider {\n");

			SerializeStructMember(stream, &collider->type, &typeInfo_Collider.members[0], indentLevel + 1); // @Hardcoded
			SerializeStructMember(stream, &collider->cube, &typeInfo_Collider.members[collider->type + 1],
					indentLevel + 1); // @Hardcoded

			Indent(stream, indentLevel); StreamWrite(stream, "}\n");
		}

		ParticleSystem *particleSystem = GetEntityParticleSystem(gameState, handle);
		if (particleSystem)
		{
			Indent(stream, indentLevel); StreamWrite(stream, "ParticleSystem {\n");
			SerializeStruct(stream, particleSystem, &typeInfo_ParticleSystem, indentLevel + 1);
			Indent(stream, indentLevel); StreamWrite(stream, "}\n");
		}

		--indentLevel;
		Indent(stream, indentLevel); StreamWrite(stream, "}\n");
	}
}

void DeserializeEntities(GameState *gameState, const u8 *fileBuffer, u64 fileSize)
{
	DynamicArray_Token tokens;
	DynamicArrayInit_Token(&tokens, 4096, malloc);
	TokenizeFile((const char *)fileBuffer, fileSize, tokens);

	Token *token = tokens.data;
	while (token->type != TOKEN_END_OF_FILE)
	{
		ASSERT(TokenIsStr(token, "Entity"));
		++token;
		ASSERT(token->type == '{');
		++token;

		Transform *transform;
		EntityHandle entityHandle = AddEntity(gameState, &transform);

		// Read components
		while (token->type != '}')
		{
			ASSERT(token->type == TOKEN_IDENTIFIER);
			Token *nameToken = token;
			++token;

			if (TokenIsStr(nameToken, "Transform"))
			{
				token = DeserializeStruct(transform, token, &typeInfo_Transform);
			}
			else if (TokenIsStr(nameToken, "Collider"))
			{
				Collider *collider = ArrayAdd_Collider(&gameState->colliders);
				token = DeserializeStruct(collider, token, &typeInfo_Collider);
				EntityAssignCollider(gameState, entityHandle, collider);
			}
			else if (TokenIsStr(nameToken, "Mesh"))
			{
				MeshInstance *meshInstance = ArrayAdd_MeshInstance(&gameState->meshInstances);
				token = DeserializeStruct(meshInstance, token, &typeInfo_MeshInstance);
				EntityAssignMesh(gameState, entityHandle, meshInstance);
			}
			else if (TokenIsStr(nameToken, "SkinnedMesh"))
			{
				SkinnedMeshInstance *skinnedMeshInstance = ArrayAdd_SkinnedMeshInstance(&gameState->skinnedMeshInstances);
				token = DeserializeStruct(skinnedMeshInstance, token, &typeInfo_SkinnedMeshInstance);
				EntityAssignSkinnedMesh(gameState, entityHandle, skinnedMeshInstance);
			}
			else if (TokenIsStr(nameToken, "ParticleSystem"))
			{
				ParticleSystem *particleSystem = ArrayAdd_ParticleSystem(&gameState->particleSystems);
				particleSystem->deviceBuffer = CreateDeviceMesh(0); // @Improve: centralize these required initializations?
				token = DeserializeStruct(particleSystem, token, &typeInfo_ParticleSystem);
				EntityAssignParticleSystem(gameState, entityHandle, particleSystem);
			}
			else
			{
				// Unsupported/invalid component?
				// @Todo: cry
				ASSERT(token->type == '{');
				int levels = 1;
				while (levels)
				{
					++token;
					if (token->type == '{')
						++levels;
					else if (token->type == '}')
						--levels;
				}
			}

			ASSERT(token->type == '}');
			++token;
		}

		ASSERT(token->type == '}');
		++token;
	}
}
