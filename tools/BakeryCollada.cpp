void ReadStringToFloats(const char *text, f32 *buffer, int count)
{
	f32 *bufferScan = buffer;
	int readCount = 0;

	char numBuffer[32];
	char *numBufferScan = numBuffer;
	for (const char *scan = text; ; ++scan)
	{
		if (*scan == ' ' || *scan == 0)
		{
			if (numBuffer != numBufferScan) // Empty string!
			{
				ASSERT(readCount < count);
				*numBufferScan = 0;
				*bufferScan++ = (f32)atof(numBuffer);
				numBufferScan = numBuffer;
				++readCount;
			}
		}
		else
		{
			*numBufferScan++ = *scan;
		}
		if (*scan == 0)
		{
			break;
		}
	}
	ASSERT(readCount == count);
}

void ReadStringToInts(const char *text, int *buffer, int count)
{
	int *bufferScan = buffer;
	int readCount = 0;

	char numBuffer[32];
	char *numBufferScan = numBuffer;
	for (const char *scan = text; ; ++scan)
	{
		if (*scan == ' ' || *scan == 0)
		{
			if (numBuffer != numBufferScan) // Empty string!
			{
				ASSERT(readCount < count);
				*numBufferScan = 0;
				*bufferScan++ = atoi(numBuffer);
				numBufferScan = numBuffer;
				++readCount;
			}
		}
		else
		{
			*numBufferScan++ = *scan;
		}
		if (*scan == 0)
		{
			break;
		}
	}
}

ErrorCode ReadColladaGeometry(XMLElement *rootEl, const char *geometryId, RawGeometry *result)
{
	*result = {};

	XMLElement *geometryEl = rootEl->FirstChildElement("library_geometries")
		->FirstChildElement("geometry");

	if (geometryId)
	{
		bool found = false;
		while (geometryEl)
		{
			if (strcmp(geometryEl->FindAttribute("name")->Value(), geometryId) == 0)
			{
				found = true;
				break;
			}
			geometryEl = geometryEl->NextSiblingElement("geometry");
		}
		if (!found)
		{
			Log("Error! couldn't find geometry with name \"%s\"\n", geometryId);
			return ERROR_NO_ELEMENT_WITH_ID;
		}
	}
	Log("Found geometry %s\n", geometryEl->FindAttribute("name")->Value());

	XMLElement *meshEl = geometryEl->FirstChildElement("mesh");

	XMLElement *verticesSource = 0;
	XMLElement *normalsSource = 0;
	XMLElement *positionsSource = 0;
	XMLElement *uvsSource = 0;
	XMLElement *triangleIndicesSource = 0;

	const char *verticesSourceName = 0;
	const char *normalsSourceName = 0;
	const char *positionsSourceName = 0;
	const char *uvsSourceName = 0;

	result->verticesOffset = -1;
	result->normalsOffset = -1;
	result->uvsOffset = -1;
	int attrCount = 0;

	// Get triangle source ids and index array
	{
		XMLElement *trianglesEl = meshEl->FirstChildElement("triangles");
		triangleIndicesSource = trianglesEl->FirstChildElement("p");
		result->triangleCount = trianglesEl->FindAttribute("count")->IntValue();

		XMLElement *inputEl = trianglesEl->FirstChildElement("input");
		while (inputEl != nullptr)
		{
			const char *semantic = inputEl->FindAttribute("semantic")->Value();
			const char *source = inputEl->FindAttribute("source")->Value();
			int offset = inputEl->FindAttribute("offset")->IntValue();
			if (strcmp(semantic, "VERTEX") == 0)
			{
				verticesSourceName = source + 1;
				result->verticesOffset = offset;
				++attrCount;
			}
			else if (strcmp(semantic, "NORMAL") == 0)
			{
				normalsSourceName = source + 1;
				result->normalsOffset = offset;
				++attrCount;
			}
			else if (strcmp(semantic, "TEXCOORD") == 0)
			{
				uvsSourceName = source + 1;
				result->uvsOffset = offset;
				++attrCount;
			}
			inputEl = inputEl->NextSiblingElement("input");
		}
	}

	// Find vertex node
	{
		XMLElement *verticesEl = meshEl->FirstChildElement("vertices");
		while (verticesEl != nullptr)
		{
			const char *id = verticesEl->FindAttribute("id")->Value();
			if (strcmp(id, verticesSourceName) == 0)
			{
				verticesSource = verticesEl;
			}
			verticesEl = verticesEl->NextSiblingElement("vertices");
		}
	}

	// Get vertex source ids
	{
		XMLElement *inputEl = verticesSource->FirstChildElement("input");
		while (inputEl != nullptr)
		{
			const char *semantic = inputEl->FindAttribute("semantic")->Value();
			const char *source = inputEl->FindAttribute("source")->Value();
			if (strcmp(semantic, "POSITION") == 0)
			{
				positionsSourceName = source + 1;
			}
			inputEl = inputEl->NextSiblingElement("input");
		}
	}

	// Find all sources
	{
		XMLElement *sourceEl = meshEl->FirstChildElement("source");
		while (sourceEl != nullptr)
		{
			const char *id = sourceEl->FindAttribute("id")->Value();
			if (strcmp(id, positionsSourceName) == 0)
			{
				positionsSource = sourceEl;
			}
			else if (strcmp(id, normalsSourceName) == 0)
			{
				normalsSource = sourceEl;
			}
			else if (strcmp(id, uvsSourceName) == 0)
			{
				uvsSource = sourceEl;
			}
			sourceEl = sourceEl->NextSiblingElement("source");
		}
	}

	// Read positions
	{
		XMLElement *arrayEl = positionsSource->FirstChildElement("float_array");
		int positionsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v3(&result->positions, positionsCount / 3, FrameAlloc);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)result->positions.data,
				positionsCount);
		result->positions.size = positionsCount / 3;
	}

	// Read normals
	result->normals = {};
	if (normalsSource)
	{
		XMLElement *arrayEl = normalsSource->FirstChildElement("float_array");
		int normalsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v3(&result->normals, normalsCount / 3, FrameAlloc);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)result->normals.data, normalsCount);
		result->normals.size = normalsCount / 3;
	}

	// Read uvs
	result->uvs = {};
	if (uvsSource)
	{
		XMLElement *arrayEl = uvsSource->FirstChildElement("float_array");
		int uvsCount = arrayEl->FindAttribute("count")->IntValue();
		ArrayInit_v2(&result->uvs, uvsCount / 2, FrameAlloc);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)result->uvs.data, uvsCount);
		result->uvs.size = uvsCount / 2;
	}

	// Read indices
	result->triangleIndices;
	{
		const int totalIndexCount = result->triangleCount * 3 * attrCount;

		ArrayInit_int(&result->triangleIndices, totalIndexCount, FrameAlloc);
		ReadStringToInts(triangleIndicesSource->FirstChild()->ToText()->Value(),
				result->triangleIndices.data, totalIndexCount);
		result->triangleIndices.size = totalIndexCount;
	}
	return ERROR_OK;
}

// WeightData is what gets injected into the vertex data, Skeleton is the controller, contains
// intrinsic joint information like bind poses
ErrorCode ReadColladaController(XMLElement *rootEl, Skeleton *skeleton, WeightData *weightData)
{
	XMLElement *controllerEl = rootEl->FirstChildElement("library_controllers")
		->FirstChildElement("controller");
	Log("Found controller %s\n", controllerEl->FindAttribute("name")->Value());
	XMLElement *skinEl = controllerEl->FirstChildElement("skin");

	const char *jointsSourceName = 0;
	const char *weightsSourceName = 0;
	XMLElement *weightCountSource = 0;
	XMLElement *weightIndicesSource = 0;
	int weightIndexCount = -1;
	{
		XMLElement *vertexWeightsEl = skinEl->FirstChildElement("vertex_weights");
		weightCountSource = vertexWeightsEl->FirstChildElement("vcount");
		weightIndicesSource = vertexWeightsEl->FirstChildElement("v");
		weightIndexCount = vertexWeightsEl->FindAttribute("count")->IntValue();

		XMLElement *inputEl = vertexWeightsEl->FirstChildElement("input");
		while (inputEl != nullptr)
		{
			const char *semantic = inputEl->FindAttribute("semantic")->Value();
			const char *source = inputEl->FindAttribute("source")->Value();
			if (strcmp(semantic, "JOINT") == 0)
			{
				jointsSourceName = source + 1;
			}
			else if (strcmp(semantic, "WEIGHT") == 0)
			{
				weightsSourceName = source + 1;
			}
			inputEl = inputEl->NextSiblingElement("input");
		}
	}

	const char *bindPosesSourceName = 0;
	{
		XMLElement *jointsEl = skinEl->FirstChildElement("joints");
		XMLElement *inputEl = jointsEl->FirstChildElement("input");
		while (inputEl != nullptr)
		{
			const char *semantic = inputEl->FindAttribute("semantic")->Value();
			const char *source = inputEl->FindAttribute("source")->Value();
			if (strcmp(semantic, "INV_BIND_MATRIX") == 0)
			{
				bindPosesSourceName = source + 1;
			}
			inputEl = inputEl->NextSiblingElement("input");
		}
	}

	XMLElement *jointsSource = 0;
	XMLElement *weightsSource = 0;
	XMLElement *bindPosesSource = 0;
	// Find all sources
	{
		XMLElement *sourceEl = skinEl->FirstChildElement("source");
		while (sourceEl != nullptr)
		{
			const char *id = sourceEl->FindAttribute("id")->Value();
			if (strcmp(id, jointsSourceName) == 0)
			{
				jointsSource = sourceEl;
			}
			else if (strcmp(id, weightsSourceName) == 0)
			{
				weightsSource = sourceEl;
			}
			else if (strcmp(id, bindPosesSourceName) == 0)
			{
				bindPosesSource = sourceEl;
			}
			sourceEl = sourceEl->NextSiblingElement("source");
		}
	}

	// Read weights
	weightData->weights = 0;
	{
		XMLElement *arrayEl = weightsSource->FirstChildElement("float_array");
		int count = arrayEl->FindAttribute("count")->IntValue();
		weightData->weights = (f32 *)FrameAlloc(sizeof(f32) * count);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), weightData->weights, count);
	}

	char *jointNamesBuffer = 0;
	// Read joint names
	{
		XMLElement *arrayEl = jointsSource->FirstChildElement("Name_array");
		skeleton->jointCount = (u8)arrayEl->FindAttribute("count")->IntValue();
		skeleton->ids = (char **)FrameAlloc(sizeof(char *) * skeleton->jointCount);

		const char *in = arrayEl->FirstChild()->ToText()->Value();
		jointNamesBuffer = (char *)FrameAlloc(sizeof(char *) * strlen(in));
		strcpy(jointNamesBuffer, in);
		char *wordBegin = jointNamesBuffer;
		int jointIdx = 0;
		for (char *scan = jointNamesBuffer; ; ++scan)
		{
			if (*scan == ' ')
			{
				if (wordBegin != scan)
				{
					skeleton->ids[jointIdx++] = wordBegin;
					*scan = 0;
				}
				wordBegin = scan + 1;
			}
			else if (*scan == 0)
			{
				if (wordBegin != scan)
				{
					skeleton->ids[jointIdx++] = wordBegin;
				}
				break;
			}
		}
	}

	// Read bind poses
	{
		XMLElement *arrayEl = bindPosesSource->FirstChildElement("float_array");
		int count = arrayEl->FindAttribute("count")->IntValue();
		ASSERT(count / 16 == skeleton->jointCount);
		skeleton->bindPoses = (mat4 *)FrameAlloc(sizeof(f32) * count);
		ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)skeleton->bindPoses,
				count);

		for (int i = 0; i < count / 16; ++i)
		{
			skeleton->bindPoses[i] = Mat4Transpose(skeleton->bindPoses[i]);
		}
	}

	// Read counts
	weightData->weightCounts = 0;
	{
		weightData->weightCounts = (int *)FrameAlloc(sizeof(int) * weightIndexCount);
		ReadStringToInts(weightCountSource->FirstChild()->ToText()->Value(), weightData->weightCounts, weightIndexCount);
	}

	// Read weight indices
	weightData->weightIndices = 0;
	{
		int count = 0;
		for (int i = 0; i < weightIndexCount; ++i)
			count += weightData->weightCounts[i] * 2; // FIXME hardcoded 2!
		weightData->weightIndices = (int *)FrameAlloc(sizeof(int) * count);
		ReadStringToInts(weightIndicesSource->FirstChild()->ToText()->Value(), weightData->weightIndices, count);
	}

	// JOINT HIERARCHY
	{
		skeleton->jointParents = (u8 *)FrameAlloc(skeleton->jointCount);
		memset(skeleton->jointParents, 0xFFFF, skeleton->jointCount);

		skeleton->restPoses = (mat4 *)FrameAlloc(sizeof(mat4) * skeleton->jointCount);

		XMLElement *sceneEl = rootEl->FirstChildElement("library_visual_scenes")
			->FirstChildElement("visual_scene");
		//Log("Found visual scene %s\n", sceneEl->FindAttribute("id")->Value());
		XMLElement *nodeEl = sceneEl->FirstChildElement("node");

		DynamicArray_XMLElementPtr stack;
		DynamicArrayInit_XMLElementPtr(&stack, 16, FrameAlloc);

		bool checkChildren = true;
		while (nodeEl != nullptr)
		{
			const char *nodeType = nodeEl->FindAttribute("type")->Value();
			if (checkChildren && strcmp(nodeType, "JOINT") == 0)
			{
				XMLElement *child = nodeEl;
				const char *childName = child->FindAttribute("sid")->Value();

				u8 jointIdx = 0xFF;
				for (int i = 0; i < skeleton->jointCount; ++i)
				{
					if (strcmp(skeleton->ids[i], childName) == 0)
						jointIdx = (u8)i;
				}
				ASSERT(jointIdx != 0xFF);

				if (stack.size)
				{
					XMLElement *parent = stack[stack.size - 1];
					const char *parentType = parent->FindAttribute("type")->Value();
					if (strcmp(parentType, "JOINT") == 0)
					{
						// Save parent ID
						const char *parentName = parent->FindAttribute("sid")->Value();

						u8 parentJointIdx = 0xFF;
						for (int i = 0; i < skeleton->jointCount; ++i)
						{
							if (strcmp(skeleton->ids[i], parentName) == 0)
								parentJointIdx = (u8)i;
							else if (strcmp(skeleton->ids[i], childName) == 0)
								jointIdx = (u8)i;
						}
						ASSERT(parentJointIdx != 0xFF);

						skeleton->jointParents[jointIdx] = parentJointIdx;

						//Log("Joint %s has parent %s\n", childName, parentName);
					}
				}

				XMLElement *matrixEl = nodeEl->FirstChildElement("matrix");
				const char *matrixStr = matrixEl->FirstChild()->ToText()->Value();
				mat4 restPose;
				ReadStringToFloats(matrixStr, (f32 *)&restPose, 16);
				skeleton->restPoses[jointIdx] = Mat4Transpose(restPose);
			}

			if (checkChildren)
			{
				XMLElement *childEl = nodeEl->FirstChildElement("node");
				if (childEl)
				{
					const char *type = nodeEl->FindAttribute("type")->Value();
					if (strcmp(type, "JOINT") == 0)
					{
						// Push joint to stack!
						stack[DynamicArrayAdd_XMLElementPtr(&stack, FrameRealloc)] = nodeEl;
					}

					nodeEl = childEl;
					continue;
				}
			}

			XMLElement *siblingEl = nodeEl->NextSiblingElement("node");
			if (siblingEl)
			{
				nodeEl = siblingEl;
				checkChildren = true;
				continue;
			}

			// Pop from stack!
			if (stack.size == 0)
				break;

			nodeEl = stack[--stack.size];
			checkChildren = false;
		}
	}

	return ERROR_OK;
}

ErrorCode ReadColladaAnimation(XMLElement *rootEl, Animation &animation, Skeleton &skeleton)
{
	DynamicArray_AnimationChannel channels;
	DynamicArrayInit_AnimationChannel(&channels, skeleton.jointCount, FrameAlloc);

	XMLElement *animationEl = rootEl->FirstChildElement("library_animations")
		->FirstChildElement("animation");
	XMLElement *subAnimEl = animationEl->FirstChildElement("animation");
	while (subAnimEl != nullptr)
	{
		//Log("Found animation %s\n", subAnimEl->FindAttribute("id")->Value());

		XMLElement *channelEl = subAnimEl->FirstChildElement("channel");
		const char *samplerSourceName = channelEl->FindAttribute("source")->Value() + 1;

		XMLElement *samplerSource = 0;
		// Find all samplers
		{
			XMLElement *samplerEl = subAnimEl->FirstChildElement("sampler");
			while (samplerEl != nullptr)
			{
				const char *id = samplerEl->FindAttribute("id")->Value();
				if (strcmp(id, samplerSourceName) == 0)
				{
					samplerSource = samplerEl;
				}
				samplerEl = samplerEl->NextSiblingElement("sampler");
			}
		}

		const char *timestampsSourceName = 0;
		const char *matricesSourceName = 0;
		{
			XMLElement *inputEl = samplerSource->FirstChildElement("input");
			while (inputEl != nullptr)
			{
				const char *semantic = inputEl->FindAttribute("semantic")->Value();
				const char *source = inputEl->FindAttribute("source")->Value();
				if (strcmp(semantic, "INPUT") == 0)
				{
					timestampsSourceName = source + 1;
				}
				else if (strcmp(semantic, "OUTPUT") == 0)
				{
					matricesSourceName = source + 1;
				}
				inputEl = inputEl->NextSiblingElement("input");
			}
		}

		XMLElement *timestampsSource = 0;
		XMLElement *matricesSource = 0;
		// Find all sources
		{
			XMLElement *sourceEl = subAnimEl->FirstChildElement("source");
			while (sourceEl != nullptr)
			{
				const char *id = sourceEl->FindAttribute("id")->Value();
				if (strcmp(id, timestampsSourceName) == 0)
				{
					timestampsSource = sourceEl;
				}
				else if (strcmp(id, matricesSourceName) == 0)
				{
					matricesSource = sourceEl;
				}
				sourceEl = sourceEl->NextSiblingElement("source");
			}
		}

		// Read timestamps
		if (animation.timestamps == nullptr)
		{
			// We assume an equal amount of samples per channel, so we only save timestamps
			// once per animation
			XMLElement *arrayEl = timestampsSource->FirstChildElement("float_array");
			animation.frameCount = arrayEl->FindAttribute("count")->IntValue();
			animation.timestamps = (f32 *)FrameAlloc(sizeof(f32) * animation.frameCount);
			ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), animation.timestamps,
					animation.frameCount);
		}

		AnimationChannel channel = {};

		// Read matrices
		{
			XMLElement *arrayEl = matricesSource->FirstChildElement("float_array");
			int count = arrayEl->FindAttribute("count")->IntValue();
			ASSERT(animation.frameCount == (u32)(count / 16));
			channel.transforms = (mat4 *)FrameAlloc(sizeof(f32) * count);
			ReadStringToFloats(arrayEl->FirstChild()->ToText()->Value(), (f32 *)channel.transforms,
					count);

			for (int i = 0; i < count / 16; ++i)
			{
				channel.transforms[i] = Mat4Transpose(channel.transforms[i]);
			}
		}

		// Get joint id
		{
			// TODO use scene to properly get the target joint
			const char *targetName = channelEl->FindAttribute("target")->Value();
			char jointIdBuffer[64];
			for (const char *scan = targetName; ; ++scan)
			{
				if (*scan == '_')
				{
					strcpy(jointIdBuffer, scan + 1);
					break;
				}
			}
			for (char *scan = jointIdBuffer; ; ++scan)
			{
				if (*scan == '/')
				{
					*scan = 0;
					break;
				}
			}
			channel.jointIndex = 0xFF;
			for (int i = 0; i < skeleton.jointCount; ++i)
			{
				if (strcmp(skeleton.ids[i], jointIdBuffer) == 0)
				{
					ASSERT(i < 0xFF);
					channel.jointIndex = (u8)i;
					break;
				}
			}
			ASSERT(channel.jointIndex != 0xFF);
			//Log("Channel thought to target joint \"%s\", ID %d\n", jointIdBuffer,
					//channel.jointIndex);
		}

		// Move start to 0
		f32 oldStart = animation.timestamps[0];
		for (u32 i = 0; i < animation.frameCount; ++i)
		{
			animation.timestamps[i] -= oldStart;
		}

		int newChannelIdx = DynamicArrayAdd_AnimationChannel(&channels, FrameRealloc);
		channels[newChannelIdx] = channel;

		subAnimEl = subAnimEl->NextSiblingElement("animation");
	}
	animation.channels = channels.data;
	animation.channelCount = channels.size;

	return ERROR_OK;
}

ErrorCode ReadColladaFile(tinyxml2::XMLDocument *dataDoc, const char *dataFileName,
		XMLElement **dataRootEl, const char *fullDataDir)
{
	char fullname[MAX_PATH];
	sprintf(fullname, "%s%s", fullDataDir, dataFileName);

	if (!Win32FileExists(fullname))
	{
		Log("ERROR! Geometry file doesn't exists: %s\n", fullname);
		return ERROR_META_MISSING_DEPENDENCY;
	}

	XMLError xmlError = dataDoc->LoadFile(fullname);
	if (xmlError != XML_SUCCESS)
	{
		Log("ERROR! Parsing XML file \"%s\" (%s)\n", fullname, dataDoc->ErrorStr());
		return (ErrorCode)xmlError;
	}

	*dataRootEl = dataDoc->FirstChildElement("COLLADA");
	if ((*dataRootEl) == nullptr)
	{
		Log("ERROR! Collada root node not found\n");
		return ERROR_COLLADA_NOROOT;
	}

	return ERROR_OK;
}

ErrorCode ProcessMetaFileCollada(MetaType type, XMLElement *rootEl, const char *filename,
		const char *fullDataDir)
{
	ErrorCode error;
	char outputName[MAX_PATH];
	GetOutputFilename(filename, outputName);

	tinyxml2::XMLDocument geometryDoc;
	XMLElement *geometryRootEl;
	RawGeometry rawGeometry;
	{
		XMLElement *geometryEl = rootEl->FirstChildElement("geometry");
		const char *geomFile = geometryEl->FirstChild()->ToText()->Value();

		const char *geomName = 0;
		const XMLAttribute *nameAttr = geometryEl->FindAttribute("id");
		if (nameAttr)
		{
			geomName = nameAttr->Value();
		}

		error = ReadColladaFile(&geometryDoc, geomFile, &geometryRootEl, fullDataDir);
		if (error != ERROR_OK)
			return error;

		error = ReadColladaGeometry(geometryRootEl, geomName, &rawGeometry);
		if (error != ERROR_OK)
			return error;
	}

	switch(type)
	{
	case METATYPE_MESH:
	{
		DynamicArray_RawVertex finalVertices = {};
		Array_u16 finalIndices = {};
		error = ConstructSkinnedMesh(&rawGeometry, nullptr, finalVertices, finalIndices);
		if (error != ERROR_OK)
			return error;

		// Output
		error = OutputMesh(outputName, finalVertices, finalIndices);
		if (error != ERROR_OK)
			return error;
	} break;
	case METATYPE_SKINNED_MESH:
	{
		Skeleton skeleton = {};
		WeightData weightData;
		// @Todo: just specify what file to get the controller from, should be no problem to use one
		// from a different file.
		error = ReadColladaController(geometryRootEl, &skeleton, &weightData);
		if (error)
			return error;

		DynamicArray_Animation animations = {};
		{
			DynamicArrayInit_Animation(&animations, 16, FrameAlloc);

			XMLElement *animationEl = rootEl->FirstChildElement("animation");
			while (animationEl != nullptr)
			{
				const char *animFile = animationEl->FirstChild()->ToText()->Value();
				Log("Found animation file: %s\n", animFile);

				bool loop = false;
				const XMLAttribute *loopAttr = animationEl->FindAttribute("loop");
				if (loopAttr)
					loop = loopAttr->IntValue() != 0;

				tinyxml2::XMLDocument dataDoc;
				XMLElement *dataRootEl;
				error = ReadColladaFile(&dataDoc, animFile, &dataRootEl, fullDataDir);
				if (error != ERROR_OK)
					return error;

				Animation &animation = animations[DynamicArrayAdd_Animation(&animations, FrameRealloc)];
				animation = {};
				animation.loop = loop;
				error = ReadColladaAnimation(dataRootEl, animation, skeleton);
				if (error)
					return error;

				animationEl = animationEl->NextSiblingElement("animation");
			}
		}

		DynamicArray_RawVertex finalVertices = {};
		Array_u16 finalIndices = {};
		error = ConstructSkinnedMesh(&rawGeometry, &weightData, finalVertices, finalIndices);
		if (error != ERROR_OK)
			return error;

		// Output
		error = OutputSkinnedMesh(outputName, finalVertices, finalIndices, skeleton, animations);
		if (error != ERROR_OK)
			return error;
	} break;
	case METATYPE_TRIANGLE_DATA:
	{
		// Make triangles
		Array_Triangle triangles;
		ArrayInit_Triangle(&triangles, rawGeometry.triangleCount, FrameAlloc);

		int attrCount = 1;
		if (rawGeometry.normalsOffset >= 0)
			++attrCount;
		if (rawGeometry.uvsOffset >= 0)
			++attrCount;

		for (int i = 0; i < rawGeometry.triangleCount; ++i)
		{
			Triangle *triangle = &triangles[triangles.size++];

			int ai = rawGeometry.triangleIndices[(i * 3 + 0) * attrCount + rawGeometry.verticesOffset];
			int bi = rawGeometry.triangleIndices[(i * 3 + 2) * attrCount + rawGeometry.verticesOffset];
			int ci = rawGeometry.triangleIndices[(i * 3 + 1) * attrCount + rawGeometry.verticesOffset];

			triangle->a = rawGeometry.positions[ai];
			triangle->b = rawGeometry.positions[bi];
			triangle->c = rawGeometry.positions[ci];

			triangle->normal = V3Normalize(V3Cross(triangle->c - triangle->a, triangle->b - triangle->a));
		}

		GeometryGrid geometryGrid;
		GenerateGeometryGrid(triangles, &geometryGrid);

		// Output
		HANDLE file = OpenForWrite(outputName);

		BakeryTriangleDataHeader header;
		u64 offsetsBlobOffset = FileSeek(file, sizeof(header), FILE_BEGIN);

		u32 offsetCount = geometryGrid.cellsSide * geometryGrid.cellsSide + 1;
		WriteToFile(file, geometryGrid.offsets, sizeof(geometryGrid.offsets[0]) * offsetCount);

		u64 trianglesBlobOffset = FilePosition(file);
		u32 triangleCount = geometryGrid.offsets[offsetCount - 1];
		WriteToFile(file, geometryGrid.triangles, sizeof(Triangle) * triangleCount);

		FileSeek(file, 0, FILE_BEGIN);
		header.lowCorner = geometryGrid.lowCorner;
		header.highCorner = geometryGrid.highCorner;
		header.cellsSide = geometryGrid.cellsSide;
		header.offsetsBlobOffset = offsetsBlobOffset;
		header.trianglesBlobOffset = trianglesBlobOffset;
		WriteToFile(file, &header, sizeof(header));

		CloseHandle(file);
	} break;
	case METATYPE_POINTS:
	{
		// Output
		HANDLE file = OpenForWrite(outputName);

		const u32 pointCount = rawGeometry.positions.size;

		BakeryPointsHeader header;
		header.pointCount = pointCount;
		header.pointsBlobOffset = sizeof(header);

		WriteToFile(file, &header, sizeof(header));
		WriteToFile(file, rawGeometry.positions.data, sizeof(v3) * pointCount);

		CloseHandle(file);
	} break;
	}

	return ERROR_OK;
}
