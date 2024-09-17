#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <tuple>

// Function to execute a command and get the output
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// Function to list available cameras
std::vector<std::pair<int, std::string>> listCameras() {
    std::vector<std::pair<int, std::string>> availableCameras;
    for (int i = 0; i < 10; ++i) { // Check the first 10 indices
        cv::VideoCapture cap(i);
        if (cap.isOpened()) {
            std::string cmd = "v4l2-ctl --device=/dev/video" + std::to_string(i) + " --all | grep 'Card type'";
            std::string deviceName = exec(cmd.c_str());
            deviceName = deviceName.substr(deviceName.find(":") + 1);
            deviceName = deviceName.substr(0, deviceName.find("\n"));
            availableCameras.push_back({i, deviceName});
            
            cap.release();
        }
    }
    return availableCameras;
}

// Function to list supported formats, resolutions, and frame rates
std::vector<std::tuple<std::string, std::string, std::string>> listSupportedFormats(int cameraIndex) {
    std::string cmd = "v4l2-ctl --device=/dev/video" + std::to_string(cameraIndex) + " --list-formats-ext";
    std::string output = exec(cmd.c_str());
    std::vector<std::tuple<std::string, std::string, std::string>> formats;

    std::istringstream iss(output);
    std::string line;
    std::string format, resolution;
    int format_count = 0;
    std::string shit = "["+ std::to_string(format_count) + "]: ";
    while (std::getline(iss, line)) {
        std::cout << line << std::endl;

        if (line.find(shit) != std::string::npos) {
            format = line.substr(line.find("'") + 1, 4);
            format_count++;
            shit = "["+ std::to_string(format_count) + "]: ";
        } else if (line.find("Size: Discrete") != std::string::npos) {
            resolution = line.substr(line.find(":") + 10);
        } else if (line.find("Interval") != std::string::npos) {
            std::string frameRate = line.substr(line.find("(") + 1);
            frameRate = frameRate.substr(0, frameRate.find("."));
            formats.push_back({format, resolution, frameRate});
        }
    }
    return formats;
}

int main() {
    // List available cameras
    std::vector<std::pair<int, std::string>> cameras = listCameras();
    if (cameras.empty()) {
        std::cerr << "No cameras found" << std::endl;
        return -1;
    }

    std::cout << "Available cameras:" << std::endl;
    for (int i = 0; i < cameras.size(); ++i) {
        std::cout << i << ": /dev/video " << cameras[i].first << " - " << cameras[i].second << std::endl;
    }

    // Select a camera
    int cameraIndex;
    std::cout << "Select a camera index: ";
    std::cin >> cameraIndex;

    // std::string
    if (cameraIndex < 0 || cameraIndex >= cameras.size()) {
        std::cerr << "Invalid camera index" << std::endl;
        return -1;
    }

    // List supported formats, resolutions, and frame rates
    std::vector<std::tuple<std::string, std::string, std::string>> formats = listSupportedFormats(cameras[cameraIndex].first);
    if (formats.empty()) {
        std::cerr << "No supported formats found" << std::endl;
        return -1;
    }

    std::cout << "Supported formats:" << std::endl;
    
    for (int i = 0; i < formats.size(); ++i) 
        std::cout << i << ": Format: " << std::get<0>(formats[i]) << " - " << std::get<1>(formats[i]) << " - " << std::get<2>(formats[i]) << " FPS" << std::endl;
    

    // Select a format
    int formatIndex;
    std::cout << "Select a format index: ";
    std::cin >> formatIndex;
    std::stringstream s1;
    s1 << "/dev/video " << cameras[cameraIndex].first << " - " << cameras[cameraIndex].second << " - Format: " << std::get<0>(formats[formatIndex]) << " - " << std::get<1>(formats[formatIndex]) << " - " << std::get<2>(formats[formatIndex]) << " FPS";


    if (formatIndex < 0 || formatIndex >= formats.size()) {
        std::cerr << "Invalid format index" << std::endl;
        return -1;
    }

    // Parse the selected format, resolution, and frame rate
    std::string format = std::get<0>(formats[formatIndex]);
    std::istringstream resStream(std::get<1>(formats[formatIndex]));
    int width, height;
    char x;
    resStream >> width >> x >> height;

    std::istringstream fpsStream(std::get<2>(formats[formatIndex]));
    double fps;
    fpsStream >> fps;

    // Open the selected camera
    cv::VideoCapture cap(cameras[cameraIndex].first);

    // Check if the camera opened successfully
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open camera" << std::endl;
        return -1;
    }

    // Set the desired format, resolution, and frame rate
    cap.set(cv::CAP_PROP_FOURCC, cv::VideoWriter::fourcc(format[0], format[1], format[2], format[3]));
    cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    cap.set(cv::CAP_PROP_FPS, fps);

    // Create a window to display the video
    std::string window_name = s1.str();
    cv::namedWindow(window_name, cv::WINDOW_FREERATIO);
    cv::resizeWindow(window_name, width, height); // Resize the window to match the selected resolution

    while (true) {
        cv::Mat frame;
        // Capture a new frame from the camera
        cap >> frame;

        // Check if the frame is empty
        if (frame.empty()) {
            std::cerr << "Error: Empty frame" << std::endl;
            break;
        }

        // Display the frame in the window
        cv::imshow(window_name, frame);

        if (cv::waitKey(10) == 'q') {
            break;
        }
    }
    
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
