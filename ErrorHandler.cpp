//==========================================================================
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
//--------------------------------------------------------------------------

//------------------------------------------------------------
//Please read the ReadME.txt which explains the purpose of the
//sample.
//-------------------------------------------------------------

#include "ErrorHandler.h"
#include <Gdiplus.h>
#include <windows.h>
#include <vector>


#pragma comment(lib, "Gdiplus.lib") 

using namespace Gdiplus;

static int pageIndex = 1; // current index of the page
static char command = 's';
static float crop_margin_left = 22; // in percent
static float crop_margin_bottom = 7.2; // in percent


// This function downloads item after setting the format for the item and initializing callback (depending on category,itemtype)
// with the directory to download images to as well as the filename for the downloaded image.
HRESULT DownloadItem(IWiaItem2* pWiaItem2)
{
    if( (!pWiaItem2) )
    {
        HRESULT hr = E_INVALIDARG;
        ReportError(TEXT("Invalid argument passed to DownloadItem()"),hr);
        return hr;
    }
     // Get the IWiaTransfer interface
    IWiaTransfer *pWiaTransfer = NULL;
    HRESULT hr = pWiaItem2->QueryInterface( IID_IWiaTransfer, (void**)&pWiaTransfer );
    if (SUCCEEDED(hr))
    {
        // Create our callback class
        CWiaTransferCallback *pWiaClassCallback = new CWiaTransferCallback;
        if (pWiaClassCallback)
        {
            // Get the IWiaTransferCallback interface from our callback class.
            IWiaTransferCallback *pWiaTransferCallback = NULL;
            hr = pWiaClassCallback->QueryInterface( IID_IWiaTransferCallback, (void**)&pWiaTransferCallback );
            if (SUCCEEDED(hr))
            {
                //Set the format for the item to BMP
                IWiaPropertyStorage* pWiaPropertyStorage = NULL;
                HRESULT hr = pWiaItem2->QueryInterface( IID_IWiaPropertyStorage, (void**)&pWiaPropertyStorage );
                if(SUCCEEDED(hr))
                {
                    hr = WritePropertyGuid(pWiaPropertyStorage,WIA_IPA_FORMAT,WiaImgFmt_JPEG);
                    if(FAILED(hr))
                    {
                        ReportError(TEXT("WritePropertyGuid() failed in DownloadItem().Format couldn't be set to BMP"),hr);
                    }
           
                    //Get the file extension and initialize callback with this extension and directory in which to transfer image 
                    //along with feeder flag.
                    //This will result in m_szFileName member callback class being set on which GetNextStream() will create and return a stream 
                    BSTR bstrFileExtension = NULL;
                    ReadPropertyBSTR(pWiaPropertyStorage,WIA_IPA_FILENAME_EXTENSION, &bstrFileExtension);
            
                    //Get the temporary folder path which is the directory where we will download the images
                    TCHAR bufferTempPath[MAX_TEMP_PATH]  = TEXT("C:\\Users\\nastik\\Pictures\\ǳ���������\\new");
                    //GetTempPath(MAX_TEMP_PATH , bufferTempPath);
            
                    // Find out item type
                    GUID itemCategory = GUID_NULL;
                    ReadPropertyGuid(pWiaItem2,WIA_IPA_ITEM_CATEGORY,&itemCategory );
                    
                    BOOL bFeederTransfer = FALSE;
                    
                    if(IsEqualGUID(itemCategory,WIA_CATEGORY_FEEDER))
                    {
                        //Set WIA_IPS_PAGES to ALL_PAGES will enable transfer of all pages in the document feeder (multi-page transfer).
                        //If somebody wants to scan a specific number of pages say N, he should set WIA_IPS_PAGES to N. 
                        WritePropertyLong(pWiaPropertyStorage,WIA_IPS_PAGES,ALL_PAGES);
                         
                        bFeederTransfer = TRUE;
            
                    }

                    //Initialize the callback class with the directory, file extension and feeder flag
                    pWiaClassCallback->InitializeCallback(bufferTempPath,bstrFileExtension,bFeederTransfer);
                    //Now download file item
                    hr = pWiaTransfer->Download(0,pWiaTransferCallback);


                    if(S_OK == hr)
                    {
                        _tprintf(TEXT("\npWiaTransfer->Download() on file item SUCCEEDED"));
                    }
                    else if(S_FALSE == hr)
                    {
                        ReportError(TEXT("pWiaTransfer->Download() on file item returned S_FALSE. File may be empty"),hr);
                    }
                    else if(FAILED(hr))
                    {
                        ReportError(TEXT("pWiaTransfer->Download() on file item failed"),hr);
                    }
            
                    //Release pWiaPropertyStorage interface
                    pWiaPropertyStorage->Release();
                    pWiaPropertyStorage = NULL;
                }
                else
                {
                    ReportError(TEXT("QueryInterface failed on IID_IWiaPropertyStorage"),hr);
                }

                // Release the callback interface
                pWiaTransferCallback->Release();
                pWiaTransferCallback = NULL;
            }
            else
            {
                ReportError( TEXT("pWiaClassCallback->QueryInterface failed on IID_IWiaTransferCallback"), hr );
            }
            // Release our callback.  It should now delete itself.
            pWiaClassCallback->Release();
            pWiaClassCallback = NULL;
        }
        else
        {
            ReportError( TEXT("Unable to create CWiaTransferCallback class instance") );
        }
            
        // Release the IWiaTransfer
        pWiaTransfer->Release();
        pWiaTransfer = NULL;
    }
    else
    {
        ReportError( TEXT("pIWiaItem2->QueryInterface failed on IID_IWiaTransfer"), hr );
    }
    return hr;
}


    //
    // Constructor and destructor
    //
    CWiaTransferCallback::CWiaTransferCallback()
    {
        m_cRef             = 1;
        m_lPageCount       = 0;
        m_bFeederTransfer  = FALSE;
        m_bstrFileExtension = NULL;
        m_bstrDirectoryName = NULL;
		m_ItemName = NULL;
        memset(m_szFileName,0,sizeof(m_szFileName));
    }
    
    CWiaTransferCallback::~CWiaTransferCallback()
    {
        if(m_bstrDirectoryName)
        {
            SysFreeString(m_bstrDirectoryName);
            m_bstrDirectoryName = NULL;
        }
        
        if(m_bstrFileExtension)
        {
            SysFreeString(m_bstrFileExtension);
            m_bstrFileExtension = NULL;
        }

		if (m_ItemName)
		{
			SysFreeString(m_ItemName);
			m_ItemName = NULL;
		}

    }

    // This function initializes various members of class CWiaTransferCallback like directory where images will be downloaded, 
    // filename extension and feeder transfer flag. It also initializes m_pWiaTransfer.
    HRESULT CWiaTransferCallback::InitializeCallback(TCHAR* bstrDirectoryName, BSTR bstrExt, BOOL bFeederTransfer )
    {
        HRESULT hr = S_OK;
        m_bFeederTransfer = bFeederTransfer;

        if(bstrDirectoryName)
        {
            m_bstrDirectoryName = SysAllocString(bstrDirectoryName);
            if(!m_bstrDirectoryName)
            {
                hr = E_OUTOFMEMORY;
                ReportError(TEXT("\nFailed to allocate memory for BSTR directory name"),hr);
                return hr;
            }
        }
        else
        {
            _tprintf(TEXT("\nNo directory name was given"));
            return E_INVALIDARG;
        }

        if (bstrExt)
        {
            m_bstrFileExtension = bstrExt;
        }
        
        return hr;
    }

    
    //
    // IUnknown functions
    //
    HRESULT CALLBACK CWiaTransferCallback::QueryInterface( REFIID riid, void **ppvObject )
    {
        // Validate arguments
        if (NULL == ppvObject)
        {
            return E_INVALIDARG;
        }

        // Return the appropropriate interface
        if (IsEqualIID( riid, IID_IUnknown ))
        {
            *ppvObject = static_cast<IWiaTransferCallback*>(this);
        }
        else if (IsEqualIID( riid, IID_IWiaTransferCallback ))
        {
            *ppvObject = static_cast<IWiaTransferCallback*>(this);
        }
        else if (IsEqualIID( riid, IID_IWiaAppErrorHandler ))
        {
            *ppvObject = static_cast<IWiaAppErrorHandler*>(this);
        }
        else
        {
            *ppvObject = NULL;
            return (E_NOINTERFACE);
        }

        // Increment the reference count before we return the interface
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();
        return S_OK;
    }
    
    ULONG CALLBACK CWiaTransferCallback::AddRef()
    {
        return InterlockedIncrement((long*)&m_cRef);
    }    
    ULONG CALLBACK CWiaTransferCallback::Release()
    {
        LONG cRef = InterlockedDecrement((long*)&m_cRef);
        if (0 == cRef)
        {
            delete this;
        }
        return cRef;
    }
    
//
// IWiaTransferCallback functions
HRESULT STDMETHODCALLTYPE CWiaTransferCallback::TransferCallback(LONG lFlags, WiaTransferParams* pWiaTransferParams)
{
    HRESULT hr = S_OK;
    
    if(pWiaTransferParams == NULL)
    {
        hr = E_INVALIDARG;
        ReportError(TEXT("\nTransferCallback was called with invalid args"),hr);
        return hr;
    }

    switch (pWiaTransferParams->lMessage)
    {
        case WIA_TRANSFER_MSG_STATUS:
            {
                _tprintf(TEXT("\rWIA_TRANSFER_MSG_STATUS - %ld%% complete"),pWiaTransferParams->lPercentComplete);
            }
            break;
        case WIA_TRANSFER_MSG_END_OF_STREAM:
            {
                _tprintf(TEXT("\nWIA_TRANSFER_MSG_END_OF_STREAM"));
            }
            break;
        case WIA_TRANSFER_MSG_END_OF_TRANSFER:
            {
                _tprintf(TEXT("\nWIA_TRANSFER_MSG_END_OF_TRANSFER"));
                _tprintf(TEXT("\nImage Transferred to file %ws"), m_szFileName);
				RotateAndCropJpeg(m_bstrDirectoryName, m_szFileName);
				pageIndex++;
            }
            break;
        case WIA_TRANSFER_MSG_DEVICE_STATUS:
            {
                _tprintf(TEXT("\nWIA_TRANSFER_MSG_DEVICE_STATUS - %ld%% complete"),pWiaTransferParams->lPercentComplete);
            }
            break;
        default:
            break;
    }
    return hr;
}



// This function is called by WIA service to get data stream from the application.
HRESULT STDMETHODCALLTYPE CWiaTransferCallback::GetNextStream(LONG lFlags, BSTR bstrItemName, BSTR bstrFullItemName, IStream **ppDestination)
{
    _tprintf(TEXT("\nGetNextStream"));
    HRESULT hr = S_OK;

    if ( (!ppDestination) || (!bstrItemName) || (!m_bstrDirectoryName) )   
    {
        hr = E_INVALIDARG;
        ReportError(TEXT("GetNextStream() was called with invalid parameters"),hr);
        return hr;
    }
    //Initialize out variables
    *ppDestination = NULL;
    

    if(m_bstrFileExtension)
    {
		if (bstrItemName)
		{
			m_ItemName = SysAllocString(bstrItemName);
			if (!m_ItemName)
			{
				hr = E_OUTOFMEMORY;
				ReportError(TEXT("\nFailed to allocate memory for BSTR item name"), hr);
				return hr;
			}
		}
		else
		{
			_tprintf(TEXT("\nNo item name was given"));
			return E_INVALIDARG;
		}

        //For feeder transfer, append the page count to the filename
        if (m_bFeederTransfer)
        {
            StringCchPrintf(m_szFileName, ARRAYSIZE(m_szFileName), TEXT("%ws\\%ws_page%d.%ws"), m_bstrDirectoryName, bstrItemName, ++m_lPageCount, m_bstrFileExtension);
        }
        else
        {
            StringCchPrintf(m_szFileName, ARRAYSIZE(m_szFileName), TEXT("%ws\\%ws_%02d.%ws"), m_bstrDirectoryName, bstrItemName, pageIndex, m_bstrFileExtension);
        }
    }
    
    else
    {
        // Dont append extension if m_bstrFileExtension = NULL.
        StringCchPrintf(m_szFileName, ARRAYSIZE(m_szFileName), TEXT("%ws\\%ws"), m_bstrDirectoryName, bstrItemName);
    }
    
    hr = SHCreateStreamOnFile(m_szFileName,STGM_CREATE | STGM_READWRITE,ppDestination);
    if (SUCCEEDED(hr))
    {
        // We're not going to keep the Stream around, so don't AddRef.
        //  The caller will release the stream when done.
    }
    else
    {
        _tprintf(TEXT("\nFailed to Create a Stream on File %ws"),m_szFileName);
        *ppDestination = NULL;
    }
    return hr;
}
// End IWiaTransferCallback functions


//
// IWiaAppErrorHandler functions
//
//
//This function handles various errors occuring during data transfer.It pops up a window asking for user intervention
//in case of 4 errors:
//1. WIA_STATUS_WARMING_UP - This is an informational message from the driver implying that the device is warming up.
//2. WIA_ERROR_WARMING_UP - This is an error message from the driver implying that the device is warming up.
//3. WIA_ERROR_PAPER_EMPTY - This is an error message from the driver implying that the device does not have paper to scan from.
//4. WIA_ERROR_COVER_OPEN - This is an error message from the driver implying that the scanner cover is open.
HRESULT STDMETHODCALLTYPE CWiaTransferCallback::ReportStatus(LONG lFlags,IWiaItem2* pWiaItem2,HRESULT hrStatus,LONG lPercentComplete)
{
    if(!pWiaItem2)
    {
        HRESULT hr = E_INVALIDARG;
        ReportError(TEXT("CWiaTransferCallback::ReportStatus() was called with invalid parameters"),hr);
        return hr;
    }
    
    HRESULT hr = WIA_STATUS_NOT_HANDLED;

    if (WIA_STATUS_WARMING_UP == hrStatus)
    {
        int i = MessageBox(0, TEXT("The scanner lamp is warming up, please wait. Hitting 'Cancel' will abort the transfer."), TEXT("Application Error Handler"), MB_RETRYCANCEL|MB_TASKMODAL);
        if (IDRETRY == i)
        {
            hr = S_OK;
        }
        else
        {
            hr = hrStatus;
        }
    }

    
    if (WIA_ERROR_COVER_OPEN == hrStatus)
    {
        int i = MessageBox(0, TEXT("Scanner cover is open. Please close the cover and press OK to continue. Hitting 'Cancel' will abort the transfer.") , TEXT("Application Error Handler"), MB_RETRYCANCEL|MB_TASKMODAL|MB_ICONERROR);
        if (IDRETRY == i)
        {
            hr = S_OK;
        }
        else
        {
            hr = hrStatus;
        }
    }
    
    if (WIA_ERROR_PAPER_EMPTY == hrStatus)
    {
        int i = MessageBox(0, TEXT("No paper to scan from. Please put paper and press OK to continue. Hitting 'Cancel' will abort the transfer."), TEXT("Application Error Handler"), MB_RETRYCANCEL|MB_TASKMODAL|MB_ICONERROR);
        if (IDRETRY == i)
        {
            hr = S_OK;
        }
        else
        {
            hr = hrStatus;
        }
    }

    if (WIA_ERROR_WARMING_UP == hrStatus)
    {
        int i = MessageBox(0, TEXT("The scanner lamp is warming up, please wait. Hitting 'Cancel' will abort the transfer."), TEXT("Application Error Handler"), MB_RETRYCANCEL|MB_TASKMODAL);
        if (IDRETRY == i)
        {
            hr = S_OK;
        }
        else
        {
            hr = hrStatus;
        }
    }
    return hr;
}

// This function returns the handle to the parent window. We will return O since there is no window in our application.
HRESULT STDMETHODCALLTYPE CWiaTransferCallback::GetWindow( HWND *phwnd )
{
    *phwnd = 0;
    return S_OK;
}


// This function enumerates items recursively starting from the root item and calls DownloadItem() on those items
// after checking their item types.
// DownloadItem() will only be called on file items. Folder items are recursively enumerated.
HRESULT EnumerateAndDownloadItems( IWiaItem2 *pWiaItem2 )
{
    // Validate arguments
    if (NULL == pWiaItem2)
    {
        HRESULT hr = E_INVALIDARG;
        ReportError(TEXT("Invalid argument passed to EnumerateAndDownloadItems()"),hr);
        return hr;
    }

    // Get the item type for this item
    LONG lItemType = 0;
    HRESULT hr = pWiaItem2->GetItemType( &lItemType );

    // Print the item name.
    PrintItemName( pWiaItem2 );

    //Find the item category
    GUID itemCategory = GUID_NULL;
    ReadPropertyGuid(pWiaItem2,WIA_IPA_ITEM_CATEGORY,&itemCategory );
     
    // If this is an transferrable file(except finished files) , download it.
    // We have deliberately left finished files for simplicity
    if ( (lItemType & WiaItemTypeFile) && (lItemType & WiaItemTypeTransfer) && (itemCategory != WIA_CATEGORY_FINISHED_FILE) )
    {
		
		bool fistIteration = true;
		int timeoutValue = 6;
		std::cout << "press 'a' for auto scanning mode, 'n' for scan next page, 'e' if there is no more pages to scan so break and exit, "
			"'r' - reset the counter to some number, 'm' to set left and bottom margin, CTRL+BREAK -  to terminate when the page will be scanned" 
			<< std::endl;
		while (1)
		{
			switch (command)
			{
			case 'a':
			{
				if (!fistIteration)
				{
					sleepForSeconds(timeoutValue);
					if (command == 's')
						break;
				}
			}
			case 'n':
			{
				std::cout << "Scanning " << pageIndex << "-th page of the book" << std::endl;

				hr = DownloadItem(pWiaItem2);
				std::cout << std::endl;
				fistIteration = false;
				break;
			}
			case 'e':
			{
				std::cout << "no more pages to scan\n";
				break;
			}
			case 'r':
			{
				int resetPageIndex = 1;
				std::cout << "What is the new NUMBER OF THE NEXT PAGE to continue?" << std::endl;
				std::cin >> resetPageIndex;
				pageIndex = resetPageIndex;
				std::cout << "page counter reset to " << pageIndex << std::endl;
				break;
			}
			case 't':
			{
				std::cout << "What is the new TIMEOUT value for autoscan?" << std::endl;
				std::cin >> timeoutValue;				
				break;
			}
			case 's': // stop auto or do nothing
				// pass
				break;
			case 'm':
				std::cout << "Left margin is " << crop_margin_left << "%\n";
				std::cout << "Bottom margin is " << crop_margin_bottom << "%\n";
				std::cout << "New left margin (XX.XX):\n";
				std::cin >> crop_margin_left;
				std::cout << "New bottom margin (XX.XX):\n";
				std::cin >> crop_margin_bottom;
				std::cout << "Left and Bottom margins has been set to " << crop_margin_left << " and " << crop_margin_bottom << std::endl;
				break;
			default: // wrong input
				std::cout << "try again\n";
				break;
			}
			
			if (command == 'e')
				break;

			if (command != 'a')
			{
				command = 0;
				std::cin >> command;
			}			
		}        
    }

    // If it is a folder, enumerate its children
    if (lItemType & WiaItemTypeFolder)
    {
        // Get the child item enumerator for this item
        IEnumWiaItem2 *pEnumWiaItem2 = NULL;
        hr = pWiaItem2->EnumChildItems(0, &pEnumWiaItem2 );
        if (SUCCEEDED(hr))
        {
            // We will loop until we get an error or pEnumWiaItem->Next returns
            // S_FALSE to signal the end of the list.
            while (S_OK == hr )
            {
                // Get the next child item
                IWiaItem2 *pChildWiaItem2 = NULL;
                hr = pEnumWiaItem2->Next( 1, &pChildWiaItem2, NULL );

                // pEnumWiaItem->Next will return S_FALSE when the list is
                // exhausted, so check for S_OK before using the returned
                // value.
                if (S_OK == hr)
                {
                    // Recurse into this item
                    EnumerateAndDownloadItems( pChildWiaItem2 );

                    // Release this item
                    pChildWiaItem2->Release();
                    pChildWiaItem2 = NULL;
                }
                else if (FAILED(hr))
                {
                    // Report that an error occurred during enumeration
                    ReportError( TEXT("Error calling pEnumWiaItem2->Next"), hr );
                }
            }

            // If the result of the enumeration is S_FALSE, since this
            // is normal, we will change it to S_OK
            if (S_FALSE == hr)
            {
                hr = S_OK;
            }

            // Release the enumerator
            pEnumWiaItem2->Release();
            pEnumWiaItem2 = NULL;
        }
        else
        {
            ReportError(TEXT("pWiaItem2->EnumChildItems() failed"),hr);
        }
    }
    return  hr;
}

//This function enumerates WIA devices and then creates an instance of each device.
//After that it calls EnumerateAndDownloadItems() on the root item got from creation of device.
HRESULT EnumerateWiaDevices( IWiaDevMgr2 *pWiaDevMgr2 )
{
    // Validate arguments
    if (NULL == pWiaDevMgr2)
    {
        HRESULT hr = E_INVALIDARG;
        ReportError(TEXT("Invalid argument passed to EnumerateWiaDevices()"),hr);
        return hr;
    }

    // Get a device enumerator interface
    IEnumWIA_DEV_INFO *pWiaEnumDevInfo = NULL;
    HRESULT hr = pWiaDevMgr2->EnumDeviceInfo( WIA_DEVINFO_ENUM_LOCAL, &pWiaEnumDevInfo );
    if (SUCCEEDED(hr))
    {
        // Reset the device enumerator to the beginning of the list
        hr = pWiaEnumDevInfo->Reset();
        if (SUCCEEDED(hr))
        {
            // We will loop until we get an error or pWiaEnumDevInfo->Next returns
            // S_FALSE to signal the end of the list.
            while (S_OK == hr)
            {
                // Get the next device's property storage interface pointer
                IWiaPropertyStorage *pWiaPropertyStorage = NULL;
                hr = pWiaEnumDevInfo->Next( 1, &pWiaPropertyStorage, NULL );

                // pWiaEnumDevInfo->Next will return S_FALSE when the list is
                // exhausted, so check for S_OK before using the returned
                // value.
                if (hr == S_OK)
                {
                    // Read some device properties - Device ID,name and descripion and return Device ID needed for creating Device
                    BSTR bstrDeviceID = NULL;
                    HRESULT hr1 = ReadWiaPropsAndGetDeviceID( pWiaPropertyStorage ,&bstrDeviceID);
                    if(SUCCEEDED(hr1))
                    {
                        // Call a function to create the device using device ID 
                        IWiaItem2 *pWiaRootItem2 = NULL;
                        hr1 = pWiaDevMgr2->CreateDevice( 0, bstrDeviceID, &pWiaRootItem2 );
                        if(SUCCEEDED(hr1))
                        {


                            // Enumerate items and for each item do transfer
                            hr1 = EnumerateAndDownloadItems( pWiaRootItem2 );
                            if(FAILED(hr1))
                            {
                                ReportError(TEXT("EnumerateAndDownloadItems() failed in EnumerateWiaDevices()"),hr1);
                            }
                            //Release pWiaRootItem2
                            pWiaRootItem2->Release();
                            pWiaRootItem2 = NULL;
                        }
                        else
                        {
                            ReportError(TEXT("Error calling IWiaDevMgr2::CreateDevice()"),hr1);
                        }
                    }
                    else
                    {
                        ReportError(TEXT("ReadWiaPropsAndGetDeviceID() failed in EnumerateWiaDevices()"),hr1);
                    }
                        
                    // Release the device's IWiaPropertyStorage*
                    pWiaPropertyStorage->Release();
                    pWiaPropertyStorage = NULL;
                }
                else if (FAILED(hr))
                {
                    // Report that an error occurred during enumeration
                    ReportError( TEXT("Error calling IEnumWIA_DEV_INFO::Next()"), hr );
                }
            }
            
            //
            // If the result of the enumeration is S_FALSE, since this
            // is normal, we will change it to S_OK.
            //
            if (S_FALSE == hr)
            {
                hr = S_OK;
            }
        }
        else
        {
            // Report that an error occurred calling Reset()
            ReportError( TEXT("Error calling IEnumWIA_DEV_INFO::Reset()"), hr );
        }
        // Release the enumerator
        pWiaEnumDevInfo->Release();
        pWiaEnumDevInfo = NULL;
    }
    else
    {
        // Report that an error occurred trying to create the enumerator
        ReportError( TEXT("Error calling IWiaDevMgr2::EnumDeviceInfo"), hr );
    }

    // Return the result of the enumeration
    return hr;
}

bool consoleHandler(int signal) {

	if (signal == CTRL_BREAK_EVENT) {
		std::cout << "\nCtrl Break Event invoked - canceling auto" << std::endl;
		command = 's';
	}
	return true;
}

// The entry function of the application
extern "C" 
int __cdecl _tmain( int, TCHAR *[] )
{

	//RotateJpeg();
	//return 0;

	SetConsoleCtrlHandler((PHANDLER_ROUTINE)consoleHandler, TRUE);


    // Initialize COM
    HRESULT hr = CoInitialize(NULL);
    if (SUCCEEDED(hr))
    {
        // Create the device manager
        IWiaDevMgr2 *pWiaDevMgr2 = NULL;
        hr = CoCreateInstance( CLSID_WiaDevMgr2, NULL, CLSCTX_LOCAL_SERVER, IID_IWiaDevMgr2, (void**)&pWiaDevMgr2 );
        if (SUCCEEDED(hr))
        {
            //The following function enumerates all of the WIA devices and performs some function on each device 
            hr = EnumerateWiaDevices( pWiaDevMgr2 );
            if (FAILED(hr))
            {
                ReportError( TEXT("Error calling EnumerateWiaDevices"), hr );
            }

            // Release the device manager
            pWiaDevMgr2->Release();
            pWiaDevMgr2 = NULL;
        }
        else
        {
            ReportError( TEXT("CoCreateInstance() failed on CLSID_WiaDevMgr"), hr );
        }

        // Uninitialize COM
        CoUninitialize();
    }
    return 0;
}

const CLSID pngEncoderClsId = { 0x557cf406, 0x1a04, 0x11d3,{ 0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e } };
const CLSID jpegEncoderClsId = { 0x557cf401, 0x1a04, 0x11d3,{ 0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e } };

Gdiplus::Image* RotateImage(TCHAR originalFile[MAX_FILENAME_LENGTH], int degree = 90)
{
	Gdiplus::Image* image = new Gdiplus::Image(originalFile);
	if (degree == 90)
		image->RotateFlip(Rotate90FlipNone);
	else if (degree == 270)
		image->RotateFlip(Rotate270FlipNone);

	return image;
}

void CWiaTransferCallback::RotateAndCropJpeg(BSTR m_bstrDirectoryName, TCHAR* m_szFileName)
{
	GdiplusStartupInput gdiplusStartupInput;
	ULONG_PTR gdiplusToken;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

	TCHAR originalFile[MAX_FILENAME_LENGTH];
	StringCchPrintf(originalFile, ARRAYSIZE(originalFile), TEXT("%ws\\%ws_%02d.%ws"), m_bstrDirectoryName, m_ItemName, pageIndex, m_bstrFileExtension);

	std::wstring s1 = originalFile;
	std::wstring s2 = m_szFileName;
	assert(s1.compare(s2) == 0);

	// Rotating image
	Gdiplus::Image *image90 = RotateImage(originalFile, 90);
	Gdiplus::Image *image270 = RotateImage(originalFile, 270);

	TCHAR rotatedFile90[MAX_FILENAME_LENGTH];
	StringCchPrintf(rotatedFile90, ARRAYSIZE(rotatedFile90), TEXT("%ws\\R90_%ws_%02d.%ws"), m_bstrDirectoryName, m_ItemName, pageIndex, m_bstrFileExtension);
	Gdiplus::Status stat = image90->Save(rotatedFile90, &jpegEncoderClsId, NULL);

	TCHAR rotatedFile270[MAX_FILENAME_LENGTH];
	StringCchPrintf(rotatedFile270, ARRAYSIZE(rotatedFile90), TEXT("%ws\\R270_%ws_%02d.%ws"), m_bstrDirectoryName, m_ItemName, pageIndex, m_bstrFileExtension);
    stat = image270->Save(rotatedFile270, &jpegEncoderClsId, NULL);
	


	TCHAR croppedFile90[MAX_FILENAME_LENGTH];
	StringCchPrintf(croppedFile90, ARRAYSIZE(croppedFile90), TEXT("%ws\\C90_%ws_%02d.%ws"), m_bstrDirectoryName, m_ItemName, pageIndex, m_bstrFileExtension);

	TCHAR croppedFile270[MAX_FILENAME_LENGTH];
	StringCchPrintf(croppedFile270, ARRAYSIZE(croppedFile270), TEXT("%ws\\C270_%ws_%02d.%ws"), m_bstrDirectoryName, m_ItemName, pageIndex, m_bstrFileExtension);
	
	// crop 90
	Gdiplus::Bitmap* rotatedBitmap90 = Gdiplus::Bitmap::FromFile(rotatedFile90);

	UINT width = rotatedBitmap90->GetWidth();
	UINT height = rotatedBitmap90->GetHeight();

	// Cropping image
	// 2338*1700
	// 356, 1592
	//const int uncroppedWidth = 2338;
	//const int uncroppedHeight = 1700;
	// position of the left bottom crop point
	//const int X = 400; // 356
	//const int Y = 1592;
	//const int croppedWidth = uncroppedWidth - X;
	//const int croppedHeight = Y;
	
	const float x_percent = crop_margin_left / 100.0;
	const float y_percent = 1 - (crop_margin_bottom / 100.0);

	const int x_offset = x_percent * width;
	const int croppedWidth = width - x_offset;
	const int croppedHeight = y_percent* height;

	Gdiplus::Bitmap* croppedBitmap90 = rotatedBitmap90->Clone(x_offset, 0, croppedWidth, croppedHeight, rotatedBitmap90->GetPixelFormat());
	DWORD lastError = GetLastError();
	if (lastError != 0)	std::cout << "GetLastError after rotatedBitmap90->Clone  = " << lastError << std::endl;

	stat = croppedBitmap90->Save(croppedFile90, &jpegEncoderClsId, NULL);
	lastError = GetLastError();
	if (lastError != 0)	std::cout << "GetLastError after croppedBitmap90->Save  = " << lastError << std::endl;

	// crop 270
	Gdiplus::Bitmap* rotatedBitmap270 = Gdiplus::Bitmap::FromFile(rotatedFile270);
	Gdiplus::Bitmap* croppedBitmap270 = rotatedBitmap270->Clone(x_offset, 0, croppedWidth, croppedHeight, rotatedBitmap270->GetPixelFormat());
	lastError = GetLastError();
	if (lastError != 0)	std::cout << "GetLastError after rotatedBitmap270->Clone  = " << lastError << std::endl;
	stat = croppedBitmap270->Save(croppedFile270, &jpegEncoderClsId, NULL);
	lastError = GetLastError();
	if (lastError != 0)	std::cout << "GetLastError after croppedBitmap270->Save  = " << lastError << std::endl;

	image90->~Image();
	image270->~Image();
	rotatedBitmap270->~Bitmap();
	croppedBitmap270->~Bitmap();
	rotatedBitmap90->~Bitmap();
	croppedBitmap90->~Bitmap();
	GdiplusShutdown(gdiplusToken);
}

