#include "webgpu.h"

#include <string.h>

WGPUDevice device;
WGPUQueue queue;
WGPUSwapChain swapchain;

WGPURenderPipeline pipeline;
WGPUBuffer vertBuf; // vertex buffer with triangle position and colours
WGPUBuffer indxBuf; // index buffer
WGPUBuffer textBuf; // texture data buffer
WGPUBuffer uRotBuf; // uniform buffer (containing the rotation angle)
WGPUBindGroup bindGroup;

/**
 * Current rotation angle (in degrees, updated per frame).
 */
float rotDeg = 0.0f;

/**
 * Vertex shader SPIR-V.
 * \code
 *	// glslc -Os -mfmt=num -o - -c in.vert
 *	#version 450
 *	layout(set = 0, binding = 0) uniform Rotation {
 *		float uRot;
 *	};
 *	layout(location = 0) in  vec2 aPos;
 *	layout(location = 1) in  vec3 aCol;
 *	layout(location = 2) in  vec2 aUV0;
 *	layout(location = 0) out vec3 vCol;
 *	layout(location = 1) out vec2 vUV0;
 *	void main() {
 *		float cosA = cos(radians(uRot));
 *		float sinA = sin(radians(uRot));
 *		mat3 rot = mat3(cosA, sinA, 0.0,
 *					   -sinA, cosA, 0.0,
 *						0.0,  0.0,  1.0);
 *		gl_Position = vec4(rot * vec3(aPos, 1.0), 1.0);
 *		vCol = aCol;
 *		vUV0 = aUV0;
 *	}
 * \endcode
 */
static uint32_t const triangle_vert[] = {
	0x07230203, 0x00010000, 0x000d0007, 0x00000047, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
	0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x000b000f, 0x00000000, 0x00000004, 0x6e69616d, 0x00000000, 0x0000002d, 0x00000031, 0x0000003e,
	0x00000040, 0x00000043, 0x00000044, 0x00050048, 0x00000009, 0x00000000, 0x00000023, 0x00000000,
	0x00030047, 0x00000009, 0x00000002, 0x00040047, 0x0000000b, 0x00000022, 0x00000000, 0x00040047,
	0x0000000b, 0x00000021, 0x00000000, 0x00050048, 0x0000002b, 0x00000000, 0x0000000b, 0x00000000,
	0x00050048, 0x0000002b, 0x00000001, 0x0000000b, 0x00000001, 0x00050048, 0x0000002b, 0x00000002,
	0x0000000b, 0x00000003, 0x00050048, 0x0000002b, 0x00000003, 0x0000000b, 0x00000004, 0x00030047,
	0x0000002b, 0x00000002, 0x00040047, 0x00000031, 0x0000001e, 0x00000000, 0x00040047, 0x0000003e,
	0x0000001e, 0x00000000, 0x00040047, 0x00000040, 0x0000001e, 0x00000001, 0x00040047, 0x00000043,
	0x0000001e, 0x00000001, 0x00040047, 0x00000044, 0x0000001e, 0x00000002, 0x00020013, 0x00000002,
	0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x0003001e, 0x00000009,
	0x00000006, 0x00040020, 0x0000000a, 0x00000002, 0x00000009, 0x0004003b, 0x0000000a, 0x0000000b,
	0x00000002, 0x00040015, 0x0000000c, 0x00000020, 0x00000001, 0x0004002b, 0x0000000c, 0x0000000d,
	0x00000000, 0x00040020, 0x0000000e, 0x00000002, 0x00000006, 0x00040017, 0x00000018, 0x00000006,
	0x00000003, 0x00040018, 0x00000019, 0x00000018, 0x00000003, 0x0004002b, 0x00000006, 0x0000001e,
	0x00000000, 0x0004002b, 0x00000006, 0x00000022, 0x3f800000, 0x00040017, 0x00000027, 0x00000006,
	0x00000004, 0x00040015, 0x00000028, 0x00000020, 0x00000000, 0x0004002b, 0x00000028, 0x00000029,
	0x00000001, 0x0004001c, 0x0000002a, 0x00000006, 0x00000029, 0x0006001e, 0x0000002b, 0x00000027,
	0x00000006, 0x0000002a, 0x0000002a, 0x00040020, 0x0000002c, 0x00000003, 0x0000002b, 0x0004003b,
	0x0000002c, 0x0000002d, 0x00000003, 0x00040017, 0x0000002f, 0x00000006, 0x00000002, 0x00040020,
	0x00000030, 0x00000001, 0x0000002f, 0x0004003b, 0x00000030, 0x00000031, 0x00000001, 0x00040020,
	0x0000003b, 0x00000003, 0x00000027, 0x00040020, 0x0000003d, 0x00000003, 0x00000018, 0x0004003b,
	0x0000003d, 0x0000003e, 0x00000003, 0x00040020, 0x0000003f, 0x00000001, 0x00000018, 0x0004003b,
	0x0000003f, 0x00000040, 0x00000001, 0x00040020, 0x00000042, 0x00000003, 0x0000002f, 0x0004003b,
	0x00000042, 0x00000043, 0x00000003, 0x0004003b, 0x00000030, 0x00000044, 0x00000001, 0x0006002c,
	0x00000018, 0x00000046, 0x0000001e, 0x0000001e, 0x00000022, 0x00050036, 0x00000002, 0x00000004,
	0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x00050041, 0x0000000e, 0x0000000f, 0x0000000b,
	0x0000000d, 0x0004003d, 0x00000006, 0x00000010, 0x0000000f, 0x0006000c, 0x00000006, 0x00000011,
	0x00000001, 0x0000000b, 0x00000010, 0x0006000c, 0x00000006, 0x00000012, 0x00000001, 0x0000000e,
	0x00000011, 0x0006000c, 0x00000006, 0x00000017, 0x00000001, 0x0000000d, 0x00000011, 0x0004007f,
	0x00000006, 0x00000020, 0x00000017, 0x00060050, 0x00000018, 0x00000023, 0x00000012, 0x00000017,
	0x0000001e, 0x00060050, 0x00000018, 0x00000024, 0x00000020, 0x00000012, 0x0000001e, 0x00060050,
	0x00000019, 0x00000026, 0x00000023, 0x00000024, 0x00000046, 0x0004003d, 0x0000002f, 0x00000032,
	0x00000031, 0x00050051, 0x00000006, 0x00000033, 0x00000032, 0x00000000, 0x00050051, 0x00000006,
	0x00000034, 0x00000032, 0x00000001, 0x00060050, 0x00000018, 0x00000035, 0x00000033, 0x00000034,
	0x00000022, 0x00050091, 0x00000018, 0x00000036, 0x00000026, 0x00000035, 0x00050051, 0x00000006,
	0x00000037, 0x00000036, 0x00000000, 0x00050051, 0x00000006, 0x00000038, 0x00000036, 0x00000001,
	0x00050051, 0x00000006, 0x00000039, 0x00000036, 0x00000002, 0x00070050, 0x00000027, 0x0000003a,
	0x00000037, 0x00000038, 0x00000039, 0x00000022, 0x00050041, 0x0000003b, 0x0000003c, 0x0000002d,
	0x0000000d, 0x0003003e, 0x0000003c, 0x0000003a, 0x0004003d, 0x00000018, 0x00000041, 0x00000040,
	0x0003003e, 0x0000003e, 0x00000041, 0x0004003d, 0x0000002f, 0x00000045, 0x00000044, 0x0003003e,
	0x00000043, 0x00000045, 0x000100fd, 0x00010038,
};

/**
 * Fragment shader SPIR-V.
 * \code
 *	// glslc -Os -mfmt=num -o - -c in.frag
 *	#version 450
 *	layout(set = 0, binding = 1) uniform texture2D uTex;
 *	layout(set = 0, binding = 2) uniform sampler   uSmp;
 *	layout(location = 0) in  vec3 vCol;
 *	layout(location = 1) in  vec2 vUV0;
 *	layout(location = 0) out vec4 fragColor;
 *	void main() {
 *		fragColor = vec4(vCol * texture(sampler2D(uTex, uSmp), vUV0).rgb, 1.0);
 *	}
 * \endcode
 */
static uint32_t const triangle_frag[] = {
	0x07230203, 0x00010000, 0x000d0007, 0x00000024, 0x00000000, 0x00020011, 0x00000001, 0x0006000b,
	0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001,
	0x0008000f, 0x00000004, 0x00000004, 0x6e69616d, 0x00000000, 0x00000009, 0x0000000c, 0x0000001a,
	0x00030010, 0x00000004, 0x00000007, 0x00040047, 0x00000009, 0x0000001e, 0x00000000, 0x00040047,
	0x0000000c, 0x0000001e, 0x00000000, 0x00040047, 0x00000010, 0x00000022, 0x00000000, 0x00040047,
	0x00000010, 0x00000021, 0x00000001, 0x00040047, 0x00000014, 0x00000022, 0x00000000, 0x00040047,
	0x00000014, 0x00000021, 0x00000002, 0x00040047, 0x0000001a, 0x0000001e, 0x00000001, 0x00020013,
	0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00030016, 0x00000006, 0x00000020, 0x00040017,
	0x00000007, 0x00000006, 0x00000004, 0x00040020, 0x00000008, 0x00000003, 0x00000007, 0x0004003b,
	0x00000008, 0x00000009, 0x00000003, 0x00040017, 0x0000000a, 0x00000006, 0x00000003, 0x00040020,
	0x0000000b, 0x00000001, 0x0000000a, 0x0004003b, 0x0000000b, 0x0000000c, 0x00000001, 0x00090019,
	0x0000000e, 0x00000006, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000,
	0x00040020, 0x0000000f, 0x00000000, 0x0000000e, 0x0004003b, 0x0000000f, 0x00000010, 0x00000000,
	0x0002001a, 0x00000012, 0x00040020, 0x00000013, 0x00000000, 0x00000012, 0x0004003b, 0x00000013,
	0x00000014, 0x00000000, 0x0003001b, 0x00000016, 0x0000000e, 0x00040017, 0x00000018, 0x00000006,
	0x00000002, 0x00040020, 0x00000019, 0x00000001, 0x00000018, 0x0004003b, 0x00000019, 0x0000001a,
	0x00000001, 0x0004002b, 0x00000006, 0x0000001f, 0x3f800000, 0x00050036, 0x00000002, 0x00000004,
	0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x0004003d, 0x0000000a, 0x0000000d, 0x0000000c,
	0x0004003d, 0x0000000e, 0x00000011, 0x00000010, 0x0004003d, 0x00000012, 0x00000015, 0x00000014,
	0x00050056, 0x00000016, 0x00000017, 0x00000011, 0x00000015, 0x0004003d, 0x00000018, 0x0000001b,
	0x0000001a, 0x00050057, 0x00000007, 0x0000001c, 0x00000017, 0x0000001b, 0x0008004f, 0x0000000a,
	0x0000001d, 0x0000001c, 0x0000001c, 0x00000000, 0x00000001, 0x00000002, 0x00050085, 0x0000000a,
	0x0000001e, 0x0000000d, 0x0000001d, 0x00050051, 0x00000006, 0x00000020, 0x0000001e, 0x00000000,
	0x00050051, 0x00000006, 0x00000021, 0x0000001e, 0x00000001, 0x00050051, 0x00000006, 0x00000022,
	0x0000001e, 0x00000002, 0x00070050, 0x00000007, 0x00000023, 0x00000020, 0x00000021, 0x00000022,
	0x0000001f, 0x0003003e, 0x00000009, 0x00000023, 0x000100fd, 0x00010038,
};

/**
 * Compressed DXT1 256x256 texture source.
 */
static uint8_t const textData[] = {
#include "testcard-dxt1.inl"
};

/**
 * Helper to create a shader from SPIR-V IR.
 *
 * \param[in] code shader source (output using the \c -V \c -x options in \c glslangValidator)
 * \param[in] size size of \a code in bytes
 * \param[in] label optional shader name
 */
static WGPUShaderModule createShader(const uint32_t* code, uint32_t size, const char* label = nullptr) {
	WGPUShaderModuleSPIRVDescriptor spirv = {};
	spirv.chain.sType = WGPUSType_ShaderModuleSPIRVDescriptor;
	spirv.codeSize = size / sizeof(uint32_t);
	spirv.code = code;
	WGPUShaderModuleDescriptor desc = {};
	desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&spirv);
	desc.label = label;
	return wgpuDeviceCreateShaderModule(device, &desc);
}

/**
 * Helper to create a buffer.
 *
 * \param[in] data pointer to the start of the raw data
 * \param[in] size number of bytes in \a data
 * \param[in] usage type of buffer
 */
static WGPUBuffer createBuffer(const void* data, size_t size, WGPUBufferUsage usage) {
	WGPUBufferDescriptor desc = {};
	desc.usage = WGPUBufferUsage_CopyDst | usage;
	desc.size  = size;
	WGPUBuffer buffer = wgpuDeviceCreateBuffer(device, &desc);
	wgpuBufferSetSubData(buffer, 0, size, data);
	return buffer;
}

/**
 * Bare minimum pipeline to draw a triangle using the above shaders.
 */
static void createPipelineAndBuffers() {
	// compile shaders
	
	WGPUShaderModule vertMod = createShader(triangle_vert, sizeof triangle_vert);
	WGPUShaderModule fragMod = createShader(triangle_frag, sizeof triangle_frag);

	// bind group layout (used by both the pipeline layout and uniform bind group, released at the end of this function)
	WGPUBindGroupLayoutEntry bglEntry[3] = {};
	bglEntry[0].binding = 0;
	bglEntry[0].visibility = WGPUShaderStage_Vertex;
	bglEntry[0].type = WGPUBindingType_UniformBuffer;
	bglEntry[1].binding = 1;
	bglEntry[1].visibility = WGPUShaderStage_Fragment;
	bglEntry[1].type = WGPUBindingType_SampledTexture;
	bglEntry[2].binding = 2;
	bglEntry[2].visibility = WGPUShaderStage_Fragment;
	bglEntry[2].type = WGPUBindingType_Sampler;

	WGPUBindGroupLayoutDescriptor bglDesc = {};
	bglDesc.entryCount = 3;
	bglDesc.entries = bglEntry;
	WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bglDesc);

	// pipeline layout (used by the render pipeline, released after its creation)
	WGPUPipelineLayoutDescriptor layoutDesc = {};
	layoutDesc.bindGroupLayoutCount = 1;
	layoutDesc.bindGroupLayouts = &bindGroupLayout;
	WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);
	
	// begin pipeline set-up
	WGPURenderPipelineDescriptor desc = {};

	desc.layout = pipelineLayout;

	desc.vertexStage.module = vertMod;
	desc.vertexStage.entryPoint = "main";

	WGPUProgrammableStageDescriptor fragStage = {};
	fragStage.module = fragMod;
	fragStage.entryPoint = "main";
	desc.fragmentStage = &fragStage;

	// describe buffer layouts (mirroring the shaders)
	WGPUVertexAttributeDescriptor vertAttrs[3] = {};
	vertAttrs[0].format = WGPUVertexFormat_Float2; // x, y
	vertAttrs[0].offset = 0;
	vertAttrs[0].shaderLocation = 0;
	vertAttrs[1].format = WGPUVertexFormat_Float3; // r, g, b
	vertAttrs[1].offset = 2 * sizeof(float);
	vertAttrs[1].shaderLocation = 1;
	vertAttrs[2].format = WGPUVertexFormat_Float2; // u, v
	vertAttrs[2].offset = 5 * sizeof(float);
	vertAttrs[2].shaderLocation = 2;
	WGPUVertexBufferLayoutDescriptor vertDesc = {};
	vertDesc.arrayStride = 7 * sizeof(float);
	vertDesc.attributeCount = 3;
	vertDesc.attributes = vertAttrs;
	WGPUVertexStateDescriptor vertState = {};
	vertState.indexFormat = WGPUIndexFormat_Uint16;
	vertState.vertexBufferCount = 1;
	vertState.vertexBuffers = &vertDesc;

	desc.vertexState = &vertState;
	desc.primitiveTopology = WGPUPrimitiveTopology_TriangleList;

	desc.sampleCount = 1;

	// describe blend
	WGPUBlendDescriptor blendDesc = {};
	blendDesc.operation = WGPUBlendOperation_Add;
	blendDesc.srcFactor = WGPUBlendFactor_SrcAlpha;
	blendDesc.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	WGPUColorStateDescriptor colorDesc = {};
	colorDesc.format = webgpu::getSwapChainFormat(device);
	colorDesc.alphaBlend = blendDesc;
	colorDesc.colorBlend = blendDesc;
	colorDesc.writeMask = WGPUColorWriteMask_All;

	desc.colorStateCount = 1;
	desc.colorStates = &colorDesc;

	desc.sampleMask = 0xFFFFFFFF; //<- Note: this currently causes Emscripten to fail (sampleMask ends up as -1, which trips an assert)

	pipeline = wgpuDeviceCreateRenderPipeline(device, &desc);

	// partial clean-up (just move to the end, no?)
	wgpuPipelineLayoutRelease(pipelineLayout);

	wgpuShaderModuleRelease(fragMod);
	wgpuShaderModuleRelease(vertMod);

	// create the buffers (x, y, r, g, b, u, v)
	float const vertData[] = {
		-0.8f, -0.8f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // BL
		 0.8f, -0.8f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // BR
		-0.8f,  0.8f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, // TL
		 0.8f,  0.8f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, // TR
	};
	uint16_t const indxData[] = {
		0, 1, 2,
		2, 1, 3,
	};
	vertBuf = createBuffer(vertData, sizeof(vertData), WGPUBufferUsage_Vertex);
	indxBuf = createBuffer(indxData, sizeof(indxData), WGPUBufferUsage_Index);

	// also creating the texture and uniform buffers (note these are copied here, not bound in any way)
	textBuf = createBuffer(textData, sizeof(textData), WGPUBufferUsage_CopySrc);
	uRotBuf = createBuffer(&rotDeg,  sizeof(rotDeg),   WGPUBufferUsage_Uniform);
	
	WGPUExtent3D textExt;
	textExt.width = 256;
	textExt.height = 256;
	textExt.depth = 1;

	WGPUTextureDescriptor textDesc = {};
	textDesc.usage = WGPUTextureUsage_CopyDst | WGPUTextureUsage_Sampled;
	textDesc.dimension = WGPUTextureDimension_2D;
	textDesc.size = textExt;
	textDesc.arrayLayerCount = 1;
	textDesc.format = WGPUTextureFormat_BC1RGBAUnorm;
	textDesc.mipLevelCount = 1;
	textDesc.sampleCount = 1;

	WGPUTexture text = wgpuDeviceCreateTexture(device, &textDesc);
	
	WGPUBufferCopyView srcView = {};
	srcView.buffer = textBuf;
	srcView.bytesPerRow = (textExt.width / 4) * 8;
	srcView.rowsPerImage = textExt.height;

	WGPUTextureCopyView dstView = {};
	dstView.texture = text;

	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);
	wgpuCommandEncoderCopyBufferToTexture(encoder, &srcView, &dstView, &textExt);
	WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);
	wgpuCommandEncoderRelease(encoder);
	wgpuQueueSubmit(queue, 1, &commands);
	wgpuCommandBufferRelease(commands);

	WGPUTextureView textView = wgpuTextureCreateView(text, NULLPTR);

	WGPUSamplerDescriptor sampDesc = {};
	sampDesc.magFilter = WGPUFilterMode_Linear;
	sampDesc.minFilter = WGPUFilterMode_Linear;
	WGPUSampler sampler =  wgpuDeviceCreateSampler(device, &sampDesc);

	// create the uniform bind group
	WGPUBindGroupEntry bgEntry[3] = {};
	bgEntry[0].binding = 0;
	bgEntry[0].buffer = uRotBuf;
	bgEntry[0].offset = 0;
	bgEntry[0].size = sizeof(rotDeg);
	bgEntry[1].binding = 1;
	bgEntry[1].textureView = textView;
	bgEntry[2].binding = 2;
	bgEntry[2].sampler = sampler;

	WGPUBindGroupDescriptor bgDesc = {};
	bgDesc.layout = bindGroupLayout;
	bgDesc.entryCount = 3;
	bgDesc.entries = bgEntry;

	bindGroup = wgpuDeviceCreateBindGroup(device, &bgDesc);

	// last bit of clean-up
	wgpuSamplerRelease(sampler);
	wgpuTextureViewRelease(textView);
	wgpuTextureRelease(text);
	wgpuBindGroupLayoutRelease(bindGroupLayout);
}

/**
 * Draws using the above pipeline and buffers.
 */
static bool redraw() {
	WGPUTextureView backBufView = wgpuSwapChainGetCurrentTextureView(swapchain);			// create textureView

	WGPURenderPassColorAttachmentDescriptor colorDesc = {};
	colorDesc.attachment = backBufView;
	colorDesc.loadOp  = WGPULoadOp_Clear;
	colorDesc.storeOp = WGPUStoreOp_Store;
	colorDesc.clearColor.r = 0.3f;
	colorDesc.clearColor.g = 0.3f;
	colorDesc.clearColor.b = 0.3f;
	colorDesc.clearColor.a = 1.0f;

	WGPURenderPassDescriptor renderPass = {};
	renderPass.colorAttachmentCount = 1;
	renderPass.colorAttachments = &colorDesc;

	WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, nullptr);			// create encoder
	WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPass);	// create pass

	// update the rotation
	rotDeg += 0.1f;
	wgpuBufferSetSubData(uRotBuf, 0, sizeof(rotDeg), &rotDeg);

	// draw the triangle (comment these five lines to simply clear the screen)
	wgpuRenderPassEncoderSetPipeline(pass, pipeline);
	wgpuRenderPassEncoderSetBindGroup(pass, 0, bindGroup, 0, 0);
	wgpuRenderPassEncoderSetVertexBuffer(pass, 0, vertBuf, 0, 0);
	wgpuRenderPassEncoderSetIndexBuffer(pass, indxBuf, 0, 0);
	wgpuRenderPassEncoderDrawIndexed(pass, 6, 1, 0, 0, 0);

	wgpuRenderPassEncoderEndPass(pass);
	wgpuRenderPassEncoderRelease(pass);														// release pass
	WGPUCommandBuffer commands = wgpuCommandEncoderFinish(encoder, nullptr);				// create commands
	wgpuCommandEncoderRelease(encoder);														// release encoder

	wgpuQueueSubmit(queue, 1, &commands);
	wgpuCommandBufferRelease(commands);														// release commands
#ifndef __EMSCRIPTEN__
	/*
	 * TODO: wgpuSwapChainPresent is unsupported in Emscripten, so what do we do?
	 */
	wgpuSwapChainPresent(swapchain);
#endif
	wgpuTextureViewRelease(backBufView);													// release textureView

	return true;
}

extern "C" int __main__(int /*argc*/, char* /*argv*/[]) {
	if (window::Handle wHnd = window::create()) {
		if ((device = webgpu::create(wHnd))) {
			queue = wgpuDeviceGetDefaultQueue(device);
			swapchain = webgpu::createSwapChain(device);
			createPipelineAndBuffers();

			window::show(wHnd);
			window::loop(wHnd, redraw);

#ifndef __EMSCRIPTEN__
			wgpuBindGroupRelease(bindGroup);
			wgpuBufferRelease(uRotBuf);
			wgpuBufferRelease(textBuf);
			wgpuBufferRelease(indxBuf);
			wgpuBufferRelease(vertBuf);
			wgpuRenderPipelineRelease(pipeline);
			wgpuSwapChainRelease(swapchain);
			wgpuQueueRelease(queue);
			wgpuDeviceRelease(device);
		#endif
		}
	#ifndef __EMSCRIPTEN__
		window::destroy(wHnd);
	#endif
	}
	return 0;
}
