#include <node_api.h> 
#include <napi.h>

#include <assert.h>
#include <stdio.h> 
#include <stdlib.h>
#include <chrono>
#include <thread>
#include <vector>

#include <sl/Camera.hpp>

/* 
	Built against ZED SDK 3.5.0
	+ CUDA 11.0
*/


		// sl::Camera zed;

		// ERROR_CODE returned_state = zed.open();
		// if (returned_state != ERROR_CODE::SUCCESS) {
		// 	throw Napi::Error::New(env, "Error opening camera");
		// }
		// // Get camera information (ZED serial number)
		// auto camera_infos = zed.getCameraInformation();
		// printf("Hello! This is my serial number: %d\n", camera_infos.serial_number);
		// zed.close();

		// if (ma_context_init(NULL, 0, NULL, &state.context) != MA_SUCCESS) {
		// 	// Error.
		// 	throw Napi::Error::New(env, "Audio Init exception");
		// }

struct Camera : public Napi::ObjectWrap<Camera> {

	sl::Camera zed;
	sl::SensorsData sensors_data;
	sl::RuntimeParameters runtime_parameters;
	// .getWidth(), .getHeight(), .getResolution(), .getChannels()
	// .getDataType(), .getMemoryType() (CPU or GPU), .getPtr()
	// sl::Mat left;
	// sl::Mat depth;
	// The point cloud stores its data on 4 channels using 32-bit float for each channel. The last float is used to store color information, where R, G, B, and alpha channels (4 x 8-bit) are concatenated into a single 32-bit float. 
	sl::Mat cloud;
	// The output is a 4 channels 32-bit matrix (X,Y,Z,empty), where X,Y,Z values encode the direction of the normal vectors.
	sl::Mat normals;

	sl::float3 acceleration;

	//sl::Mat confidence_map;
	sl::Resolution capture_res;
	uint64_t ms = 0;

    Camera(const Napi::CallbackInfo& info) : Napi::ObjectWrap<Camera>(info) {
		Napi::Env env = info.Env();

		if (info.Length()) open(info);
	}

	Napi::Value open(const Napi::CallbackInfo& info) {
		Napi::Env env = info.Env();

		// expect some configuration here
		const Napi::Object options = info.Length() ? info[0].ToObject() : Napi::Object::New(env);

		// TODO configure from options:
		// https://www.stereolabs.com/docs/api/structsl_1_1InitParameters.html
		sl::InitParameters init_parameters;
		init_parameters.camera_resolution = sl::RESOLUTION::HD720;
		init_parameters.camera_fps = 30;
		init_parameters.depth_mode = sl::DEPTH_MODE::NEURAL; // PERFORMANCE, QUALITY, ULTRA, or NEURAL
		init_parameters.coordinate_units = sl::UNIT::METER;
		init_parameters.depth_minimum_distance = 0.3;
		init_parameters.depth_maximum_distance = 8.0;
		init_parameters.depth_stabilization = 50; // 50%
		init_parameters.coordinate_system = sl::COORDINATE_SYSTEM::RIGHT_HANDED_Y_UP;
		//init_parameters.sdk_verbose = true; // Enable verbose logging
		init_parameters.async_grab_camera_recovery = true;
		// int 	camera_image_flip
		// bool 	camera_disable_self_calib
		// bool 	enable_right_side_measure
		// bool 	svo_real_time_mode
		// CUdevice 	sdk_gpu_id
		// int 	sdk_verbose
		// String 	sdk_verbose_log_file
		// CUcontext 	sdk_cuda_ctx
		// InputType 	input
		// String 	optional_settings_path
		// String 	optional_opencv_calibration_file
		// bool 	sensors_required
		// bool 	enable_image_enhancement
		// float 	open_timeout_sec
		// bool 	async_grab_camera_recovery
		// float 	grab_compute_capping_fps

		//https://www.stereolabs.com/docs/depth-sensing/depth-settings/
		//https://www.stereolabs.com/docs/api/structsl_1_1RuntimeParameters.html#a97bb28af0c7ce0abacd621910cae8c44
		// REFERENCE_FRAME 	measure3D_reference_frame
		runtime_parameters.enable_depth = true;
		runtime_parameters.confidence_threshold = 5;
		runtime_parameters.texture_confidence_threshold = 95;
		runtime_parameters.remove_saturated_areas = true;
		runtime_parameters.enable_fill_mode = false;	
		
		// from SVO file:
		//init_parameters.input.setFromSVOFile(input_path);
		// from IP stream:
		// ..
		if (options.Has("serial")) {
			// try to open device by serial
			uint32_t z = options.Get("serial").ToNumber().Uint32Value();
			init_parameters.input.setFromSerialNumber(z);
		} else if (options.Has("id")) {
			// try to open device by id
			uint32_t z = options.Get("id").ToNumber().Uint32Value();
			init_parameters.input.setFromCameraID(z);
		} 

		sl::ERROR_CODE err = zed.open(init_parameters);
		if (err != sl::ERROR_CODE::SUCCESS) {
			 throw Napi::Error::New(env, sl::toString(err).get());
		}

		auto cam_info = zed.getCameraInformation();
		//cout << cam_info.camera_model << ", ID: " << z << ", SN: " << cam_info.serial_number << " Opened" << endl;

		Napi::Object This = info.This().As<Napi::Object>();
		This.Set("serial", cam_info.serial_number);
		This.Set("model", sl::toString(cam_info.camera_model).get());
		This.Set("input_type", sl::toString(cam_info.input_type).get());

		// has calibration parameters, firmware, fps, resolution:
		//This.Set("camera_configuration", sl::toString(cam_info.camera_configuration).get());
		This.Set("firmware_version", cam_info.camera_configuration.firmware_version);
		This.Set("fps", cam_info.camera_configuration.fps);
		
		auto resolution = cam_info.camera_configuration.resolution;
		This.Set("width", resolution.width);
		This.Set("height", resolution.height);

		size_t subdiv = 1;
		capture_res.width = resolution.width / subdiv;
		capture_res.height = resolution.height / subdiv;

		// CalibrationParameters calibration_params = zed.getCameraInformation()->calibration_parameters;
		// // Focal length of the left eye in pixels
		// float focal_left_x = calibration_params.left_cam.fx;
		// // First radial distortion coefficient
		// float k1 = calibration_params.left_cam.disto[0];
		// // Translation between left and right eye on z-axis
		// float tz = calibration_params.T.z;
		// // Horizontal field of view of the left eye in degrees
		// float h_fov = calibration_params.left_cam.h_fov;

		// has firmware, imu, barometer, magnetometer etc.
		//This.Set("sensors_configuration", sl::toString(cam_info.sensors_configuration).get());

		// camera settings:
		// Set exposure to 50% of camera framerate
		// zed.setCameraSettings(VIDEO_SETTINGS::EXPOSURE, 50);
		// // Set white balance to 4600K
		// zed.setCameraSettings(VIDEO_SETTINGS::WHITE_BALANCE, 4600);
		// // Reset to auto exposure
		// zed.setCameraSettings(VIDEO_SETTINGS::EXPOSURE, VIDEO_SETTINGS_VALUE_AUTO);

		// allocate some memory to store the Left/Depth images
		cloud.alloc(capture_res, sl::MAT_TYPE::F32_C4);
		This.Set("cloud", Napi::TypedArrayOf<float>::New(env, 
			4 * capture_res.width * capture_res.height,
			Napi::ArrayBuffer::New(env, cloud.getPtr<float>(), cloud.getStepBytes() * cloud.getHeight()),
			0,
			napi_float32_array)
		);

		normals.alloc(capture_res, sl::MAT_TYPE::F32_C4);
		This.Set("normals", Napi::TypedArrayOf<float>::New(env, 
			4 * capture_res.width * capture_res.height,
			Napi::ArrayBuffer::New(env, normals.getPtr<float>(), normals.getStepBytes() * normals.getHeight()),
			0,
			napi_float32_array)
		);

		This.Set("acceleration", Napi::TypedArrayOf<float>::New(env, 
			3,
			Napi::ArrayBuffer::New(env, &acceleration.x, 3 * 4),
			0,
			napi_float32_array)
		);

		//confidence_map.alloc(capture_res, sl::MAT_TYPE::8U_C4);
		// This.Set("confidence", Napi::TypedArrayOf<uint8_t>::New(env, 
		// 	4 * capture_res.width * capture_res.height,
		// 	Napi::ArrayBuffer::New(env, confidence_map.getPtr<uint8_t>(), confidence_map.getStepBytes() * confidence_map.getHeight()),
		// 	0,
		// 	napi_uint8_array)
		// );
		return This;
	}

	~Camera() {
		zed_close();
		std::cout << "~MyObject"  << std::endl;
	}

	void zed_close() {
		// join thread
		// release memory

		zed.close();
	}

	Napi::Value close(const Napi::CallbackInfo& info) {
		zed_close();
		return info.This();
	}

	Napi::Value isOpened(const Napi::CallbackInfo& info) {
		Napi::Env env = info.Env();
		return Napi::Boolean::New(env, zed.isOpened());
	}

	// convert to a thread fun
	Napi::Value grab(const Napi::CallbackInfo& info) {
		Napi::Env env = info.Env();
		if (!zed.isOpened()) return env.Null();

		auto err = zed.grab(runtime_parameters);
		if (err == sl::ERROR_CODE::END_OF_SVOFILE_REACHED) zed.setSVOPosition(0);
		if (err != sl::ERROR_CODE::SUCCESS) return info.Env().Null();

		Napi::Object This = info.This().As<Napi::Object>();
		
		if (info.Length()) {
			const Napi::Object options = info[0].ToObject();
			// modify runtime_parameters here
			

			//https://www.stereolabs.com/docs/depth-sensing/depth-settings/
			//https://www.stereolabs.com/docs/api/structsl_1_1RuntimeParameters.html#a97bb28af0c7ce0abacd621910cae8c44
			// REFERENCE_FRAME 	measure3D_reference_frame
			runtime_parameters.enable_depth = true;
			runtime_parameters.confidence_threshold = 5;
			runtime_parameters.texture_confidence_threshold = 95;
			runtime_parameters.remove_saturated_areas = true;
			runtime_parameters.enable_fill_mode = false;	
		}

		
		// Extract multi-sensor data
		zed.getSensorsData(sensors_data, sl::TIME_REFERENCE::IMAGE); // Get frame synchronized sensor data
		acceleration = sensors_data.imu.linear_acceleration;

		//This.Set("acceleration", )

		//sl::float3 angular_velocity = imu_data.angular_velocity;
		// auto barometer_data = sensors_data.barometer;
		// auto magnetometer_data = sensors_data.magnetometer;
		// Retrieve linear acceleration and angular velocity
		
		// Retrieve pressure and relative altitude
		// float pressure = barometer_data.pressure;
		// float relative_altitude = barometer_data.relative_altitude;
		// Retrieve magnetic field
    	//float3 magnetic_field = magnetometer_data.magnetic_field_uncalibrated;

		// could go straight to GPU here
		// optional resolution argument
		//zed.retrieveImage(left, sl::VIEW::LEFT); //sl::VIEW::DEPTH);
		//zed.retrieveMeasure(depth, sl::MEASURE::DEPTH); 
		zed.retrieveMeasure(cloud, sl::MEASURE::XYZRGBA, sl::MEM::CPU, capture_res);
		zed.retrieveMeasure(normals, sl::MEASURE::NORMALS, sl::MEM::CPU, capture_res);
		ms = zed.getTimestamp(sl::TIME_REFERENCE::IMAGE).getMilliseconds();

		// sl::Mat confidence_map;
		//zed.retrieveMeasure(confidence_map, sl::MEASURE::CONFIDENCE);

		// printf("left %d x %d of %s\n", left.getWidth(), left.getHeight(), sl::toString(left.getDataType()).get());
		// printf("depth %d x %d of %s\n", depth.getWidth(), depth.getHeight(), sl::toString(depth.getDataType()).get());
		// printf("cloud %d x %d of %s step %d\n", cloud.getWidth(), cloud.getHeight(), sl::toString(cloud.getDataType()).get(), cloud.getStepBytes());
		// printf("normals %d x %d of %s step %d\n", normals.getWidth(), normals.getHeight(), sl::toString(normals.getDataType()).get(), normals.getStepBytes());

		return info.This();
	}

};


class Module : public Napi::Addon<Module> {
public:

	/*
		Returns array
	*/
	Napi::Value devices(const Napi::CallbackInfo& info) {
		Napi::Env env = info.Env();
		Napi::Object devices = Napi::Array::New(env);

		std::vector<sl::DeviceProperties> devList = sl::Camera::getDeviceList();
		for (int i = 0; i < devList.size(); i++) {
			Napi::Object device = Napi::Object::New(env);
			device.Set("id", devList[i].id);
			device.Set("serial", devList[i].serial_number);
			device.Set("model", sl::toString(devList[i].camera_model).get());
			device.Set("state", sl::toString(devList[i].camera_state).get());
			devices[i] = device;
		}
		return devices;
	}

	Napi::Value open(const Napi::CallbackInfo& info) {
		Napi::Env env = info.Env();
	}
	
	Module(Napi::Env env, Napi::Object exports) {
		// See https://github.com/nodejs/node-addon-api/blob/main/doc/class_property_descriptor.md
		DefineAddon(exports, {
			// InstanceMethod("start", &Module::start),
			// InstanceMethod("end", &Module::end),
			// //InstanceMethod("test", &Module::test),

			// // InstanceValue
			// // InstanceAccessor
			InstanceAccessor<&Module::devices>("devices"),
			// InstanceAccessor<&Module::Gett>("t"),
			// InstanceAccessor<&Module::GetSamplerate>("samplerate"),
		});
		
		// This method is used to hook the accessor and method callbacks
		Napi::Function ctor = Camera::DefineClass(env, "Camera", {
			
			Camera::InstanceMethod<&Camera::open>("open"),
			Camera::InstanceMethod<&Camera::close>("close"),
			Camera::InstanceMethod<&Camera::isOpened>("isOpened"),
			Camera::InstanceMethod<&Camera::grab>("grab"),
		});

		// Create a persistent reference to the class constructor. This will allow
		// a function called on a class prototype and a function
		// called on instance of a class to be distinguished from each other.
		Napi::FunctionReference* constructor = new Napi::FunctionReference();
		*constructor = Napi::Persistent(ctor);
		exports.Set("Camera", ctor);
		// Store the constructor as the add-on instance data. This will allow this
		// add-on to support multiple instances of itself running on multiple worker
		// threads, as well as multiple instances of itself running in different
		// contexts on the same thread.
		// By default, the value set on the environment here will be destroyed when
		// the add-on is unloaded using the `delete` operator, but it is also
		// possible to supply a custom deleter.
		env.SetInstanceData<Napi::FunctionReference>(constructor);
	}
};

NODE_API_ADDON(Module)