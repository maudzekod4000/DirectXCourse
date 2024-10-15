#include <dxgi1_6.h>
#pragma comment(lib, "dxgi.lib")

#include <iostream>
#include <wrl/client.h>

void printGPUInfo(IDXGIFactory4* factory) {
	UINT idx = 0;
	Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
	
	while (factory->EnumAdapters1(idx, &adapter) != DXGI_ERROR_NOT_FOUND) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		std::wcout << L"Adapter " << idx << L": " << desc.Description << std::endl;
		std::wcout << L"  Dedicated Video Memory: " << desc.DedicatedVideoMemory / (1024 * 1024) << L" MB" << std::endl;
		std::wcout << L"  Dedicated System Memory: " << desc.DedicatedSystemMemory / (1024 * 1024) << L" MB" << std::endl;
		std::wcout << L"  Shared System Memory: " << desc.SharedSystemMemory / (1024 * 1024) << L" MB" << std::endl;
		idx++;
	}
}

int main() {
	IDXGIFactory4* dxgiFactory = nullptr;
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

	if (FAILED(hr)) {
		std::cout << "Direct X failed!" << std::endl;
		return 1;
	}

	printGPUInfo(dxgiFactory);
	return 0;
}
