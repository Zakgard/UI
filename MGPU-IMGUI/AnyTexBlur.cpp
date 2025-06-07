#include "AnyTexBlur.h"
#include "d3dcompiler.h"
#include <stdexcept>

AnyTexBlur::AnyTexBlur(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap, UINT descriptorSize)
    : md3dDevice(device), md3dSrvHeap(srvHeap), mDescriptorSize(descriptorSize)
{
    InitializeComputeRootSignature();
    InitializeBlurPSO();
}

AnyTexBlur::~AnyTexBlur()
{
}

void AnyTexBlur::InitializeComputeRootSignature()
{
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc{};
    rootSigDesc.NumParameters = 3; // ���������, SRV, UAV
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    D3D12_ROOT_PARAMETER rootParams[3];
    rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParams[0].Constants.Num32BitValues = 4; // w, h, radius, pass
    rootParams[0].Constants.ShaderRegister = 0;
    rootParams[0].Constants.RegisterSpace = 0;
    rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParams[1].Descriptor.ShaderRegister = 0;
    rootParams[1].Descriptor.RegisterSpace = 0;
    rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
    rootParams[2].Descriptor.ShaderRegister = 0;
    rootParams[2].Descriptor.RegisterSpace = 0;
    rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    rootSigDesc.pParameters = rootParams;

    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to serialize root signature");
    }

    hr = md3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&mComputeRootSignature));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create root signature");
    }
}

void AnyTexBlur::InitializeBlurPSO()
{

    // ����������� ����� � ������ ��������
    const WCHAR* horzShaderPath = L"Shaders/HorzBlurCS.hlsl";
    const WCHAR* vertShaderPath = L"Shaders/VertBlurCS.hlsl";

    // ���������� ��� �������� ���������������� �������� � ������
    ComPtr<ID3DBlob> horzShaderBlob;
    ComPtr<ID3DBlob> vertShaderBlob;
    ComPtr<ID3DBlob> errorBlob;

    // ���������� ��������������� �������
    HRESULT hr = D3DCompileFromFile(
        horzShaderPath,        // ���� � ����� �������
        nullptr,               // ������� (�� ������������)
        nullptr,               // ������� (�� ������������)
        "CSMain",              // ��� ������� ����� �������
        "cs_5_0",              // ������� ������ ������� (Compute Shader 5.0)
        0,                     // ����� ���������� (��� �������������� �����)
        0,                     // ����� �������� (�� ������������)
        &horzShaderBlob,       // ��������� �� blob � ���������������� ��������
        &errorBlob             // ��������� �� blob � ����������� �� �������
    );
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile horizontal blur shader");
    }

    // ���������� ������������� �������
    hr = D3DCompileFromFile(
        vertShaderPath,        // ���� � ����� �������
        nullptr,               // ������� (�� ������������)
        nullptr,               // ������� (�� ������������)
        "CSMain",              // ��� ������� ����� �������
        "cs_5_0",              // ������� ������ ������� (Compute Shader 5.0)
        0,                     // ����� ���������� (��� �������������� �����)
        0,                     // ����� �������� (�� ������������)
        &vertShaderBlob,       // ��������� �� blob � ���������������� ��������
        &errorBlob             // ��������� �� blob � ����������� �� �������
    );
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
        }
        throw std::runtime_error("Failed to compile vertical blur shader");
    }

    // �������� PSO ��� ��������������� ��������
    D3D12_COMPUTE_PIPELINE_STATE_DESC horzPsoDesc = {};
    horzPsoDesc.pRootSignature = mComputeRootSignature.Get();
    horzPsoDesc.CS = { horzShaderBlob->GetBufferPointer(), horzShaderBlob->GetBufferSize() };
    hr = md3dDevice->CreateComputePipelineState(&horzPsoDesc, IID_PPV_ARGS(&mHorzBlurPso));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create horizontal blur PSO");
    }

    // �������� PSO ��� ������������� ��������
    D3D12_COMPUTE_PIPELINE_STATE_DESC vertPsoDesc = {};
    vertPsoDesc.pRootSignature = mComputeRootSignature.Get();
    vertPsoDesc.CS = { vertShaderBlob->GetBufferPointer(), vertShaderBlob->GetBufferSize() };
    hr = md3dDevice->CreateComputePipelineState(&vertPsoDesc, IID_PPV_ARGS(&mVertBlurPso));
    if (FAILED(hr)) {
        throw std::runtime_error("Failed to create vertical blur PSO");
    }
}

void AnyTexBlur::BuildDescriptors(ID3D12Resource* inputTexture, ID3D12Resource* outputTexture)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;

    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;

    // �������� ������ ��������� ���������� � ����
    mInputSrvCpu = md3dSrvHeap->GetCPUDescriptorHandleForHeapStart();
    mInputSrvGpu = md3dSrvHeap->GetGPUDescriptorHandleForHeapStart();
    mOutputUavCpu = { mInputSrvCpu.ptr + mDescriptorSize };
    mOutputUavGpu = { mInputSrvGpu.ptr + mDescriptorSize };

    md3dDevice->CreateShaderResourceView(inputTexture, &srvDesc, mInputSrvCpu);
    md3dDevice->CreateUnorderedAccessView(outputTexture, nullptr, &uavDesc, mOutputUavCpu);
}

void AnyTexBlur::BlurTexture(ID3D12GraphicsCommandList* cmdList, ID3D12Resource* inputTexture, ID3D12Resource* outputTexture, int blurCount)
{
    D3D12_RESOURCE_DESC inputDesc = inputTexture->GetDesc();
    UINT width = (UINT)inputDesc.Width;
    UINT height = inputDesc.Height;

    BuildDescriptors(inputTexture, outputTexture);

    cmdList->SetComputeRootSignature(mComputeRootSignature.Get());

    // ������� ������� �������� � ��������� SRV
    D3D12_RESOURCE_BARRIER barriers[2];
    barriers[0] = {};
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = inputTexture;
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

    // ������� �������� �������� � ��������� UAV
    barriers[1] = {};
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = outputTexture;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    cmdList->ResourceBarrier(2, barriers);

    // ��������� �������� (������, ������, ������ ��������, ������)
    UINT constants[4] = { width, height, 4, 0 }; // ������ 4 � ��������� ��������
    cmdList->SetComputeRoot32BitConstants(0, 4, constants, 0);
    cmdList->SetComputeRootDescriptorTable(1, mInputSrvGpu);
    cmdList->SetComputeRootDescriptorTable(2, mOutputUavGpu);

    // �������������� ��������
    cmdList->SetPipelineState(mHorzBlurPso.Get());
    cmdList->Dispatch((width + 15) / 16, height, 1);

    // ������������ �������� (����� �������� ������������� �������� ��� ���������� ��������)
    cmdList->SetPipelineState(mVertBlurPso.Get());
    cmdList->Dispatch(width, (height + 15) / 16, 1);

    // ���������� ��������� �������
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    cmdList->ResourceBarrier(2, barriers);
}