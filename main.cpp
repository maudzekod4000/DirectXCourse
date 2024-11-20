#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")
#include <d3d12.h>
#pragma comment(lib, "d3d12.lib")

#include <iostream>

void displayAdapterInfo(IDXGIAdapter1* adapter, UINT idx) {
	DXGI_ADAPTER_DESC1 desc;
	HRESULT hr = adapter->GetDesc1(&desc);
	if (FAILED(hr)) {
		std::cerr << "Failed to get description for adapter " << idx << std::endl;
		return;
	}

	std::wcout << L"Adapter " << idx << L": " << desc.Description << std::endl;
	std::wcout << L"  Dedicated Video Memory: " << desc.DedicatedVideoMemory / (1024 * 1024) << L" MB" << std::endl;
	std::wcout << L"  Dedicated System Memory: " << desc.DedicatedSystemMemory / (1024 * 1024) << L" MB" << std::endl;
	std::wcout << L"  Shared System Memory: " << desc.SharedSystemMemory / (1024 * 1024) << L" MB" << std::endl;
}

void printGPUInfo(IDXGIFactory4* factory) {
	UINT idx = 0;
	IDXGIAdapter1* adapter;
	
	while (factory->EnumAdapters1(idx, &adapter) != DXGI_ERROR_NOT_FOUND) {
		displayAdapterInfo(adapter, idx);
		idx++;
		adapter->Release();
	}
}

bool adapterSupportsDirectX(IDXGIAdapter1* adapter, D3D_FEATURE_LEVEL feature) {
	ID3D12Device* device = nullptr;
	const HRESULT hr = D3D12CreateDevice(
		adapter,
		feature,
		IID_PPV_ARGS(&device)
	);

	if (SUCCEEDED(hr)) {
		device->Release();  // Release the device if it was created successfully
		return true;
	}
	return false;
}

IDXGIAdapter1* selectD12EnabledAdapter(IDXGIFactory4* factory) {
	UINT idx = 0;
	IDXGIAdapter1* adapter;

	while (factory->EnumAdapters1(idx, &adapter) != DXGI_ERROR_NOT_FOUND) {
		if (adapterSupportsDirectX(adapter, D3D_FEATURE_LEVEL_12_0)) {
			return adapter;
		}
	}
	return nullptr; // No DirectX enabled devices found;
}

ID3D12Device* createDeviceFromAdapter(IDXGIAdapter1* adapter) {
	ID3D12Device* device = nullptr;
	const HRESULT hr = D3D12CreateDevice(
		adapter,
		D3D_FEATURE_LEVEL_12_0,
		IID_PPV_ARGS(&device)
	);

	if (FAILED(hr)) {
		return nullptr;
	}

	return device;
}

ID3D12CommandQueue* createCommandQueue(ID3D12Device* device) {
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;

	ID3D12CommandQueue* commandQueue = nullptr;
	HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue));
	if (FAILED(hr)) {
		return nullptr;
	}

	return commandQueue;
}

ID3D12CommandAllocator* createCommandAllocator(ID3D12Device* device) {
	ID3D12CommandAllocator* commandAllocator = nullptr;

	HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

	if (FAILED(hr)) {
		return nullptr;
	}

	return commandAllocator;
}

ID3D12GraphicsCommandList* createGraphicsCommandList(ID3D12Device* device,
	ID3D12CommandAllocator* commandAllocator)
{
	ID3D12GraphicsCommandList* commandList = nullptr;

	// Create the graphics command list
	HRESULT hr = device->CreateCommandList(0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		commandAllocator,
		nullptr, // No initial pipeline state
		IID_PPV_ARGS(&commandList));

	if (FAILED(hr)) {
		return nullptr;
	}

	return commandList;
}

ID3D12Resource* createTextureResource(ID3D12Device* d) {
	D3D12_RESOURCE_DESC desc;
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = 640;
	desc.Height = 480;
	desc.DepthOrArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1; // AA
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

	ID3D12Resource* resource;

	HRESULT res = d->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(&resource)
	);

	if (FAILED(res)) {
		return nullptr;
	}
	return resource;
}

ID3D12Resource* createReadbackBuffer(ID3D12Device* d, ID3D12Resource* source) {
	D3D12_RESOURCE_DESC readbackBufferDesc = {};
	readbackBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	// Outputs
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[1] = {};
	UINT numRows[1] = {};
	UINT64 rowSizesInBytes[1] = {};
	UINT64 totalBytes = 0;

	// Call GetCopyableFootprints
	d->GetCopyableFootprints(
		&source->GetDesc(),  // Resource description
		0,              // First subresource
		1,              // Number of subresources
		0,              // Base offset
		layouts,        // Subresource layouts
		numRows,        // Number of rows
		rowSizesInBytes,// Row sizes
		&totalBytes     // Total size
	);

	// Retrieve the width
	UINT64 width = layouts[0].Footprint.Width;
	readbackBufferDesc.Width = width;
	// TODO: Continue onward.
	readbackBufferDesc.Height = 1;
	readbackBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	readbackBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_READBACK;

	ID3D12Resource* resource;

	HRESULT res = d->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&readbackBufferDesc,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		nullptr,
		IID_PPV_ARGS(&resource)
	);

	if (FAILED(res)) {
		return nullptr;
	}
	return resource;
}

ID3D12DescriptorHeap* createDescriptorHeap(ID3D12Device* d) {
	D3D12_DESCRIPTOR_HEAP_DESC descriptorDesc;
	descriptorDesc.NumDescriptors = 1;
	descriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ID3D12DescriptorHeap* heap;
	HRESULT res = d->CreateDescriptorHeap(&descriptorDesc, IID_PPV_ARGS(&heap));

	if (FAILED(res)) {
		return nullptr;
	}
	return heap;
}

int main() {
	IDXGIFactory4* dxgiFactory;
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	if (FAILED(hr)) {
		std::cerr << "Direct X failed!" << std::endl;
		return 1;
	}

	printGPUInfo(dxgiFactory);

	IDXGIAdapter1* d12Adapter = selectD12EnabledAdapter(dxgiFactory);

	if (!d12Adapter) {
		std::cerr << "Could not find DirectX 12 compatible adapter." << std::endl;
		return 1;
	}

	std::cout << "----" << std::endl;
	std::cout << "Selected DirectX 12 adapter:" << std::endl;
	displayAdapterInfo(d12Adapter, 0);

	ID3D12Device* d12Device = createDeviceFromAdapter(d12Adapter);
	d12Adapter->Release();

	if (!d12Device) {
		std::cerr << "Failed to create DirectX 12 device. Aborting..." << std::endl;
		return 1;
	}

	std::cout << "Device created" << std::endl;

	ID3D12CommandQueue* commandQ = createCommandQueue(d12Device);

	if (!commandQ) {
		std::cerr << "Failed to create command queue." << std::endl;
		return 1;
	}

	std::cout << "Command queue created" << std::endl;

	ID3D12CommandAllocator* commandAlloc = createCommandAllocator(d12Device);

	if (!commandAlloc) {
		std::cerr << "Failed to create command allocator." << std::endl;
		return 1;
	}

	std::cout << "Command allocator created." << std::endl;

	ID3D12GraphicsCommandList* commandList = createGraphicsCommandList(d12Device, commandAlloc);

	if (!commandList) {
		std::cerr << "Failed to create command list" << std::endl;
		return 1;
	}

	std::cout << "Graphics command list created." << std::endl;

	// Do some work
	ID3D12Resource* renderTarget = createTextureResource(d12Device);

	if (!renderTarget) {
		std::cerr << "Failed to create render target resource" << std::endl;
		return 1;
	}

	ID3D12DescriptorHeap* descriptorHeap = createDescriptorHeap(d12Device);

	if (!descriptorHeap) {
		std::cerr << "Failed to create descriptor heap to store all the descriptors to resources" << std::endl;
		return 1;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	d12Device->CreateRenderTargetView(renderTarget, nullptr, renderTargetViewHandle);

	// Commands
	// 1. Set the render target on which the GPU will work
	commandList->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, nullptr);

	// 2. Set the clear color
	FLOAT clearColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	commandList->ClearRenderTargetView(renderTargetViewHandle, clearColor, 0, nullptr);

	ID3D12Resource* readbackBuffer = createReadbackBuffer(d12Device, renderTarget);

	if (!readbackBuffer) {
		std::cerr << "Failed to create readback buffer" << std::endl;
		return 1;
	}

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Transition.pResource = renderTarget;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;

	commandList->ResourceBarrier(1, &barrier);

	D3D12_TEXTURE_COPY_LOCATION sourceLoc;
	sourceLoc.pResource = renderTarget;
	sourceLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	sourceLoc.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION destLoc;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts[1] = {};
	UINT numRows[1] = {};
	UINT64 rowSizesInBytes[1] = {};
	UINT64 totalBytes = 0;

	// Call GetCopyableFootprints
	d12Device->GetCopyableFootprints(
		&renderTarget->GetDesc(),  // Resource description
		0,              // First subresource
		1,              // Number of subresources
		0,              // Base offset
		layouts,        // Subresource layouts
		numRows,        // Number of rows
		rowSizesInBytes,// Row sizes
		&totalBytes     // Total size
	);
	destLoc.PlacedFootprint = layouts[0];
	destLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

	commandList->CopyTextureRegion(&destLoc, 0, 0, 0, &sourceLoc, nullptr);

	D3D12_RESOURCE_BARRIER barrierBack = {};
	barrierBack.Transition.pResource = renderTarget;
	barrierBack.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
	barrierBack.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	commandList->ResourceBarrier(1, &barrierBack);

	commandList->Close();

	ID3D12CommandList* commandLists[] = { commandList };
	commandQ->ExecuteCommandLists(_countof(commandLists), commandLists);

	d12Device->Release();
	commandQ->Release();
	commandAlloc->Release();
	commandList->Release();
	dxgiFactory->Release();
	return 0;
}
