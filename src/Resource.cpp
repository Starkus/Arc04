RESOURCE_LOAD_MESH(ResourceLoadMesh)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_MESH;

	u8 *fileBuffer;
	u64 fileSize;
	bool success = PlatformReadEntireFile(filename, &fileBuffer, &fileSize, FrameAlloc);
	ASSERT(success);

	Vertex *vertexData;
	u16 *indexData;
	u32 vertexCount;
	u32 indexCount;
	ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

	int attribs = RENDERATTRIB_POSITION | RENDERATTRIB_UV | RENDERATTRIB_NORMAL;
	result->mesh.deviceMesh = CreateDeviceIndexedMesh(attribs);
	SendIndexedMesh(&result->mesh.deviceMesh, vertexData, vertexCount, sizeof(Vertex),
			indexData, indexCount, false);

	return result;
}

RESOURCE_LOAD_SKINNED_MESH(ResourceLoadSkinnedMesh)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_SKINNEDMESH;

	ResourceSkinnedMesh *skinnedMesh = &result->skinnedMesh;

	u8 *fileBuffer;
	u64 fileSize;
	bool success = PlatformReadEntireFile(filename, &fileBuffer, &fileSize, FrameAlloc);
	ASSERT(success);

	SkinnedVertex *vertexData;
	u16 *indexData;
	u32 vertexCount;
	u32 indexCount;
	ReadSkinnedMesh(fileBuffer, skinnedMesh, &vertexData, &indexData, &vertexCount, &indexCount);

	int attribs = RENDERATTRIB_POSITION | RENDERATTRIB_UV | RENDERATTRIB_NORMAL |
		RENDERATTRIB_INDICES | RENDERATTRIB_WEIGHTS;
	// @Broken: This allocs things on transient memory and never frees them
	skinnedMesh->deviceMesh = CreateDeviceIndexedMesh(attribs);
	SendIndexedMesh(&skinnedMesh->deviceMesh, vertexData, vertexCount, sizeof(SkinnedVertex),
			indexData, indexCount, false);

	return result;
}

RESOURCE_LOAD_LEVEL_GEOMETRY_GRID(ResourceLoadLevelGeometryGrid)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_LEVELGEOMETRYGRID;

	ResourceGeometryGrid *geometryGrid = &result->geometryGrid;

	u8 *fileBuffer;
	u64 fileSize;
	bool success = PlatformReadEntireFile(filename, &fileBuffer, &fileSize, FrameAlloc);
	ASSERT(success);

	ReadTriangleGeometry(fileBuffer, geometryGrid);

	return result;
}

RESOURCE_LOAD_POINTS(ResourceLoadPoints)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_POINTS;

	ResourcePointCloud *pointCloud = &result->points;

	u8 *fileBuffer;
	u64 fileSize;
	bool success = PlatformReadEntireFile(filename, &fileBuffer, &fileSize, FrameAlloc);
	ASSERT(success);

	ReadPoints(fileBuffer, pointCloud);

	return result;
}

RESOURCE_LOAD_SHADER(ResourceLoadShader)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_SHADER;

	ResourceShader *shader = &result->shader;

	u8 *fileBuffer;
	u64 fileSize;
	bool success = PlatformReadEntireFile(filename, &fileBuffer, &fileSize, FrameAlloc);
	ASSERT(success);

	const char *vertexSource, *fragmentSource;
	ReadBakeryShader(fileBuffer, &vertexSource, &fragmentSource);

	DeviceProgram programHandle = CreateDeviceProgram();

	DeviceShader vertexShaderHandle = CreateShader(SHADERTYPE_VERTEX);
	LoadShader(&vertexShaderHandle, vertexSource);
	AttachShader(programHandle, vertexShaderHandle);

	DeviceShader fragmentShaderHandle = CreateShader(SHADERTYPE_FRAGMENT);
	LoadShader(&fragmentShaderHandle, fragmentSource);
	AttachShader(programHandle, fragmentShaderHandle);

	LinkDeviceProgram(programHandle);

	shader->programHandle = programHandle;
	return result;
}

RESOURCE_LOAD_TEXTURE(ResourceLoadTexture)
{
	Resource *result = CreateResource(filename);
	result->type = RESOURCETYPE_TEXTURE;

	u8 *fileBuffer;
	u64 fileSize;
	bool success = PlatformReadEntireFile(filename, &fileBuffer, &fileSize, FrameAlloc);
	ASSERT(success);

	const u8 *imageData;
	ReadImage(fileBuffer, &imageData, &result->texture.width, &result->texture.height,
			&result->texture.components);

	result->texture.deviceTexture = CreateDeviceTexture();
	SendTexture(result->texture.deviceTexture, imageData, result->texture.width,
			result->texture.height, result->texture.components);

	return result;
}
