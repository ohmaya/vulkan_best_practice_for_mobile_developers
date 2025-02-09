/* Copyright (c) 2019, Arm Limited and Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge,
 * to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "application.h"
#include "common/vk_common.h"
#include "platform/filesystem.h"
#include "utils.h"

namespace vkb
{
class Application;

enum class ExitCode
{
	Success,
	Fatal
};

class Platform
{
  public:
	Platform();

	virtual ~Platform() = default;

	/**
	 * @brief Sets up windowing system and logging
	 * @param app The application to prepare after the platform is prepared
	 */
	virtual bool initialize(std::unique_ptr<Application> &&app);

	/**
	 * @brief Prepares the active app supplied in the initialize function
	 */
	virtual bool prepare();

	/**
	 * @brief Gets a handle from the platform's Vulkan surface 
	 * @param instance The Vulkan instance
	 * @returns A VkSurfaceKHR handle, for use by the application
	 */
	virtual VkSurfaceKHR create_surface(VkInstance instance) = 0;

	/**
	 * @brief Handles the main loop of the platform
	 *        This function is responsible for calling run()
	 */
	virtual void main_loop() = 0;

	/**
	 * @brief Handles the running of the app
	 */
	void run();

	/**
	 * @brief Terminates the platform and the application
	 * @param code Determines how the platform should exit
	 */
	virtual void terminate(ExitCode code);

	/**
	 * @brief Requests to close the platform at the next available point
	 */
	virtual void close() const = 0;

	/**
	 * @brief Returns the working directory of the application set by the platform
	 * @returns The path to the working directory
	 */
	static const std::string &get_external_storage_directory();

	/**
	 * @brief Returns the suitable directory for temporary files from the environment variables set in the system
	 * @returns The path to the temp folder on the system
	 */
	static const std::string &get_temp_directory();

	/**
	 * @return The dot-per-inch scale factor
	 */
	virtual float get_dpi_factor() const;

	Application &get_app() const;

	std::vector<std::string> &get_arguments();

	static void set_arguments(const std::vector<std::string> &args);

	static void set_external_storage_directory(const std::string &dir);

	static void set_temp_directory(const std::string &dir);

  protected:
	std::unique_ptr<Application> active_app;

	bool benchmark_mode{false};

	uint32_t total_benchmark_frames{0};

	uint32_t remaining_benchmark_frames{0};

	Timer timer;

	virtual std::vector<spdlog::sink_ptr> get_platform_sinks();

  private:
	/// Static so can be set via JNI code in android_platform.cpp
	static std::vector<std::string> arguments;

	static std::string external_storage_directory;

	static std::string temp_directory;
};
}        // namespace vkb
