#ifdef _ORBIS
#include "PS4RendererAPI.h"
#include "PS4Window.h"
#include <video_out.h>	//Video System
#include "PS4Shader.h"
#include "PS4Mesh.h"
#include <gnmx\basegfxcontext.h>

#include <.\graphics\api_gnm\toolkit\allocators.h>
#include <.\graphics\api_gnm\toolkit\stack_allocator.h>

#include "../../Common/Matrix4.h"

using namespace sce::Vectormath::Scalar::Aos;

namespace SonyMath = sce::Vectormath::Scalar::Aos;

#include <iostream>

using namespace NCL;
using namespace NCL::PS4;
using namespace NCL::Maths;

sce::Gnmx::Toolkit::IAllocator	oAllocator;
sce::Gnmx::Toolkit::IAllocator	gAllocator;

PS4RendererAPI::PS4RendererAPI(Window& window)
	: RendererAPI(window),
	_MaxCMDBufferCount(3),
	  _bufferCount(3),
	  _GarlicMemory(1024 * 1024 * 512),
	  _OnionMemory( 1024 * 1024 * 256)
{
	framesSubmitted		 = 0;
	currentGPUBuffer	 = 0;
	currentScreenBuffer	 = 0;
	prevScreenBuffer	 = 0;

	std::cerr << "Starting Rich Code!" << std::endl;

	currentGFXContext	 = nullptr;

	InitialiseMemoryAllocators();

	InitialiseGCMRendering();
	InitialiseVideoSystem();
	window.SetRenderer(this);
	SwapScreenBuffer();
	SwapCommandBuffer();//always swap at least once...
}

PS4RendererAPI::~PS4RendererAPI()	{
	DestroyGCMRendering();
	DestroyVideoSystem();
	DestroyMemoryAllocators();
}

void	PS4RendererAPI::InitialiseVideoSystem() {
	screenBuffers = new PS4ScreenBuffer*[_bufferCount];

	for (int i = 0; i < _bufferCount; ++i) {
		screenBuffers[i] = GenerateScreenBuffer(1920, 1080);
	}

	//Now we can open up the video port
	videoHandle		= sceVideoOutOpen(0, SCE_VIDEO_OUT_BUS_TYPE_MAIN, 0, NULL);

	SceVideoOutBufferAttribute attribute;
	sceVideoOutSetBufferAttribute(&attribute,
		SCE_VIDEO_OUT_PIXEL_FORMAT_B8_G8_R8_A8_SRGB,
		SCE_VIDEO_OUT_TILING_MODE_TILE,
		SCE_VIDEO_OUT_ASPECT_RATIO_16_9,
		screenBuffers[0]->colourTarget.getWidth(),
		screenBuffers[0]->colourTarget.getHeight(),
		screenBuffers[0]->colourTarget.getPitch()
	);

	void* bufferAddresses[_bufferCount];

	for (int i = 0; i < _bufferCount; ++i) {
		bufferAddresses[i] = screenBuffers[i]->colourTarget.getBaseAddress();
	}

	sceVideoOutRegisterBuffers(videoHandle, 0, bufferAddresses, _bufferCount, &attribute);
}

void	PS4RendererAPI::InitialiseGCMRendering() {
	frames = (PS4Frame*)onionAllocator->allocate(sizeof(PS4Frame) * _MaxCMDBufferCount, alignof(PS4Frame));

	for (int i = 0; i < _MaxCMDBufferCount; ++i) {
		new (&frames[i])PS4Frame();
	}

	sce::Gnmx::Toolkit::Allocators allocators = sce::Gnmx::Toolkit::Allocators(oAllocator, gAllocator, ownerHandle);
	Gnmx::Toolkit::initializeWithAllocators(&allocators);
}

void	PS4RendererAPI::InitialiseMemoryAllocators() {
	stackAllocators[MEMORY_GARLIC].init(SCE_KERNEL_WC_GARLIC, _GarlicMemory);
	stackAllocators[MEMORY_ONION ].init(SCE_KERNEL_WB_ONION , _OnionMemory);

	oAllocator = Gnmx::Toolkit::GetInterface(&stackAllocators[MEMORY_ONION]);
	gAllocator = Gnmx::Toolkit::GetInterface(&stackAllocators[MEMORY_GARLIC]);

	this->garlicAllocator   = &gAllocator;
	this->onionAllocator	= &oAllocator;
	Gnm::registerOwner(&ownerHandle, "PS4RendererAPI");
}

void PS4RendererAPI::DestroyMemoryAllocators() {
	stackAllocators[MEMORY_GARLIC].deinit();
	stackAllocators[MEMORY_ONION ].deinit();
}

PS4ScreenBuffer*	PS4RendererAPI::GenerateScreenBuffer(uint width, uint height, bool colour, bool depth, bool stencil) {
	PS4ScreenBuffer* buffer = new PS4ScreenBuffer();

	if (colour) {	
		Gnm::RenderTargetSpec spec;
		spec.init();
		spec.m_width		= width;
		spec.m_height		= height;
		spec.m_numSamples	= Gnm::kNumSamples1;
		spec.m_numFragments = Gnm::kNumFragments1;
		spec.m_colorFormat	= Gnm::kDataFormatB8G8R8A8UnormSrgb;	

		GpuAddress::computeSurfaceTileMode(Gnm::GpuMode::kGpuModeBase, &spec.m_colorTileModeHint, GpuAddress::kSurfaceTypeColorTargetDisplayable, spec.m_colorFormat, 1);

		int32_t success = buffer->colourTarget.init(&spec);

		if (success != SCE_GNM_OK) {
			bool a = true;
		}

		const Gnm::SizeAlign colourAlign = buffer->colourTarget.getColorSizeAlign();

		void *colourMemory = stackAllocators[MEMORY_GARLIC].allocate(colourAlign);

		Gnm::registerResource(nullptr, ownerHandle, colourMemory, colourAlign.m_size,
			"Colour", Gnm::kResourceTypeDepthRenderTargetBaseAddress, 0);

		buffer->colourTarget.setAddresses(colourMemory, NULL, NULL);
	}

	if (depth) {
		Gnm::DepthRenderTargetSpec spec;
		spec.init();
		spec.m_width			= width;
		spec.m_height			= height;
		spec.m_numFragments		= Gnm::kNumFragments1;
		spec.m_zFormat			= Gnm::ZFormat::kZFormat32Float;
		spec.m_stencilFormat	= (stencil ? Gnm::kStencil8 : Gnm::kStencilInvalid);

		GpuAddress::computeSurfaceTileMode(Gnm::GpuMode::kGpuModeBase, &spec.m_tileModeHint, GpuAddress::kSurfaceTypeDepthTarget, Gnm::DataFormat::build(spec.m_zFormat), 1);

		void* stencilMemory = 0;

		int32_t success = buffer->depthTarget.init(&spec);

		if (success != SCE_GNM_OK) {
			bool a = true;
		}

		void *depthMemory = stackAllocators[MEMORY_GARLIC].allocate(buffer->depthTarget.getZSizeAlign());

		Gnm::registerResource(nullptr, ownerHandle, depthMemory, buffer->depthTarget.getZSizeAlign().m_size,
			"Depth", Gnm::kResourceTypeDepthRenderTargetBaseAddress, 0);

		if (stencil) {
			stencilMemory = stackAllocators[MEMORY_GARLIC].allocate(buffer->depthTarget.getStencilSizeAlign());

			Gnm::registerResource(nullptr, ownerHandle, stencilMemory, buffer->depthTarget.getStencilSizeAlign().m_size,
				"Stencil", Gnm::kResourceTypeDepthRenderTargetBaseAddress, 0);
		}

		buffer->depthTarget.setAddresses(depthMemory, stencilMemory);
	}

	return buffer;
}

void	PS4RendererAPI::DestroyGCMRendering() {
	//onionAllocator->release(frames);
}

void	PS4RendererAPI::DestroyVideoSystem() {
	for (int i = 0; i < _bufferCount; ++i) {
		delete screenBuffers[i];
	}
	delete[] screenBuffers;

	sceVideoOutClose(videoHandle);
}

void	PS4RendererAPI::OnWindowResize(int w, int h)  {
	currentWidth	= w;
	currentHeight	= h;
}

void	PS4RendererAPI::BeginFrame()   {

}

void PS4RendererAPI::EndFrame()			{
	framesSubmitted++;
}

void	PS4RendererAPI::SwapBuffers() {
	SwapScreenBuffer();
	SwapCommandBuffer();
}

void	PS4RendererAPI::SwapScreenBuffer() {
	prevScreenBuffer	= currentScreenBuffer;
	currentScreenBuffer = (currentScreenBuffer + 1) % _bufferCount;
	sceVideoOutSubmitFlip(videoHandle, prevScreenBuffer, SCE_VIDEO_OUT_FLIP_MODE_VSYNC, 0);

	currentPS4Buffer = screenBuffers[currentScreenBuffer];
}

void	PS4RendererAPI::SwapCommandBuffer() {
	if (currentGFXContext) {	
		if (currentGFXContext->submit() != sce::Gnm::kSubmissionSuccess) {
			std::cerr << "Graphics queue submission failed?" << std::endl;
		}
		Gnm::submitDone();
	}

	currentGPUBuffer	= (currentGPUBuffer + 1) % _MaxCMDBufferCount;
	 
	currentFrame		= &frames[currentGPUBuffer]; 
	currentGFXContext	= &currentFrame->GetCommandBuffer();
}
 
void	PS4RendererAPI::SetRenderBuffer(PS4ScreenBuffer*buffer, bool clearColour, bool clearDepth, bool clearStencil) {
	currentPS4Buffer = buffer;
	currentGFXContext->setRenderTargetMask(0xF);
	currentGFXContext->setRenderTarget(0, &currentPS4Buffer->colourTarget);
	currentGFXContext->setDepthRenderTarget(&currentPS4Buffer->depthTarget);

	currentGFXContext->setupScreenViewport(0, 0, currentPS4Buffer->colourTarget.getWidth(), currentPS4Buffer->colourTarget.getHeight(), 0.5f, 0.5f);
	currentGFXContext->setScreenScissor(0, 0, currentPS4Buffer->colourTarget.getWidth(), currentPS4Buffer->colourTarget.getHeight());
	currentGFXContext->setWindowScissor(0, 0, currentPS4Buffer->colourTarget.getWidth(), currentPS4Buffer->colourTarget.getHeight(), sce::Gnm::WindowOffsetMode::kWindowOffsetDisable);
	currentGFXContext->setGenericScissor(0, 0, currentPS4Buffer->colourTarget.getWidth(), currentPS4Buffer->colourTarget.getHeight(), sce::Gnm::WindowOffsetMode::kWindowOffsetDisable);

	ClearBuffer(clearColour, clearDepth, clearStencil);
}

void	PS4RendererAPI::ClearBuffer(bool colour, bool depth, bool stencil) {
	if (colour) {
		//Vector4 defaultClearColour(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, 1.0f);
		SonyMath::Vector4 defaultClearColour(0.1f, 0.1f, 0.1f, 1.0f);
		SurfaceUtil::clearRenderTarget(*currentGFXContext, &currentPS4Buffer->colourTarget, defaultClearColour);
	}

	if (depth) {
		float defaultDepth = 1.0f;
		SurfaceUtil::clearDepthTarget(*currentGFXContext, &currentPS4Buffer->depthTarget, defaultDepth);
	}

	if (stencil && currentPS4Buffer->depthTarget.getStencilReadAddress()) {
		int defaultStencil = 0;
		SurfaceUtil::clearStencilTarget(*currentGFXContext, &currentPS4Buffer->depthTarget, defaultStencil);
	}
}

void PS4RendererAPI::DrawMesh(MeshGeometry* mesh) {
	//BindMesh(mesh);
	//DrawBoundMesh();
}

void PS4RendererAPI::BindShader(ShaderBase* shader) {

}

void PS4RendererAPI::BindTexture(const TextureBase* tex, std::string uniform, int texSlot) {
	//BindTextureToShader(tex, uniform, 0);
}

void PS4RendererAPI::UpdateUniformFloat(ShaderBase* shader, std::string uniform, float f) {
	//PS4Shader* ps4Shader = dynamic_cast<PS4Shader*>(shader);
	//if (!ps4Shader) {
	//	return;
	//}

	//int fLoc = glGetUniformLocation(oglShader->GetProgramID(), uniform.c_str());
	//glUniform1i(fLoc, f);
}

void PS4RendererAPI::UpdateUniformMatrix4(ShaderBase* shader, std::string uniform, Maths::Matrix4 matrix) {
	//PS4Shader* ps4Shader = dynamic_cast<PS4Shader*>(shader);
	//if (!ps4Shader) {
	//	return;
	//}

	Matrix4* modelData = (Matrix4*)currentGFXContext->allocateFromCommandBuffer(sizeof(Matrix4), Gnm::kEmbeddedDataAlignment4);
	*modelData = matrix;

	Gnm::Buffer constantBuffer;
	constantBuffer.initAsConstantBuffer(modelData, sizeof(Matrix4));
	constantBuffer.setResourceMemoryType(Gnm::kResourceMemoryTypeRO);

	UpdateAllUniform(shader, uniform, buffer);

	//int matLoc = glGetUniformLocation(oglShader->GetProgramID(), uniform.c_str());
	//glUniformMatrix4fv(matLoc, 1, false, (float*)&matrix);
}

void PS4RendererAPI::SetDepth(bool d) {
	//d ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
}

void PS4RendererAPI::SetBlend(bool b) {
	//b ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void PS4RendererAPI::UpdateAllUniform(PS4Shader* shader, std::string uniform, Gnm::buffer buffer) {
	int vsIndex = realShader->GetConstantBufferIndex(Gnm::kShaderStageVs, uniform.c_str());
	if (vsIndex != -1) {
		currentGFXContext->setConstantBuffers(Gnm::kShaderStageVs, objIndex, 1, &buffer);
	}
	int psIndex = realShader->GetConstantBufferIndex(Gnm::kShaderStageVs, uniform.c_str());
	if (psIndex != -1) {
		currentGFXContext->setConstantBuffers(Gnm::kShaderStagePs, objIndex, 1, &buffer);
	}
}

#endif