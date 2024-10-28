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

	d12Device->Release();
	commandQ->Release();
	commandAlloc->Release();
	commandList->Release();
	dxgiFactory->Release();
	return 0;
}
