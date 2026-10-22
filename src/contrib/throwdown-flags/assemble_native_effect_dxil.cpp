// Offline DXIL assembly/validation only; does not open or control the game.
#include <Windows.h>
#include <wrl/client.h>
#include <dxc/dxcapi.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <string>
using Microsoft::WRL::ComPtr;
void ok(HRESULT h){if(FAILED(h))throw std::runtime_error("DXC operation failed: "+std::to_string(h));}
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("input.ll output.dxbc");
 auto library=LoadLibraryW(L"C:/VulkanSDK/1.4.350.0/Bin/dxcompiler.dll");if(!library)throw std::runtime_error("DXC library unavailable");
 const auto create=reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(library,"DxcCreateInstance"));if(!create)throw std::runtime_error("DXC factory unavailable");
 std::ifstream f(argv[1],std::ios::binary);std::vector<char> source((std::istreambuf_iterator<char>(f)),{});if(source.empty())throw std::runtime_error("Empty shader source");
 ComPtr<IDxcUtils> utils;ok(create(CLSID_DxcUtils,IID_PPV_ARGS(&utils)));
 ComPtr<IDxcBlobEncoding> text;ok(utils->CreateBlob(source.data(),UINT32(source.size()),DXC_CP_UTF8,&text));
 ComPtr<IDxcAssembler> assembler;ok(create(CLSID_DxcAssembler,IID_PPV_ARGS(&assembler)));ComPtr<IDxcOperationResult> result;
 ok(assembler->AssembleToContainer(text.Get(),&result));HRESULT status{};ok(result->GetStatus(&status));
 if(FAILED(status)){ComPtr<IDxcBlobEncoding> errors;result->GetErrorBuffer(&errors);if(errors)std::cerr.write(static_cast<char*>(errors->GetBufferPointer()),errors->GetBufferSize());ok(status);}
 ComPtr<IDxcBlob> binary;ok(result->GetResult(&binary));
 ComPtr<IDxcValidator> validator;ok(create(CLSID_DxcValidator,IID_PPV_ARGS(&validator)));ComPtr<IDxcOperationResult> validation;
 ok(validator->Validate(binary.Get(),DxcValidatorFlags_InPlaceEdit,&validation));ok(validation->GetStatus(&status));
 if(FAILED(status)){ComPtr<IDxcBlobEncoding> errors;validation->GetErrorBuffer(&errors);if(errors)std::cerr.write(static_cast<char*>(errors->GetBufferPointer()),errors->GetBufferSize());ok(status);}
 ComPtr<IDxcBlob> checked;ok(validation->GetResult(&checked));
 std::ofstream out(argv[2],std::ios::binary);out.write(static_cast<char*>(checked->GetBufferPointer()),checked->GetBufferSize());
 std::cout<<"PASS DXIL assembly and validation bytes="<<checked->GetBufferSize()<<"\n";
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
