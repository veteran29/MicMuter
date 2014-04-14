#include "stdafx.h"
#include <vector>

class EndpointManage
{
public:
	EndpointManage();
	~EndpointManage();
	BOOL error;
	int mute_Endpoint();

private:
	std::vector<IMMDevice*> device;
	LPWSTR GetDeviceName(IMMDeviceCollection *DeviceCollection, UINT DeviceIndex);
	int initialize_DeviceCollection();
	int EndpointManage::mute_captureDevice(IAudioEndpointVolume *endpointVolume);
	int EndpointManage::unmute_captureDevice(IAudioEndpointVolume *endpointVolume);
	
};

EndpointManage::EndpointManage()
{
	this->initialize_DeviceCollection();
}

EndpointManage::~EndpointManage()
{
	for(UINT i=0; i<this->device.size(); i++)
	{
		SafeRelease(&this->device[i]);
	}
	this->device.clear();
	printf("Vector memory free'd");
}

int EndpointManage::initialize_DeviceCollection()
{
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
		this->error=TRUE;
        goto Exit;
    }
	
	//
	//This loop goes over each audio endpoint in our device collection,
	//gets and diplays its friendly name and then tries to mute it
	//
	IMMDevice *device = NULL;
    for (UINT i = 0 ; i < deviceCount ; i += 1)
    {
		LPWSTR deviceName;

		deviceName = GetDeviceName(deviceCollection, i); //Get device friendly name

		if (deviceName == NULL)
		{
			printf("Can't get device name");
			goto Exit;
		}
		/*if ( wcsncmp(deviceName, L"Microphone", strlen("Microphone") ) != 0 )
		{
			printf("Skipped device has index: %d and name: %S\n\n", i, deviceName);
			free(deviceName);
			continue;
		}*/
		
		printf("Device to be muted has index: %d and name: %S\n\n", i, deviceName);
		free(deviceName); //this needs to be done because name is stored in a heap allocated buffer
		
		hr = deviceCollection->Item(i, &device );
		if (FAILED(hr))
		{
			printf("Unable to retrieve device %d: %x\n", i, hr);
			goto Exit;
		}
		this->device.push_back(device); //IMMDevice
	}

	printf("Initialization of device collection finished, it has lenght of %X elements\n",deviceCount);
	printf("Initialization of device vector finished, it has lenght of %X elements\n",this->device.size() );
Exit:
	//SafeRelease(&device);
	SafeRelease(&deviceEnumerator);
	SafeRelease(&deviceCollection);
	return 0;
}

LPWSTR EndpointManage::GetDeviceName(IMMDeviceCollection *DeviceCollection, UINT DeviceIndex)
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

int EndpointManage::mute_Endpoint()
{
		for (UINT i = 0 ; i < this->device.size(); i += 1)
		{
			HRESULT hr;
			//This is the Core Audio interface of interest
			IAudioEndpointVolume *endpointVolume = NULL;

			//We activate it here
			hr = this->device[i]->Activate( __uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, 
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
				mute_captureDevice(endpointVolume);
			else
				unmute_captureDevice(endpointVolume);

			SafeRelease(&endpointVolume);
		}


Exit: //Core Audio clean up here
	printf("Safe relasing\n");
	
    return 0;
}

int EndpointManage::unmute_captureDevice(IAudioEndpointVolume *endpointVolume)
{
	HRESULT hr = endpointVolume->SetMute(TRUE, NULL); //Try to mute endpoint here
			if (FAILED(hr))
			{
				printf("Unable to set mute state on endpoint: %x\n", hr);
			}
			else
				printf("Endpoint muted successfully!\n");
			return 0;
}

int EndpointManage::mute_captureDevice(IAudioEndpointVolume *endpointVolume)
{
	HRESULT hr = endpointVolume->SetMute(FALSE, NULL); //Try to unmute endpoint here
	if (FAILED(hr))
	{
		printf("Unable to set unmute state on endpoint: %x\n", hr);
	}
	else
		printf("Endpoint unmuted successfully!\n");
	return 0;
}