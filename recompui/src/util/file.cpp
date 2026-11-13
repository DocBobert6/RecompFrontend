#include "file.h"

#include <SDL.h>
#include "nfd.h"
#include "RmlUi/Core.h"

#include "recompui/program_config.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>

#if defined(__ANDROID__)
#include <jni.h>
#include <SDL_system.h>
#endif

#if defined(_WIN32)
#include <Shlobj.h>
#elif defined(__linux__)
#include <unistd.h>
#include <pwd.h>
#elif defined(__APPLE__)
#include "apple/rt64_apple.h"
#endif

namespace recompui {
    static void perform_file_dialog_operation(const std::function<void(bool, const std::filesystem::path&)>& callback) {
        nfdnchar_t* native_path = nullptr;
        nfdresult_t result = NFD_OpenDialogN(&native_path, nullptr, 0, nullptr);

        bool success = (result == NFD_OKAY);
        std::filesystem::path path;

        if (success) {
            path = std::filesystem::path{native_path};
            NFD_FreePathN(native_path);
        }

        callback(success, path);
    }

    static void perform_file_dialog_operation_multiple(const std::function<void(bool, const std::list<std::filesystem::path>&)>& callback) {
        const nfdpathset_t* native_paths = nullptr;
        nfdresult_t result = NFD_OpenDialogMultipleN(&native_paths, nullptr, 0, nullptr);

        bool success = (result == NFD_OKAY);
        std::list<std::filesystem::path> paths;
        nfdpathsetsize_t count = 0;

        if (success) {
            NFD_PathSet_GetCount(native_paths, &count);
            for (nfdpathsetsize_t i = 0; i < count; i++) {
                nfdnchar_t* cur_path = nullptr;
                nfdresult_t cur_result = NFD_PathSet_GetPathN(native_paths, i, &cur_path);
                if (cur_result == NFD_OKAY) {
                    paths.emplace_back(std::filesystem::path{cur_path});
                }
            }
            NFD_PathSet_Free(native_paths);
        }

        callback(success, paths);
    }

    std::filesystem::path file::get_app_folder_path() {
        // directly check for portable.txt (windows and native linux binary)
        if (std::filesystem::exists("portable.txt")) {
            return std::filesystem::current_path();
        }

#if defined(__APPLE__)
        // Check for portable file in the directory containing the app bundle.
        const auto app_bundle_path = file::apple::get_bundle_directory().parent_path();
        if (std::filesystem::exists(app_bundle_path / "portable.txt")) {
            return app_bundle_path;
        }
#endif

        std::filesystem::path recomp_dir{};

#if defined(_WIN32)
        // Deduce local app data path.
        PWSTR known_path = NULL;
        HRESULT result = SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &known_path);
        if (result == S_OK) {
            recomp_dir = std::filesystem::path{known_path} / programconfig::get_program_id();
        }

        CoTaskMemFree(known_path);
#elif defined(__linux__) || defined(__APPLE__)
        // check for APP_FOLDER_PATH env var
        if (getenv("APP_FOLDER_PATH") != nullptr) {
            return std::filesystem::path{getenv("APP_FOLDER_PATH")};
        }

#if defined(__APPLE__)
        const auto supportdir = file::apple::get_application_support_directory();
        if (supportdir) {
            return *supportdir / programconfig::get_program_id();
        }
#endif

        const char *homedir;

        if ((homedir = getenv("HOME")) == nullptr) {
            #if defined(__linux__)
            homedir = getpwuid(getuid())->pw_dir;
            #elif defined(__APPLE__)
                homedir = GetHomeDirectory();
            #endif
        }

        if (homedir != nullptr) {
            recomp_dir = std::filesystem::path{homedir} / (std::u8string{u8".config/"} + programconfig::get_program_id());
        }
#endif

        return recomp_dir;
    }

    std::filesystem::path file::get_program_path() {
#if defined(__ANDROID__)
        // Program assets are extracted from the APK by the activity, which
        // exports their location. Must be checked before __linux__ (also
        // defined on Android); an empty path here means assets fail to load
        // and the UI init exits.
        if (const char* program_path = std::getenv("APP_PROGRAM_PATH")) {
            return std::filesystem::path{program_path};
        }
        return "";
#elif defined(__APPLE__)
        return file::apple::get_bundle_resource_directory();
#elif defined(__linux__) && defined(RECOMP_FLATPAK)
        return "/app/bin";
#else
        return "";
#endif
    }

    std::filesystem::path file::get_asset_path(const char* asset) {
        return file::get_program_path() / "assets" / asset;
    }

#ifdef __ANDROID__
    // SAF pickers are asynchronous: the callback is parked here, the activity's
    // static openFileDialog()/openModFileDialog() shows the picker, and the
    // app's JNI result handler calls android_deliver_* to resolve it.
    static std::mutex android_dialog_mutex;
    static std::function<void(bool, const std::filesystem::path&)> android_dialog_callback;
    static std::function<void(bool, const std::list<std::filesystem::path>&)> android_dialog_multiple_callback;

    // Results are parked here by android_deliver_* (which runs on whatever
    // thread the JNI call arrives on) and consumed on the UI render thread by
    // android_drain_pending_dialog_results(), since the callbacks mutate
    // launcher/RmlUi state owned by that thread.
    static bool android_dialog_result_pending = false;
    static bool android_dialog_result_success = false;
    static std::filesystem::path android_dialog_result_path;
    static bool android_dialog_multiple_result_pending = false;
    static bool android_dialog_multiple_result_success = false;
    static std::list<std::filesystem::path> android_dialog_multiple_result_paths;

    static bool android_launch_file_dialog(const char* method_name) {
        JNIEnv* env = static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());
        jobject activity = static_cast<jobject>(SDL_AndroidGetActivity());
        if (env == nullptr || activity == nullptr) {
            if (env != nullptr && activity != nullptr) {
                env->DeleteLocalRef(activity);
            }
            return false;
        }

        jclass activity_class = env->GetObjectClass(activity);
        jmethodID open_method = activity_class != nullptr
            ? env->GetStaticMethodID(activity_class, method_name, "()V")
            : nullptr;
        if (open_method == nullptr && env->ExceptionCheck()) {
            // GetStaticMethodID leaves a pending NoSuchMethodError.
            env->ExceptionDescribe();
            env->ExceptionClear();
        }
        bool ok = false;
        if (open_method != nullptr) {
            env->CallStaticVoidMethod(activity_class, open_method);
            ok = !env->ExceptionCheck();
            if (!ok) {
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
        }
        if (activity_class != nullptr) {
            env->DeleteLocalRef(activity_class);
        }
        env->DeleteLocalRef(activity);
        return ok;
    }

    void file::android_deliver_file_dialog_result(bool success, const char* path) {
        std::lock_guard lock(android_dialog_mutex);
        android_dialog_result_success = success && path != nullptr;
        android_dialog_result_path = path != nullptr ? std::filesystem::path{path} : std::filesystem::path{};
        android_dialog_result_pending = true;
    }

    void file::android_deliver_file_dialog_multiple_result(bool success, const char* const* paths, size_t path_count) {
        std::list<std::filesystem::path> path_list;
        for (size_t i = 0; i < path_count; i++) {
            if (paths[i] != nullptr) {
                path_list.emplace_back(paths[i]);
            }
        }

        std::lock_guard lock(android_dialog_mutex);
        android_dialog_multiple_result_success = success && !path_list.empty();
        android_dialog_multiple_result_paths = std::move(path_list);
        android_dialog_multiple_result_pending = true;
    }

    void file::android_drain_pending_dialog_results() {
        std::function<void(bool, const std::filesystem::path&)> callback;
        bool success = false;
        std::filesystem::path path;
        std::function<void(bool, const std::list<std::filesystem::path>&)> multiple_callback;
        bool multiple_success = false;
        std::list<std::filesystem::path> multiple_paths;
        {
            std::lock_guard lock(android_dialog_mutex);
            if (android_dialog_result_pending) {
                android_dialog_result_pending = false;
                callback = std::move(android_dialog_callback);
                android_dialog_callback = {};
                success = android_dialog_result_success;
                path = std::move(android_dialog_result_path);
                android_dialog_result_path.clear();
            }
            if (android_dialog_multiple_result_pending) {
                android_dialog_multiple_result_pending = false;
                multiple_callback = std::move(android_dialog_multiple_callback);
                android_dialog_multiple_callback = {};
                multiple_success = android_dialog_multiple_result_success;
                multiple_paths = std::move(android_dialog_multiple_result_paths);
                android_dialog_multiple_result_paths.clear();
            }
        }
        // Run the callbacks outside the lock; they may open another dialog.
        if (callback) {
            callback(success, path);
        }
        if (multiple_callback) {
            multiple_callback(multiple_success, multiple_paths);
        }
    }

    bool file::show_diagnostics_activity() {
#ifdef __ANDROID__
        // [wcw diag] Test-only: asks the activity to open the Diagnostics
        // view. Same JNI pattern as android_launch_file_dialog — the
        // activity's static showDiagnostics() starts DiagnosticsActivity.
        return android_launch_file_dialog("showDiagnostics");
#else
        return false;
#endif
    }
#endif

    void file::open_file_dialog(std::function<void(bool success, const std::filesystem::path& path)> callback) {
#ifdef __APPLE__
        file::apple::dispatch_on_ui_thread([callback]() {
            perform_file_dialog_operation(callback);
        });
#elif defined(__ANDROID__)
        {
            std::lock_guard lock(android_dialog_mutex);
            if (android_dialog_callback) {
                callback(false, {});
                return;
            }
            android_dialog_callback = std::move(callback);
        }
        if (!android_launch_file_dialog("openFileDialog")) {
            android_deliver_file_dialog_result(false, nullptr);
        }
#else
        perform_file_dialog_operation(callback);
#endif
    }

    void file::open_file_dialog_multiple(std::function<void(bool success, const std::list<std::filesystem::path>& paths)> callback) {
#ifdef __APPLE__
        file::apple::dispatch_on_ui_thread([callback]() {
            perform_file_dialog_operation_multiple(callback);
        });
#elif defined(__ANDROID__)
        {
            std::lock_guard lock(android_dialog_mutex);
            if (android_dialog_multiple_callback) {
                callback(false, {});
                return;
            }
            android_dialog_multiple_callback = std::move(callback);
        }
        if (!android_launch_file_dialog("openModFileDialog")) {
            android_deliver_file_dialog_multiple_result(false, nullptr, 0);
        }
#else
        perform_file_dialog_operation_multiple(callback);
#endif
    }

    void file::show_error_message_box(const char *title, const char *message) {
#ifdef __APPLE__
    std::string title_copy(title);
    std::string message_copy(message);

    file::apple::dispatch_on_ui_thread([title_copy, message_copy] {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title_copy.c_str(), message_copy.c_str(), nullptr);
    });
#else
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, message, nullptr);
#endif
    }
}
