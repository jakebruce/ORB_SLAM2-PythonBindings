#define PY_ARRAY_UNIQUE_SYMBOL pbcvt_ARRAY_API
#include <opencv2/core/core.hpp>
#include <pyboostcvconverter/pyboostcvconverter.hpp>
#include <ORB_SLAM2/KeyFrame.h>
#include <ORB_SLAM2/Converter.h>
#include <ORB_SLAM2/Tracking.h>
#include "ORBSlamPython.h"

#if (PY_VERSION_HEX >= 0x03000000)
static void* init_ar() {
#else
static void init_ar() {
#endif
    Py_Initialize();

    import_array();
    return NUMPY_IMPORT_ARRAY_RETVAL;
}

BOOST_PYTHON_MODULE(orbslam2)
{
    init_ar();

    boost::python::to_python_converter<cv::Mat, pbcvt::matToNDArrayBoostConverter>();
    pbcvt::matFromNDArrayBoostConverter();

    boost::python::enum_<ORB_SLAM2::Tracking::eTrackingState>("TrackingState")
        .value("SYSTEM_NOT_READY", ORB_SLAM2::Tracking::eTrackingState::SYSTEM_NOT_READY)
        .value("NO_IMAGES_YET", ORB_SLAM2::Tracking::eTrackingState::NO_IMAGES_YET)
        .value("NOT_INITIALIZED", ORB_SLAM2::Tracking::eTrackingState::NOT_INITIALIZED)
        .value("OK", ORB_SLAM2::Tracking::eTrackingState::OK)
        .value("LOST", ORB_SLAM2::Tracking::eTrackingState::LOST);
    
    boost::python::enum_<ORB_SLAM2::System::eSensor>("Sensor")
        .value("MONOCULAR", ORB_SLAM2::System::eSensor::MONOCULAR)
        .value("STEREO", ORB_SLAM2::System::eSensor::STEREO)
        .value("RGBD", ORB_SLAM2::System::eSensor::RGBD);

    boost::python::class_<ORBSlamPython, boost::noncopyable>("System", boost::python::init<const char*, const char*, boost::python::optional<int, int, ORB_SLAM2::System::eSensor>>())
        .def(boost::python::init<std::string, std::string, boost::python::optional<int, int, ORB_SLAM2::System::eSensor>>())
        .def("initialize", &ORBSlamPython::initialize)
        .def("load_and_process_mono", &ORBSlamPython::loadAndProcessMono)
        .def("process_image_mono", &ORBSlamPython::processMono)
		.def("load_and_process_stereo", &ORBSlamPython::loadAndProcessStereo)
        .def("process_image_stereo", &ORBSlamPython::processStereo)
		.def("load_and_process_rgbd", &ORBSlamPython::loadAndProcessRGBD)
        .def("process_image_rgbd", &ORBSlamPython::processRGBD)
        .def("shutdown", &ORBSlamPython::shutdown)
        .def("is_running", &ORBSlamPython::isRunning)
        .def("reset", &ORBSlamPython::reset)
        .def("set_resolution", &ORBSlamPython::setResolution)
        .def("set_mode", &ORBSlamPython::setMode)
        .def("set_use_viewer", &ORBSlamPython::setUseViewer)
        .def("get_trajectory_points", &ORBSlamPython::getTrajectoryPoints)
        .def("get_tracking_state", &ORBSlamPython::getTrackingState)
        .def("save_settings", &ORBSlamPython::saveSettings)
        .def("load_settings", &ORBSlamPython::loadSettings)
        .def("save_settings_file", &ORBSlamPython::saveSettingsFile)
        .staticmethod("save_settings_file")
        .def("load_settings_file", &ORBSlamPython::loadSettingsFile)
        .staticmethod("load_settings_file");
}

ORBSlamPython::ORBSlamPython(std::string vocabFile, std::string settingsFile,
        int resolutionX, int resolutionY, ORB_SLAM2::System::eSensor sensorMode)
    : vocabluaryFile(vocabFile),
    settingsFile(settingsFile),
	sensorMode(sensorMode),
    system(nullptr),
    resolutionX(resolutionX),
    resolutionY(resolutionY),
    bUseViewer(false),
	bUseRGB(true)
{
    
}

ORBSlamPython::ORBSlamPython(const char* vocabFile, const char* settingsFile,
        int resolutionX, int resolutionY, ORB_SLAM2::System::eSensor sensorMode)
    : vocabluaryFile(vocabFile),
    settingsFile(settingsFile),
	sensorMode(sensorMode),
    system(nullptr),
    resolutionX(resolutionX),
    resolutionY(resolutionY),
    bUseViewer(false),
	bUseRGB(true)
{

}

ORBSlamPython::~ORBSlamPython()
{
}

bool ORBSlamPython::initialize()
{
    system = std::make_shared<ORB_SLAM2::System>(vocabluaryFile, settingsFile, sensorMode, bUseViewer);
    return true;
}

bool ORBSlamPython::isRunning()
{
    return system != nullptr;
}

void ORBSlamPython::reset()
{
    if (system)
    {
        system->Reset();
    }
}

bool ORBSlamPython::loadAndProcessMono(std::string imageFile, double timestamp)
{
    if (!system)
    {
        return false;
    }
    cv::Mat im = cv::imread(imageFile, cv::IMREAD_COLOR);
    if (bUseRGB) {
		cv::cvtColor(im, im, cv::COLOR_BGR2RGB);
	}
    return this->processMono(im, timestamp);
}

bool ORBSlamPython::processMono(cv::Mat image, double timestamp)
{
    if (!system)
    {
        return false;
    }
    if (image.data) {
		cv::Mat resizedImage;
        cv::resize(image, resizedImage, cv::Size(resolutionX, resolutionY));
        cv::Mat pose = system->TrackMonocular(resizedImage, timestamp);
        return !pose.empty();
    } else {
        return false;
    }
}

bool ORBSlamPython::loadAndProcessStereo(std::string leftImageFile, std::string rightImageFile, double timestamp)
{
    if (!system)
    {
        return false;
    }
    cv::Mat leftImage = cv::imread(leftImageFile, cv::IMREAD_COLOR);
	cv::Mat rightImage = cv::imread(rightImageFile, cv::IMREAD_COLOR);
	if (bUseRGB) {
		cv::cvtColor(leftImage, leftImage, cv::COLOR_BGR2RGB);
		cv::cvtColor(rightImage, rightImage, cv::COLOR_BGR2RGB);
	}
    return this->processStereo(leftImage, rightImage, timestamp);
}

bool ORBSlamPython::processStereo(cv::Mat leftImage, cv::Mat rightImage, double timestamp)
{
    if (!system)
    {
        return false;
    }
    if (leftImage.data && rightImage.data) {
		cv::Mat resizedLeftImage;
		cv::Mat resizedRightImage;
        cv::resize(leftImage, resizedLeftImage, cv::Size(resolutionX, resolutionY));
        cv::resize(rightImage, resizedRightImage, cv::Size(resolutionX, resolutionY));
        cv::Mat pose = system->TrackStereo(resizedLeftImage, resizedRightImage, timestamp);
        return !pose.empty();
    } else {
        return false;
    }
}

bool ORBSlamPython::loadAndProcessRGBD(std::string imageFile, std::string depthImageFile, double timestamp)
{
    if (!system)
    {
        return false;
    }
    cv::Mat im = cv::imread(imageFile, cv::IMREAD_COLOR);
	if (bUseRGB) {
		cv::cvtColor(im, im, cv::COLOR_BGR2RGB);
	}
    cv::Mat imDepth = cv::imread(depthImageFile, cv::IMREAD_UNCHANGED);
    return this->processRGBD(im, imDepth, timestamp);
}

bool ORBSlamPython::processRGBD(cv::Mat image, cv::Mat depthImage, double timestamp)
{
    if (!system)
    {
        return false;
    }
    if (image.data && depthImage.data) {
		cv::Mat resizedImage;
		cv::Mat resizedDepthImage;
        cv::resize(image, resizedImage, cv::Size(resolutionX, resolutionY));
        cv::resize(depthImage, resizedDepthImage, cv::Size(resolutionX, resolutionY));
        cv::Mat pose = system->TrackRGBD(resizedImage, resizedDepthImage, timestamp);
        return !pose.empty();
    } else {
        return false;
    }
}

void ORBSlamPython::shutdown()
{
    if (system)
    {
        system->Shutdown();
        system.reset();
    }
}

ORB_SLAM2::Tracking::eTrackingState ORBSlamPython::getTrackingState() const
{
    if (system) {
        return static_cast<ORB_SLAM2::Tracking::eTrackingState>(system->GetTrackingState());
    }
    return ORB_SLAM2::Tracking::eTrackingState::SYSTEM_NOT_READY;
}

boost::python::list ORBSlamPython::getTrajectoryPoints() const
{
    if (!system)
    {
        return boost::python::list();
    }
    
    // This is copied from the ORB_SLAM2 System.SaveTrajectoryTUM function, with some changes to output a python tuple.
    vector<ORB_SLAM2::KeyFrame*> vpKFs = system->GetKeyFrames();
    std::sort(vpKFs.begin(), vpKFs.end(), ORB_SLAM2::KeyFrame::lId);

    // Transform all keyframes so that the first keyframe is at the origin.
    // After a loop closure the first keyframe might not be at the origin.
    //cv::Mat Two = vpKFs[0]->GetPoseInverse();

    boost::python::list trajectory;

    for(size_t i=0; i<vpKFs.size(); i++)
    {
        ORB_SLAM2::KeyFrame* pKF = vpKFs[i];

       // pKF->SetPose(pKF->GetPose()*Two);

        if(pKF->isBad())
            continue;

        cv::Mat R = pKF->GetRotation().t();
        vector<float> q = ORB_SLAM2::Converter::toQuaternion(R);
        cv::Mat t = pKF->GetCameraCenter();
        
        trajectory.append(boost::python::make_tuple(
            pKF->mTimeStamp,    // Timestamp
            t.at<float>(0), t.at<float>(1), t.at<float>(2),  // Location
            q[0], q[1], q[2], q[3]  // Orientation
        ));

    }

    return trajectory;
}

void ORBSlamPython::setResolution(int x, int y)
{
    resolutionX = x;
    resolutionY = y;
}

void ORBSlamPython::setMode(ORB_SLAM2::System::eSensor mode)
{
    sensorMode = mode;
}

void ORBSlamPython::setUseViewer(bool useViewer)
{
    bUseViewer = useViewer;
}

void ORBSlamPython::setRGBMode(bool rgb)
{
    bUseRGB = rgb;
}

bool ORBSlamPython::saveSettings(boost::python::dict settings) const
{
    return ORBSlamPython::saveSettingsFile(settings, settingsFile);
}

boost::python::dict ORBSlamPython::loadSettings() const
{
    return ORBSlamPython::loadSettingsFile(settingsFile);
}

bool ORBSlamPython::saveSettingsFile(boost::python::dict settings, std::string settingsFilename)
{
    cv::FileStorage fs(settingsFilename.c_str(), cv::FileStorage::WRITE);
    
    boost::python::list keys = settings.keys();
    for (int index = 0; index < boost::python::len(keys); ++index)
    {
        boost::python::extract<std::string> extractedKey(keys[index]);
        if (!extractedKey.check())
        {
            continue;
        }
        std::string key = extractedKey;
        
        boost::python::extract<int> intValue(settings[key]);
        if (intValue.check())
        {
            fs << key << int(intValue);
            continue;
        }
        
        boost::python::extract<float> floatValue(settings[key]);
        if (floatValue.check())
        {
            fs << key << float(floatValue);
            continue;
        }
        
        boost::python::extract<std::string> stringValue(settings[key]);
        if (stringValue.check())
        {
            fs << key << std::string(stringValue);
            continue;
        }
    }
    
    return true;
}

// Helpers for reading cv::FileNode objects into python objects.
boost::python::list readSequence(cv::FileNode fn, int depth=10);
boost::python::dict readMap(cv::FileNode fn, int depth=10);

boost::python::dict ORBSlamPython::loadSettingsFile(std::string settingsFilename)
{
    cv::FileStorage fs(settingsFilename.c_str(), cv::FileStorage::READ);
    cv::FileNode root = fs.root();
    if (root.isMap()) 
    {
        return readMap(root);
    }
    else if (root.isSeq())
    {
        boost::python::dict settings;
        settings["root"] = readSequence(root);
        return settings;
    }
    return boost::python::dict();
}


// ----------- HELPER DEFINITIONS -----------
boost::python::dict readMap(cv::FileNode fn, int depth)
{
    boost::python::dict map;
    if (fn.isMap()) {
        cv::FileNodeIterator it = fn.begin(), itEnd = fn.end();
        for (; it != itEnd; ++it) {
            cv::FileNode item = *it;
            std::string key = item.name();
            
            if (item.isNone())
            {
                map[key] = boost::python::object();
            }
            else if (item.isInt())
            {
                map[key] = int(item);
            }
            else if (item.isString())
            {
                map[key] = std::string(item);
            }
            else if (item.isReal())
            {
                map[key] = float(item);
            }
            else if (item.isSeq() && depth > 0)
            {
                map[key] = readSequence(item, depth-1);
            }
            else if (item.isMap() && depth > 0)
            {
                map[key] = readMap(item, depth-1);  // Depth-limited recursive call to read inner maps
            }
        }
    }
    return map;
}

boost::python::list readSequence(cv::FileNode fn, int depth)
{
    boost::python::list sequence;
    if (fn.isSeq()) {
        cv::FileNodeIterator it = fn.begin(), itEnd = fn.end();
        for (; it != itEnd; ++it) {
            cv::FileNode item = *it;
            
            if (item.isNone())
            {
                sequence.append(boost::python::object());
            }
            else if (item.isInt())
            {
                sequence.append(int(item));
            }
            else if (item.isString())
            {
                sequence.append(std::string(item));
            }
            else if (item.isReal())
            {
                sequence.append(float(item));
            }
            else if (item.isSeq() && depth > 0)
            {
                sequence.append(readSequence(item, depth-1)); // Depth-limited recursive call to read nested sequences
            }
            else if (item.isMap() && depth > 0)
            {
                sequence.append(readMap(item, depth-1));
            }
        }
    }
    return sequence;
}
