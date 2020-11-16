void ResourceLoadMesh(Resource *resource, const u8 *fileBuffer, bool initialize)
{
	Vertex *vertexData;
	u16 *indexData;
	u32 vertexCount;
	u32 indexCount;
	ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount);

	if (initialize)
	{
		int attribs = RENDERATTRIB_POSITION | RENDERATTRIB_UV | RENDERATTRIB_NORMAL;
		resource->mesh.deviceMesh = CreateDeviceIndexedMesh(attribs);
	}

	SendIndexedMesh(&resource->mesh.deviceMesh, vertexData, vertexCount, sizeof(Vertex),
			indexData, indexCount, false);
}

void ResourceLoadSkinnedMesh(Resource *resource, const u8 *fileBuffer, bool initialize)
{
	ResourceSkinnedMesh *skinnedMesh = &resource->skinnedMesh;

	SkinnedVertex *vertexData;
	u16 *indexData;
	u32 vertexCount;
	u32 indexCount;
	ReadSkinnedMesh(fileBuffer, skinnedMesh, &vertexData, &indexData, &vertexCount, &indexCount);

	if (initialize)
	{
		int attribs = RENDERATTRIB_POSITION | RENDERATTRIB_UV | RENDERATTRIB_NORMAL |
			RENDERATTRIB_INDICES | RENDERATTRIB_WEIGHTS;
		skinnedMesh->deviceMesh = CreateDeviceIndexedMesh(attribs);
	}

	// @Broken: This allocs things on transient memory and never frees them
	SendIndexedMesh(&skinnedMesh->deviceMesh, vertexData, vertexCount, sizeof(SkinnedVertex),
			indexData, indexCount, false);
}

void ResourceLoadLevelGeometryGrid(Resource *resource, const u8 *fileBuffer, bool initialize)
{
	(void) initialize;
	ReadTriangleGeometry(fileBuffer, &resource->geometryGrid);
}

void ResourceLoadCollisionMesh(Resource *resource, const u8 *fileBuffer, bool initialize)
{
	(void) initialize;
	ReadCollisionMesh(fileBuffer, &resource->collisionMesh);
}

void ResourceLoadShader(Resource *resource, const u8 *fileBuffer, bool initialize)
{
	ResourceShader *shader = &resource->shader;

	const char *vertexSource, *fragmentSource;
	ReadBakeryShader(fileBuffer, &vertexSource, &fragmentSource);

	if (initialize)
		shader->programHandle = CreateDeviceProgram();
	else
		WipeDeviceProgram(shader->programHandle);

	DeviceShader vertexShaderHandle = CreateShader(SHADERTYPE_VERTEX);
	LoadShader(&vertexShaderHandle, vertexSource);
	AttachShader(shader->programHandle, vertexShaderHandle);

	DeviceShader fragmentShaderHandle = CreateShader(SHADERTYPE_FRAGMENT);
	LoadShader(&fragmentShaderHandle, fragmentSource);
	AttachShader(shader->programHandle, fragmentShaderHandle);

	LinkDeviceProgram(shader->programHandle);
}

void ResourceLoadTexture(Resource *resource, const u8 *fileBuffer, bool initialize)
{
	const u8 *imageData;
	ReadImage(fileBuffer, &imageData, &resource->texture.width, &resource->texture.height,
			&resource->texture.components);

	if (initialize)
	{
		resource->texture.deviceTexture = CreateDeviceTexture();
	}

	SendTexture(resource->texture.deviceTexture, imageData, resource->texture.width,
			resource->texture.height, resource->texture.components);
}
