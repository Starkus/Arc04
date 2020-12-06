void ResourceLoadMesh(Resource *resource, const u8 *fileBuffer, bool initialize)
{
	ResourceMesh *meshRes = &resource->mesh;

	Vertex *vertexData;
	u16 *indexData;
	u32 vertexCount;
	u32 indexCount;
	const char *materialFilename;
	ReadMesh(fileBuffer, &vertexData, &indexData, &vertexCount, &indexCount, &materialFilename);

	if (initialize)
	{
		int attribs = RENDERATTRIB_POSITION | RENDERATTRIB_UV | RENDERATTRIB_NORMAL;
		meshRes->deviceMesh = CreateDeviceIndexedMesh(attribs);
	}

	SendIndexedMesh(&meshRes->deviceMesh, vertexData, vertexCount, sizeof(Vertex),
			indexData, indexCount, false);

	if (strlen(materialFilename))
	{
		const Resource *materialRes = GetResource(materialFilename);
		if (!materialRes)
		{
			materialRes = LoadResource(RESOURCETYPE_MATERIAL, materialFilename);
			ASSERT(materialRes);
		}
		meshRes->materialRes = materialRes;
	}
	else
		meshRes->materialRes = nullptr;
}

void ResourceLoadSkinnedMesh(Resource *resource, const u8 *fileBuffer, bool initialize)
{
	ResourceSkinnedMesh *skinnedMesh = &resource->skinnedMesh;

	SkinnedVertex *vertexData;
	u16 *indexData;
	u32 vertexCount;
	u32 indexCount;
	const char *materialFilename;
	ReadSkinnedMesh(fileBuffer, skinnedMesh, &vertexData, &indexData, &vertexCount, &indexCount,
			&materialFilename);

	if (initialize)
	{
		int attribs = RENDERATTRIB_POSITION | RENDERATTRIB_UV | RENDERATTRIB_NORMAL |
			RENDERATTRIB_INDICES | RENDERATTRIB_WEIGHTS;
		skinnedMesh->deviceMesh = CreateDeviceIndexedMesh(attribs);
	}

	// @Broken: This allocs things on transient memory and never frees them
	SendIndexedMesh(&skinnedMesh->deviceMesh, vertexData, vertexCount, sizeof(SkinnedVertex),
			indexData, indexCount, false);

	if (strlen(materialFilename))
	{
		const Resource *materialRes = GetResource(materialFilename);
		if (!materialRes)
		{
			materialRes = LoadResource(RESOURCETYPE_MATERIAL, materialFilename);
			ASSERT(materialRes);
		}
		skinnedMesh->materialRes = materialRes;
	}
	else
		skinnedMesh->materialRes = nullptr;
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

	ASSERT(resource->texture.components >= RENDERIMAGECOMPONENTS_1 &&
			resource->texture.components <= RENDERIMAGECOMPONENTS_4);
	SendTexture(resource->texture.deviceTexture, imageData, resource->texture.width,
			resource->texture.height, (RenderImageComponents)resource->texture.components);
}

void ResourceLoadMaterial(Resource *resource, const u8 *fileBuffer, bool initialize)
{
	(void) initialize;
	ResourceMaterial *material = &resource->material;

	RawBakeryMaterial rawMaterial;
	ReadMaterial(fileBuffer, &rawMaterial);

	const Resource *shaderRes = GetResource(rawMaterial.shaderFilename);
	if (!shaderRes)
	{
		shaderRes = LoadResource(RESOURCETYPE_SHADER, rawMaterial.shaderFilename);
		ASSERT(shaderRes);
	}
	material->shaderRes = shaderRes;

	material->textureCount = rawMaterial.textureCount;
	ASSERT(material->textureCount <= ArrayCount(material->textures));
	for (u32 texIdx = 0; texIdx < rawMaterial.textureCount; ++texIdx)
	{
		const Resource *textureRes = GetResource(rawMaterial.textureFilenames[texIdx]);
		if (!textureRes)
		{
			textureRes = LoadResource(RESOURCETYPE_TEXTURE, rawMaterial.textureFilenames[texIdx]);
			ASSERT(textureRes);
		}
		material->textures[texIdx] = textureRes;
	}
}
