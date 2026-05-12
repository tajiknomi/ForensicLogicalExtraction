#pragma once

#include "../backend/include/extractor.h"
#include "../backend/include/adb.h"
#include <msclr/marshal_cppstd.h>

using namespace System;

namespace Wrapper {

    public ref class ForensicExtractorWrapper
    {
    private:
        ADB* adb;
        ForensicExtractor* extractor;

    public:
        // Constructor
        ForensicExtractorWrapper(String^ adbPath);

        // Destructor
        ~ForensicExtractorWrapper();

        // Finalizer
        !ForensicExtractorWrapper();

        // Methods
        void ShowConnectedDevices();
        int ExtractDeviceInfo(String^ outputFileName);
        int ExtractUserInstalledAppsList(String^ outputFileName);
        int ExtractSMS(String^ outputFileName);
        int ExtractCallLogs(String^ outputFileName);
        int PullMedia(String^ dirName);
        int ExtractMediaStoreDb(String^ outputFileName);
        int ExtractCalendarEntities(String^ outputFileName);
        int NumOfConnectedAdbDevices();
    };

} // namespace Wrapper