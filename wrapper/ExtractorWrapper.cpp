#include "pch.h"                // PCH first
#include "ExtractorWrapper.h"

using namespace msclr::interop;

namespace Wrapper {

    // Constructor
    ForensicExtractorWrapper::ForensicExtractorWrapper(String^ adbPath)
    {
        std::wstring nativePath = marshal_as<std::wstring>(adbPath);
        adb = new ADB(nativePath);
        extractor = new ForensicExtractor(*adb);
    }

    // Destructor
    ForensicExtractorWrapper::~ForensicExtractorWrapper()
    {
        this->!ForensicExtractorWrapper();
    }

    // Finalizer
    ForensicExtractorWrapper::!ForensicExtractorWrapper()
    {
        if (extractor) { delete extractor; extractor = nullptr; }
        if (adb) { delete adb; adb = nullptr; }
    }

    // Methods
    void ForensicExtractorWrapper::ShowConnectedDevices()
    {
        extractor->showConnectedDevices();
    }

    int ForensicExtractorWrapper::ExtractDeviceInfo(String^ outputFileName)
    {
        std::wstring nativeFile = marshal_as<std::wstring>(outputFileName);
        return extractor->extractDeviceInfo(nativeFile);
    }

    int ForensicExtractorWrapper::ExtractUserInstalledAppsList(String^ outputFileName)
    {
        std::wstring nativeFile = marshal_as<std::wstring>(outputFileName);
        return extractor->extractUserInstalledAppsList(nativeFile);
    }

    int ForensicExtractorWrapper::ExtractSMS(String^ outputFileName)
    {
        std::wstring nativeFile = marshal_as<std::wstring>(outputFileName);
        return extractor->extractSMS(nativeFile);
    }

    int ForensicExtractorWrapper::ExtractCallLogs(String^ outputFileName)
    {
        std::wstring nativeFile = marshal_as<std::wstring>(outputFileName);
        return extractor->extractCallLogs(nativeFile);
    }

    int ForensicExtractorWrapper::PullMedia(String^ dirName)
    {
        std::wstring nativeDir = marshal_as<std::wstring>(dirName);
        return extractor->pullMedia(nativeDir);
    }

    int ForensicExtractorWrapper::ExtractMediaStoreDb(String^ outputFileName)
    {
        std::wstring nativeFile = marshal_as<std::wstring>(outputFileName);
        return extractor->extractMediaStoreDb(nativeFile);
    }

    int ForensicExtractorWrapper::ExtractCalendarEntities(String^ outputFileName)
    {
        std::wstring nativeFile = marshal_as<std::wstring>(outputFileName);
        return extractor->extractCalendarEntities(nativeFile);
    }

    int ForensicExtractorWrapper::NumOfConnectedAdbDevices()
    {
        return extractor->numOfConnectedAdbDevices();
    }

} // namespace Wrapper