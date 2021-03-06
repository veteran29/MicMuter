//#define _DEBUG
#ifdef _DEBUG
	#include <io.h>
	#include <fcntl.h>
#endif

#include "resource.h"
#include "stdafx.h"

#include <string>
#include <iostream>
#include <functiondiscoverykeys.h>
#include <EndpointVolume.h>

//
const wchar_t *mute_it=L"Microphone";
//

//  Retrieves the device friendly name for device in a device collection
LPWSTR GetDeviceName(IMMDeviceCollection *DeviceCollection, UINT DeviceIndex)
{
    IMMDevice *device;
    LPWSTR deviceId;
    HRESULT hr;

    hr = DeviceCollection->Item(DeviceIndex, &device);
    if (FAILED(hr))
    {
        printf("Unable to get device %d: %x\n", DeviceIndex, hr);
        return NULL;
    }
    hr = device->GetId(&deviceId);
    if (FAILED(hr))
    {
        printf("Unable to get device %d id: %x\n", DeviceIndex, hr);
        return NULL;
    }

    IPropertyStore *propertyStore;
    hr = device->OpenPropertyStore(STGM_READ, &propertyStore);
    SafeRelease(&device);
    if (FAILED(hr))
    {
        printf("Unable to open device %d property store: %x\n", DeviceIndex, hr);
        return NULL;
    }

    PROPVARIANT friendlyName;
    PropVariantInit(&friendlyName);
    hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
    SafeRelease(&propertyStore);

    if (FAILED(hr))
    {
        printf("Unable to retrieve friendly name for device %d : %x\n", DeviceIndex, hr);
        return NULL;
    }

    wchar_t deviceName[128];
    hr = StringCbPrintf(deviceName, sizeof(deviceName), L"%s (%s)", friendlyName.vt != VT_LPWSTR ? L"Unknown" : friendlyName.pwszVal, deviceId);
    if (FAILED(hr))
    {
        printf("Unable to format friendly name for device %d : %x\n", DeviceIndex, hr);
        return NULL;
    }

    PropVariantClear(&friendlyName);
    CoTaskMemFree(deviceId);

    wchar_t *returnValue = _wcsdup(deviceName);
    if (returnValue == NULL)
    {
        printf("Unable to allocate buffer for return\n");
        return NULL;
    }
    return returnValue;
}


int WINAPI WinMain(HINSTANCE hInstance,
            HINSTANCE hPrevInstance,
            LPSTR    lpCmdLine,
            int       ncmdShow)
{
	#ifdef _DEBUG
	//console for debbuging
		AllocConsole();

		HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
		int hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
		FILE* hf_out = _fdopen(hCrt, "w");
		setvbuf(hf_out, NULL, _IONBF, 1);
		*stdout = *hf_out;

		HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
		hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
		FILE* hf_in = _fdopen(hCrt, "r");
		setvbuf(hf_in, NULL, _IONBF, 128);
		*stdin = *hf_in;
	#endif

    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        printf("Unable to initialize COM: %x\n", hr);
        goto Exit;
    }

    IMMDeviceEnumerator *deviceEnumerator = NULL;

	//initialize device enumerator
    hr = CoCreateInstance( __uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, 
		                   IID_PPV_ARGS(&deviceEnumerator) );
    if (FAILED(hr))
    {
        printf("Unable to instantiate device enumerator: %x\n", hr);
        goto Exit;
    }


	IMMDeviceCollection *deviceCollection = NULL;
	
	//Here we enumerate the audio endpoints of interest (in this case audio capture endpoints)
	//into our device collection. We use "eCapture" for audio capture endpoints, "eRender" for 
	//audio output endpoints and "eAll" for all audio endpoints 
    hr = deviceEnumerator->EnumAudioEndpoints( eCapture, DEVICE_STATE_ACTIVE, 
		                                       &deviceCollection );
    if (FAILED(hr))
    {
        printf("Unable to retrieve device collection: %x\n", hr);
        goto Exit;
    }


    UINT deviceCount;

    hr = deviceCollection->GetCount(&deviceCount);
    if (FAILED(hr))
    {
        printf("Unable to get device collection length: %x\n", hr);
        goto Exit;
    }


	IMMDevice *device = NULL;
	
	//
	//This loop goes over each audio endpoint in our device collection,
	//gets and diplays its friendly name and then tries to mute it
	//
    for (UINT i = 0 ; i < deviceCount ; i += 1)
    {
		LPWSTR deviceName;

		//Here we use the GetDeviceName() function provided with the sample 
		//(see source code zip)
		deviceName = GetDeviceName(deviceCollection, i); //Get device friendly name

		if (deviceName == NULL) goto Exit;
		if ( wcscmp(deviceName, mute_it) != 1 )
		{
			printf("Skipped device has index: %d and name: %S\n", i, deviceName);
			continue;
		}
		
		printf("Device to be muted has index: %d and name: %S\n", i, deviceName);

		free(deviceName); //this needs to be done because name is stored in a heap allocated buffer


		device = NULL;

		//Put device ref into device var
		hr = deviceCollection->Item(i, &device);
		if (FAILED(hr))
		{
			printf("Unable to retrieve device %d: %x\n", i, hr);
			goto Exit;
		}


		//This is the Core Audio interface of interest
		IAudioEndpointVolume *endpointVolume = NULL;

		//We activate it here
		hr = device->Activate( __uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, 
			                   reinterpret_cast<void **>(&endpointVolume) );
		if (FAILED(hr))
		{
			printf("Unable to activate endpoint volume on output device: %x\n", hr);
			goto Exit;
		}

		BOOL mute;
		hr = endpointVolume->GetMute(&mute); // Get Mute Status
		if (FAILED(hr))
			{
				printf("Unable to get mute state on endpoint: %x\n", hr);
				goto Exit;
			}

		if(mute)
		{
			hr = endpointVolume->SetMute(FALSE, NULL); //Try to unmute endpoint here
			if (FAILED(hr))
			{
				printf("Unable to set unmute state on endpoint: %x\n", hr);
				goto Exit;
			}
			else
				printf("Endpoint unmuted successfully!\n");
			PlaySound(MAKEINTRESOURCE(IDR_ON),GetModuleHandle(NULL),SND_RESOURCE);
		}
		else
		{
			hr = endpointVolume->SetMute(TRUE, NULL); //Try to mute endpoint here
			if (FAILED(hr))
			{
				printf("Unable to set mute state on endpoint: %x\n", hr);
				goto Exit;
			}
			else
				printf("Endpoint muted successfully!\n");
			PlaySound(MAKEINTRESOURCE(IDR_OFF),GetModuleHandle(NULL),SND_RESOURCE);
		}
    }


Exit: //Core Audio and COM clean up here
    SafeRelease(&deviceCollection);
    SafeRelease(&deviceEnumerator);
    SafeRelease(&device);
    CoUninitialize();
	getchar();
    return 0;
}