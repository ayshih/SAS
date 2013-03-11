#include "ImperxStream.hpp"
#include <iostream>

ImperxStream::ImperxStream()
    : lStream()
    , lPipeline( &lStream )
{
    lDeviceInfo = NULL;
    lDeviceParams = NULL;
    lStreamParams = NULL;
}

ImperxStream::~ImperxStream()
{
    Stop();
    Disconnect();
}

int ImperxStream::Connect()
{
    PvResult lResult;	

    // Create an GEV system and an interface.
    PvSystem lSystem;

    // Find all GEV Devices on the network.
    lSystem.SetDetectionTimeout( 2000 );
    lResult = lSystem.Find();
    if( !lResult.IsOK() )
    {
	printf( "PvSystem::Find Error: %s", lResult.GetCodeString().GetAscii() );
	return -1;
    }

    // Get the number of GEV Interfaces that were found using GetInterfaceCount.
    PvUInt32 lInterfaceCount = lSystem.GetInterfaceCount();

    // Display information about all found interface
    // For each interface, display information about all devices.
    for( PvUInt32 x = 0; x < lInterfaceCount; x++ )
    {
	// get pointer to each of interface
	PvInterface * lInterface = lSystem.GetInterface( x );

	printf( "Interface %i\nMAC Address: %s\nIP Address: %s\nSubnet Mask: %s\n\n",
		x,
		lInterface->GetMACAddress().GetAscii(),
		lInterface->GetIPAddress().GetAscii(),
		lInterface->GetSubnetMask().GetAscii() );

	// Get the number of GEV devices that were found using GetDeviceCount.
	PvUInt32 lDeviceCount = lInterface->GetDeviceCount();

	for( PvUInt32 y = 0; y < lDeviceCount ; y++ )
	{
	    lDeviceInfo = lInterface->GetDeviceInfo( y );
	    printf( "Device %i\nMAC Address: %s\nIP Address: %s\nSerial number: %s\n\n",
		    y,
		    lDeviceInfo->GetMACAddress().GetAscii(),
		    lDeviceInfo->GetIPAddress().GetAscii(),
		    lDeviceInfo->GetSerialNumber().GetAscii() );
	}
    }

    // Connect to the last GEV Device found.
    if( lDeviceInfo != NULL )
    {
	printf( "Connecting to %s\n",
		lDeviceInfo->GetMACAddress().GetAscii() );

	lResult = lDevice.Connect( lDeviceInfo );
	if ( !lResult.IsOK() )
	{
	    printf( "Unable to connect to %s\n", 
		    lDeviceInfo->GetMACAddress().GetAscii() );
	    return -1;
	}
	else
	{
	    printf( "Successfully connected to %s\n", 
		    lDeviceInfo->GetMACAddress().GetAscii() );
	}
    }
    else
    {
	printf( "No device found\n" );
	return -1;
    }

    // Get device parameters need to control streaming
    lDeviceParams = lDevice.GetGenParameters();

    return 0;
}

int ImperxStream::Connect(const std::string &IP)
{
    PvResult lResult;
    
    // Create an GEV system and an interface.
    PvSystem lSystem;

    // Find all GEV Devices on the network.
    lSystem.SetDetectionTimeout( 2000 );
    lResult = lSystem.Find();
    if( !lResult.IsOK() )
    {
	//Failed to find PvAnything
	printf( "PvSystem::Find Error: %s", lResult.GetCodeString().GetAscii() );
	return -1;
    }

    // Get the number of GEV Interfaces that were found using GetInterfaceCount.
    PvUInt32 lInterfaceCount = lSystem.GetInterfaceCount();

    // Search through interfaces for any devices
    // Check devices for correct target IP
    for( PvUInt32 x = 0; x < lInterfaceCount; x++ )
    {
	// get pointer to each of interface
	PvInterface * lInterface = lSystem.GetInterface( x );

	// Get the number of GEV devices that were found using GetDeviceCount.
	PvUInt32 lDeviceCount = lInterface->GetDeviceCount();

	for( PvUInt32 y = 0; y < lDeviceCount ; y++ )
	{
	    PvDeviceInfo *tDeviceInfo = lInterface->GetDeviceInfo( y );
	    std::string laddress(tDeviceInfo->GetIPAddress().GetAscii());
	    if (!laddress.compare(IP))
	    {
		lDeviceInfo = tDeviceInfo;
		printf( "Interface %i\nMAC Address: %s\nIP Address: %s\nSubnet Mask: %s\n\n",
			x,
			lInterface->GetMACAddress().GetAscii(),
			lInterface->GetIPAddress().GetAscii(),
			lInterface->GetSubnetMask().GetAscii() );
		printf( "Device %i\nMAC Address: %s\nIP Address: %s\nSerial number: %s\n\n",
			y,
			tDeviceInfo->GetMACAddress().GetAscii(),
			tDeviceInfo->GetIPAddress().GetAscii(),
			tDeviceInfo->GetSerialNumber().GetAscii() );
	    }
	}
    }

    // Connect to the last GEV Device found.
    if( lDeviceInfo != NULL )
    {
	printf( "Connecting to %s\n",
		lDeviceInfo->GetMACAddress().GetAscii() );

	lResult = lDevice.Connect( lDeviceInfo );
	if ( !lResult.IsOK() )
	{
	    printf( "Unable to connect to %s\n", 
		    lDeviceInfo->GetMACAddress().GetAscii() );
	    return -1;
	}
	else
	{
	    printf( "Successfully connected to %s\n", 
		    lDeviceInfo->GetMACAddress().GetAscii() );
	}
    }
    else
    {
	printf( "No device found\n" );
	return -1;
    }
    // Get device parameters need to control streaming
    lDeviceParams = lDevice.GetGenParameters();

    return 0;
}

int ImperxStream::Initialize()
{
    if(lDeviceInfo == NULL)
    {
	std::cout << "No device connected!\n";
	return -1;
    }

    // Negotiate streaming packet size
    lDevice.NegotiatePacketSize();

    // Open stream - have the PvDevice do it for us
    printf( "Opening stream to device\n" );
    lStream.Open( lDeviceInfo->GetIPAddress() );

    // Reading payload size from device
    PvInt64 lSize = 0;
    lDeviceParams->GetIntegerValue( "PayloadSize", lSize );

    // Set the Buffer size and the Buffer count
    lPipeline.SetBufferSize( static_cast<PvUInt32>( lSize ) );
    lPipeline.SetBufferCount( 16 ); // Increase for high frame rate without missing block IDs

    // Have to set the Device IP destination to the Stream
    lDevice.SetStreamDestination( lStream.GetLocalIPAddress(), lStream.GetLocalPort() ); 
    // IMPORTANT: the pipeline needs to be "armed", or started before 
    // we instruct the device to send us images
    printf( "Starting pipeline\n" );
    lPipeline.Start();

    // Get stream parameters/stats
    lStreamParams = lStream.GetParameters();

    // TLParamsLocked is optional but when present, it MUST be set to 1
    // before sending the AcquisitionStart command
    lDeviceParams->SetIntegerValue( "TLParamsLocked", 1 );

    printf( "Resetting timestamp counter...\n" );
    lDeviceParams->ExecuteCommand( "GevTimestampControlReset" );
    return 0;
}
    
void ImperxStream::Snap(cv::Mat &frame)
{
    // The pipeline is already "armed", we just have to tell the device
    // to start sending us images
    lDeviceParams->ExecuteCommand( "AcquisitionStart" );
    int lWidth, lHeight;
    // Retrieve next buffer		
    PvBuffer *lBuffer = NULL;
    PvResult lOperationResult;
    PvResult lResult = lPipeline.RetrieveNextBuffer( &lBuffer, 1000, &lOperationResult );
        
    if ( lResult.IsOK() )
    {
	if ( lOperationResult.IsOK() )
	{
	    // Process Buffer
            
	    if ( lBuffer->GetPayloadType() == PvPayloadTypeImage )
	    {
		// Get image specific buffer interface
		PvImage *lImage = lBuffer->GetImage();
	      
		// Read width, height
		lWidth = (int) lImage->GetWidth();
		lHeight = (int) lImage->GetHeight();
		unsigned char *img = lImage->GetDataPointer();
//		cv::Mat lframe(lHeight,lWidth,CV_8UC1,img, cv::Mat::AUTO_STEP);
//		lframe.copyTo(frame);
		for (int m = 0; m < lHeight; m++)
		{
		    for (int n = 0; n < lWidth; n++)
		    {
			frame.at<unsigned char>(m,n) = img[m*lWidth + n];
//			std::cout << (short int) img[n*lHeight +m] << " ";
		    }
		}
	    }
	    else
	    {
		std::cout << "No image\n";
	    }
	}
	else
	{
	    std::cout << "Damaged Result\n";
	}
	// We have an image - do some processing (...) and VERY IMPORTANT,
	// release the buffer back to the pipeline

	lPipeline.ReleaseBuffer( lBuffer );
    }
    else
    {
	std::cout << "Timeout\n";
    }
}


long long int ImperxStream::getTemperature()
{		
    long long int lTempValue = 0.0;
    lDevice.GetGenParameters()->GetIntegerValue( "CurrentTemperature", lTempValue );
	
    return lTempValue;	
}


void ImperxStream::Stop()
{
    if (lDeviceParams != NULL)
    {
	// Tell the device to stop sending images
	std::cout << "Stop: Send AcquisitionStop\n";
	lDeviceParams->ExecuteCommand( "AcquisitionStop" );
    
	// If present reset TLParamsLocked to 0. Must be done AFTER the 
	// streaming has been stopped
	std::cout << "Stop: set TLParamsLocked to 0\n";
	lDeviceParams->SetIntegerValue( "TLParamsLocked", 0 );
    }

    // We stop the pipeline - letting the object lapse out of 
    // scope would have had the destructor do the same, but we do it anyway    
    if(lPipeline.IsStarted())
    {
	std::cout << "Stop: Stop pipeline\n";
	lPipeline.Stop();
    }

    // Now close the stream. Also optionnal but nice to have
    
    if(lStream.IsOpen())
    {
	std::cout << "Stop: Closing stream\n";
	lStream.Close();
    }
}

void ImperxStream::Disconnect()
{
    if(lDevice.IsConnected())
    {
	printf( "Disconnecting device\n" );
	lDevice.Disconnect();
    }
}

void ImperxStream::ConfigureSnap()
{
    lDeviceParams->SetEnumValue("AcquisitionMode","SingleFrame");
    lDeviceParams->SetEnumValue("ExposureMode","Timed");
    lDeviceParams->SetEnumValue("PixelFormat","Mono8");    
}

int ImperxStream::SetExposure(int exposureTime)
{
    PvResult outcome;
    if (exposureTime >= 5 && exposureTime <= 38221)
    {
	outcome = lDeviceParams->SetIntegerValue("ExposureTimeRaw",exposureTime);
	if (outcome.IsSuccess())
	{
	    return 0;
	}
    }
    return -1;
}

int ImperxStream::SetROISize(cv::Size size)
{
    return SetROISize(size.width, size.height);
}

int ImperxStream::SetROISize(int width, int height)
{
    int outcomes[2];
    outcomes[0] = SetROIHeight(height);
    outcomes[1] = SetROIWidth(width);   
    if (outcomes[0] == 0 && outcomes[1] == 0)
    {
	return 0;
    }
    return -1;
}

int ImperxStream::SetROIHeight(int height)
{
    PvResult outcome;
    if (height >= 1 && height <= 966)
    {
	outcome = lDeviceParams->SetIntegerValue("Height", height);
	if (outcome.IsSuccess())
	{
	    return 0;
	}
    }
    return -1;
}
    
int ImperxStream::SetROIWidth(int width)
{
    PvResult outcome;
    if (width >= 8 && width <= 1296 && (width % 8) == 0)
    {
	outcome = lDeviceParams->SetIntegerValue("Width", width);
	if (outcome.IsSuccess())
	{
	    return 0;
	}
    }
    return -1;   
}

int ImperxStream::SetROIOffset(cv::Point offset)
{
    return SetROIOffset(offset.x, offset.y);
}

int ImperxStream::SetROIOffset(int x, int y)
{
    int outcomes[2];
    outcomes[0] = SetROIOffsetX(x);
    outcomes[1] = SetROIOffsetY(y);   
    if (outcomes[0] == 0 && outcomes[1] == 0)
    {
	return 0;
    }
    else
    {
	return -1;
    }
}

int ImperxStream::SetROIOffsetX(int x)
{
    PvResult outcome;
    if (x >= 0 && x <= 965)
    {
	outcome = lDeviceParams->SetIntegerValue("OffsetX", x);
	if (outcome.IsSuccess())
	{
	    return 0;
	}
    }
    return -1;
}

int ImperxStream::SetROIOffsetY(int y)
{
    PvResult outcome;
    if (y >= 0 && y <= 1295)
    {
	outcome = lDeviceParams->SetIntegerValue("OffsetY", y);
	if (outcome.IsSuccess())
	{
	    return 0;
	}
    }
    return -1;
}

int ImperxStream::GetExposure()
{
    PvInt64 exposure;
    lDeviceParams->GetIntegerValue("ExposureTimeRaw", exposure);
    return (int) exposure;
}

cv::Size ImperxStream::GetROISize()
{
    int width, height;
    width = GetROIWidth();
    height = GetROIHeight();
    return cv::Size(width, height);
}

cv::Point ImperxStream::GetROIOffset()
{
    int x, y;
    x = GetROIOffsetX();
    y = GetROIOffsetY();
    return cv::Point(x,y);
}

int ImperxStream::GetROIHeight()
{
    PvInt64 height;
    lDeviceParams->GetIntegerValue("Height", height);
    return (int) height;
}

int ImperxStream::GetROIWidth()
{
    PvInt64 width;
    lDeviceParams->GetIntegerValue("Width", width);
    return (int) width;
}

int ImperxStream::GetROIOffsetX()
{
    PvInt64 x;
    lDeviceParams->GetIntegerValue("OffsetX", x);
    return (int) x;
}

int ImperxStream::GetROIOffsetY()
{
    PvInt64 y;
    lDeviceParams->GetIntegerValue("OffsetY", y);
    return (int) y;
}
