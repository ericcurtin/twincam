/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2019, Google Inc.
 *
 * main.cpp - cam - The libcamera swiss army knife
 */

#include <dirent.h>
#include <getopt.h>
#ifdef HAVE_LIBUDEV
#include <libudev.h>
#endif
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <atomic>
#include <iomanip>

#include <libcamera/libcamera.h>
#include <libcamera/property_ids.h>

#include "camera_session.h"
#include "event_loop.h"
#include "twincam.h"
#include "twncm_fnctl.h"
#include "twncm_stdlib.h"
#include "uptime.h"

using namespace libcamera;

class CamApp {
 public:
  CamApp();

  static CamApp* instance();

  int init();
  void cleanup() const;

  int exec();
  void quit();

 private:
  void cameraAdded(std::shared_ptr<Camera> cam);
  void cameraRemoved(std::shared_ptr<Camera> cam);
  void captureDone();
  int run();

  static std::string cameraName(const Camera* camera);

  static CamApp* app_;

  std::unique_ptr<CameraManager> cm_;

  EventLoop loop_;
};

CamApp* CamApp::app_ = nullptr;

CamApp::CamApp() {
  CamApp::app_ = this;
}

CamApp* CamApp::instance() {
  return CamApp::app_;
}

static bool sysfs_exists() {
  static const char* const sysfs_dirs[] = {
      "/sys/subsystem/media/devices",
      "/sys/bus/media/devices",
      "/sys/class/media/devices",
  };

  for (const char* dirname : sysfs_dirs) {
    DIR* dir = opendir(dirname);
    if (dir) {
      closedir(dir);
      return true;
    }
  }

  return false;
}

static bool dev_exists(const char* fn) {
  const char name[] = "/dev/";
  DIR* folder = opendir(name);
  if (!folder) {
    return false;
  }

  for (struct dirent* res; (res = readdir(folder));) {
    if (!memcmp(res->d_name, fn, 5)) {
      closedir(folder);
      return true;
    }
  }

  closedir(folder);

  return false;
}

#ifdef HAVE_LIBUDEV
static int checkUdevDevice(struct udev_device* dev, const char* subsys) {
  const char* subsystem = udev_device_get_subsystem(dev);
  VERBOSE_PRINT("subsystem: '%s'\n", subsystem);
  if (!subsystem)
    return -ENODEV;

  if (!strcmp(subsystem, subsys)) {
    return 0;
  }

  return -ENODEV;
}

struct udev_enum {
  struct udev_enumerate* ptr = 0;

  ~udev_enum() {
    if (ptr)
      udev_enumerate_unref(ptr);
  }
};

struct udev {
  struct udev* ptr = 0;

  ~udev() {
    if (ptr)
      udev_unref(ptr);
  }
};

struct udev_device {
  struct udev_device* ptr = 0;

  ~udev_device() {
    if (ptr)
      udev_device_unref(ptr);
  }
};

static int wait_for_udev(const char* subsys) {
  int ret = -1;
  udev udev;
  udev.ptr = udev_new();
  if (!udev.ptr) {
    return -ENODEV;
  }

  udev_enum udev_enum;
  udev_enum.ptr = udev_enumerate_new(udev.ptr);  // enumerate
  if (!udev_enum.ptr) {
    return -ENOMEM;
  }

  ret = udev_enumerate_add_match_subsystem(udev_enum.ptr, subsys);
  if (ret < 0)
    return ret;

  ret = udev_enumerate_add_match_is_initialized(udev_enum.ptr);
  if (ret < 0)
    return ret;

  struct udev_list_entry* ents;
  ret = udev_enumerate_scan_devices(udev_enum.ptr);
  if (ret < 0)
    return ret;

  ents = udev_enumerate_get_list_entry(udev_enum.ptr);
  if (!ents) {
    return -1;
  }

  struct udev_list_entry* ent;
  udev_list_entry_foreach(ent, ents) {
    udev_device dev;
    const char* devnode;
    const char* syspath = udev_list_entry_get_name(ent);

    dev.ptr = udev_device_new_from_syspath(udev.ptr, syspath);
    if (!dev.ptr) {
      EPRINT("Failed to get device for '%s', skipping\n", syspath);
      continue;
    }

    devnode = udev_device_get_devnode(dev.ptr);
    if (!devnode) {
      EPRINT("Failed to get device node for '%s', skipping\n", syspath);
      continue;
    }

    if (checkUdevDevice(dev.ptr, subsys) < 0) {
      EPRINT("Not a valid camera device for '%s', skipping\n", syspath);
      continue;
    } else {
      ret = 0;
    }

    break;
  }

  return ret;
}
#endif

int CamApp::init() {
  cm_ = std::make_unique<CameraManager>();

  // V4L2 specific
  int ret = -1;
  const int end = 400;
  for (int i = 0; i < end; ++i) {
    if (sysfs_exists()) {
      ret = 0;
      break;
    }

    usleep(10000);
  }

  if (ret < 0) {
    PRINT("Failed to find sysfs\n");
    return ret;
  }

  VERBOSE_PRINT("Found sysfs\n");

  ret = -1;
  for (int i = 0; i < end; ++i) {
    if (dev_exists("video")) {
      ret = 0;
      break;
    }

    usleep(10000);
  }

  if (ret < 0) {
    PRINT("Failed to find a /dev/video* entry\n");
    return ret;
  }

  VERBOSE_PRINT("Found /dev/video* entry\n");

  ret = -1;
  for (int i = 0; i < end; ++i) {
    if (dev_exists("media")) {
      ret = 0;
      break;
    }

    usleep(10000);
  }

  if (ret < 0) {
    PRINT("Failed to find a /dev/media* entry\n");
    return ret;
  }

  VERBOSE_PRINT("Found /dev/media* entry\n");

#ifdef HAVE_LIBUDEV
  ret = -1;
  for (int i = 0; i < end; ++i) {
    if (!wait_for_udev("media")) {
      ret = 0;
      break;
    }

    usleep(10000);
  }

  if (ret < 0) {
    PRINT("Failed to find a udev entry\n");
    return ret;
  }

  VERBOSE_PRINT("Found media udev entry\n");

  ret = -1;
  for (int i = 0; i < end; ++i) {
    if (!wait_for_udev("video4linux")) {
      ret = 0;
      break;
    }

    usleep(10000);
  }

  if (ret < 0) {
    PRINT("Failed to find a video4linux udev entry\n");
    return ret;
  }

  VERBOSE_PRINT("Found video4linux udev entry\n");
#endif

  ret = cm_->start();
  if (ret) {
    PRINT("Failed to start camera manager: %s\n", strerror(-ret));
    return ret;
  }

  ret = -1;
  for (int i = 0; i < end; ++i) {
    if (!cm_->cameras().empty()) {
      ret = 0;
      break;
    }

    usleep(10000);
  }

  if (ret < 0) {
    PRINT("Failed to find a camera\n");
    return ret;
  }

  VERBOSE_PRINT("Found camera\n");

  return 0;
}

void CamApp::cleanup() const {
  cm_->stop();
}

int CamApp::exec() {
  PRINT_FUNC();
  int ret;

  ret = run();
  cleanup();

  return ret;
}

void CamApp::quit() {
  loop_.exit();
}

int CamApp::run() {
  PRINT_FUNC();
  if (opts.print_available_cameras) {
    PRINT("Available cameras:\n");
    for (size_t i = 0; i < cm_->cameras().size(); ++i) {
      PRINT("%zu: %s\n", i, cameraName(cm_->cameras()[i].get()).c_str());
    }

    return 0;
  }

  if (cm_->cameras().empty()) {
    PRINT("No cameras available\n");
    return 0;
  }

  CameraSession session(cm_.get());
  int ret = session.init();
  if (ret) {
    PRINT("Failed to init camera session\n");
    return ret;
  }

  ret = session.start();
  if (ret) {
    PRINT("Failed to start camera session\n");
    return ret;
  }

  loop_.exec();

  session.stop();

  return 0;
}

std::string CamApp::cameraName(const Camera* camera) {
  const ControlList& props = camera->properties();
  bool addModel = true;
  std::string name;

  /*
   * Construct the name from the camera location, model and ID. The model
   * is only used if the location isn't present or is set to External.
   */
  const std::optional<int>& location = props.get(properties::Location);
  if (location) {
    switch (*location) {
      case properties::CameraLocationFront:
        addModel = false;
        name = "Internal front camera ";
        break;
      case properties::CameraLocationBack:
        addModel = false;
        name = "Internal back camera ";
        break;
      case properties::CameraLocationExternal:
        name = "External camera ";
        break;
      default:
        break;
    }
  }

  if (addModel) {
    /*
     * If the camera location is not availble use the camera model
     * to build the camera name.
     */
    const std::optional<std::string>& model = props.get(properties::Model);
    if (model)
      name = "'" + *model + "' ";
  }

  name += "(" + camera->id() + ")";

  return name;
}

static void signalHandler([[maybe_unused]] int signal) {
  VERBOSE_PRINT("Exiting\n");
  CamApp::instance()->quit();
  if (opts.to_syslog) {
    closelog();
  }
}

static void printExit([[maybe_unused]] int signal) {
  VERBOSE_PRINT("Exit with signal: %d\n", signal);
  CamApp::instance()->quit();
  if (opts.to_syslog) {
    closelog();
  }
}

static void chrootThis([[maybe_unused]] int signal) {
  VERBOSE_PRINT("Chrooting: %d\n", signal);
  int ret = chdir("/sysroot");
  if (ret) {
    EPRINT("chdir /sysroot failed: %d\n", ret);
    return;
  }

  ret = chroot(".");
  if (ret) {
    EPRINT("chroot failed: %d\n", ret);
    return;
  }

  ret = chdir("/");
  if (ret) {
    EPRINT("chdir / failed: %d\n", ret);
    return;
  }

  setlocale(LC_ALL, "");
}

static int processArgs(int argc, char** argv) {
  const struct option options[] = {{"camera", required_argument, 0, 'c'},
                                   {"daemon", no_argument, 0, 'd'},
#ifdef HAVE_DRM
                                   {"drm", no_argument, 0, 'D'},
#endif
                                   {"filename", required_argument, 0, 'F'},
                                   {"function", no_argument, 0, 'f'},
                                   {"help", no_argument, 0, 'h'},
                                   {"kill", no_argument, 0, 'k'},
                                   {"list-cameras", no_argument, 0, 'l'},
                                   {"new-root-dir", no_argument, 0, 'n'},
                                   {"pixel-format", required_argument, 0, 'p'},
#ifdef HAVE_SDL
                                   {"sdl", no_argument, 0, 'S'},
#endif
                                   {"syslog", no_argument, 0, 's'},
                                   {"uptime", no_argument, 0, 'u'},
                                   {"verbose", no_argument, 0, 'v'},
                                   {NULL, 0, 0, '\0'}};

  for (int opt; (opt = getopt_long(argc, argv, "c:dDF:fhklnp:Ssuv", options,
                                   NULL)) != -1;) {
    int fd;
    char buf[16];
    switch (opt) {
      case 'c':
        opts.camera = twncm_atoi(optarg);
        break;
      case 'd':
        fd = twncm_open_write("/var/run/twincam.pid");
        if (fd < 0) {
          break;
        }

        pid_write(fd);
        twncm_close(fd);
        break;
#ifdef HAVE_DRM
      case 'D':
        opts.drm = true;
        break;
#endif
      case 'F':
        opts.filename = optarg;
        break;
      case 'f':
        opts.print_func = true;
        break;
      case 'k':
        fd = twncm_open_read("/var/run/twincam.pid");
        pid_read(fd, buf);
        kill(twncm_atoi(buf), SIGTERM);

        return 1;
      case 'l':
        opts.print_available_cameras = true;
        break;
      case 'n':
        fd = twncm_open_read("/var/run/twincam.pid");
        pid_read(fd, buf);
        kill(twncm_atoi(buf), SIGUSR1);

        return 1;
      case 'p':
        opts.pf = optarg;
        break;
#ifdef HAVE_SDL
      case 'S':
        opts.sdl = true;
        break;
#endif
      case 's':
        opts.to_syslog = true;
        setenv("LIBCAMERA_LOG_FILE", "syslog", 1);
        openlog("twincam", 0, LOG_LOCAL1);
        break;
      case 'u':
        opts.uptime = true;
        break;
      case 'v':
        opts.verbose = true;
        setenv("LIBCAMERA_LOG_LEVELS", "DEBUG", 1);
        break;
      default:
        static const char* help =
            "Usage: twincam [OPTIONS]\n\n"
            "Options:\n"
            "  -c, --camera        Camera to select\n"
            "  -d, --daemon        Daemon mode (write a pid file "
            "/var/run/twincam.pid)\n"
#ifdef HAVE_DRM
            "  -D, --drm           Display viewfinder through drm\n"
#endif
            "  -F, --filename      Write captured frames to disk\n"
            "  -f, --function      function tracer\n"
            "  -h, --help          Print this help\n"
            "  -k, --kill          Kill twincam (sends SIGTERM to "
            "pidfile pid)\n"
            "  -l, --list-cameras  List cameras\n"
            "  -n, --new-root-dir  chroot to /sysroot (sends SIGUSR1 to "
            "pidfile pid)\n"
            "  -p, --pixel-format  Select pixel format\n"
#ifdef HAVE_SDL
            "  -S, --sdl           Display viewfinder through SDL\n"
#endif
            "  -s, --syslog        Also trace output in syslog\n"
            "  -u, --uptime        prepend prints with uptime\n"
            "  -v, --verbose       Enable verbose logging";
        PRINT("%s\n", help);

        return 1;
    }
  }

  return 0;
}

options opts;
int main(int argc, char** argv) {
  PRINT_FUNC();  // Although this is never printed, it's important because it
                 // sets the initial timer
  CamApp app;
  struct sigaction sa = {};
  struct sigaction sa_err = {};
  struct sigaction sa_chroot = {};
  int ret = processArgs(argc, argv);
  if (ret) {
    ret = 0;
    goto end;
  }

  VERBOSE_PRINT("Successfully processed args\n");

  ret = app.init();
  if (ret) {
    ret = ret == -EINTR ? 0 : EXIT_FAILURE;
    goto end;
  }

  sa.sa_handler = &signalHandler;
  sa_err.sa_handler = &printExit;
  sa_chroot.sa_handler = &chrootThis;
  sigaction(SIGINT, &sa, nullptr);
  sigaction(SIGFPE, &sa_err, nullptr);
  sigaction(SIGILL, &sa_err, nullptr);
  sigaction(SIGSEGV, &sa_err, nullptr);
  sigaction(SIGBUS, &sa_err, nullptr);
  sigaction(SIGABRT, &sa_err, nullptr);
  sigaction(SIGIOT, &sa_err, nullptr);
  sigaction(SIGTRAP, &sa_err, nullptr);
  sigaction(SIGSYS, &sa_err, nullptr);
  sigaction(SIGUSR1, &sa_chroot, nullptr);

  if (app.exec()) {
    ret = EXIT_FAILURE;
    goto end;
  }

end:
  VERBOSE_PRINT("Exit with status: %d\n", ret);
  return ret;
}
