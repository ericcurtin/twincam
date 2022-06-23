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

static bool dev_video_exists() {
  const char name[] = "/dev/";
  DIR* folder = opendir(name);
  if (!folder) {
    return false;
  }

  for (struct dirent* res; (res = readdir(folder));) {
    if (!memcmp(res->d_name, "video", 5)) {
      closedir(folder);
      return true;
    }
  }

  closedir(folder);

  return false;
}

#ifdef HAVE_LIBUDEV
static int checkUdevDevice(struct udev_device* dev) {
  const char* subsystem = udev_device_get_subsystem(dev);
  VERBOSE_PRINT("subsystem: '%s'\n", subsystem);
  if (!subsystem)
    return -ENODEV;

  if (!strcmp(subsystem, "video4linux")) {
    return 0;
  }

  return -ENODEV;
}

static int wait_for_udev() {
  struct udev* udev = udev_new();  // init
  int ret = -1;
  struct udev_enumerate* udev_enum = 0;
  if (!udev) {
    ret = -ENODEV;
    goto done;
  }

  udev_enum = udev_enumerate_new(udev);  // enumerate
  if (!udev_enum) {
    ret = -ENOMEM;
    goto done;
  }

  ret = udev_enumerate_add_match_subsystem(udev_enum, "video4linux");
  if (ret < 0)
    goto done;

  ret = udev_enumerate_add_match_is_initialized(udev_enum);
  if (ret < 0)
    goto done;

  struct udev_list_entry* ents;
  ret = udev_enumerate_scan_devices(udev_enum);
  if (ret < 0)
    goto done;

  ents = udev_enumerate_get_list_entry(udev_enum);
  if (!ents) {
    ret = -1;
    goto done;
  }

  struct udev_list_entry* ent;
  udev_list_entry_foreach(ent, ents) {
    struct udev_device* dev;
    const char* devnode;
    const char* syspath = udev_list_entry_get_name(ent);

    dev = udev_device_new_from_syspath(udev, syspath);
    if (!dev) {
      EPRINT("Failed to get device for '%s', skipping\n", syspath);
      continue;
    }

    devnode = udev_device_get_devnode(dev);
    if (!devnode) {
      udev_device_unref(dev);
      EPRINT("Failed to get device node for '%s', skipping\n", syspath);
      continue;
    }

    if (checkUdevDevice(dev) < 0) {
      EPRINT("Not a valid camera device for '%s', skipping\n", syspath);
      continue;
    } else {
      ret = 0;
    }

    udev_device_unref(dev);
    break;
  }

done:
  if (udev_enum)
    udev_enumerate_unref(udev_enum);

  if (udev)
    udev_unref(udev);

  return ret;
}
#endif

int CamApp::init() {
  cm_ = std::make_unique<CameraManager>();

  // V4L2 specific
  int ret = -1;
  for (int i = 0; i < 400; ++i) {
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

  ret = -1;
  for (int i = 0; i < 400; ++i) {
    if (dev_video_exists()) {
      ret = 0;
      break;
    }

    usleep(10000);
  }

  if (ret < 0) {
    PRINT("Failed to find a /dev/media* entry\n");
    return ret;
  }

#ifdef HAVE_LIBUDEV
  ret = -1;
  for (int i = 0; i < 400; ++i) {
    if (wait_for_udev() >= 0) {
      ret = 0;
      break;
    }

    usleep(10000);
  }

  if (ret < 0) {
    PRINT("Failed to find a udev entry\n");
    return ret;
  }
#endif

  ret = cm_->start();
  if (ret) {
    PRINT("Failed to start camera manager: %s\n", strerror(-ret));
    return ret;
  }

  return 0;
}

void CamApp::cleanup() const {
  cm_->stop();
}

int CamApp::exec() {
  PRINT_UPTIME();
  int ret;

  ret = run();
  cleanup();

  return ret;
}

void CamApp::quit() {
  loop_.exit();
}

int CamApp::run() {
  PRINT_UPTIME();
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
  if (props.contains(properties::Location)) {
    switch (props.get(properties::Location)) {
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

  if (addModel && props.contains(properties::Model)) {
    /*
     * If the camera location is not availble use the camera model
     * to build the camera name.
     */
    name = "'" + props.get(properties::Model) + "' ";
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
                                   {"filename", no_argument, 0, 'F'},
                                   {"help", no_argument, 0, 'h'},
                                   {"list-cameras", no_argument, 0, 'l'},
                                   {"new-root-dir", no_argument, 0, 'n'},
                                   {"syslog", no_argument, 0, 's'},
                                   {"uptime", no_argument, 0, 'u'},
                                   {"verbose", no_argument, 0, 'v'},
                                   {"sdl", no_argument, 0, 'S'},
                                   {"pixel-format", no_argument, 0, 'p'},
                                   {NULL, 0, 0, '\0'}};
  for (int opt; (opt = getopt_long(argc, argv, "c:dF:hlnusvSp:", options,
                                   NULL)) != -1;) {
    int fd;
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
      case 'F':
        opts.filename = optarg;
        break;
      case 'l':
        opts.print_available_cameras = true;
        break;
      case 'n':
        fd = twncm_open_read("/var/run/twincam.pid");
        char buf[16];
        pid_read(fd, buf);
        kill(twncm_atoi(buf), SIGUSR1);

        return 1;
      case 'u':
        opts.print_uptime = true;
        break;
      case 's':
        opts.to_syslog = true;
        setenv("LIBCAMERA_LOG_FILE", "syslog", 1);
        openlog("twincam", 0, LOG_LOCAL1);
        break;
      case 'p':
        opts.pf = optarg;
        break;
      case 'S':
        opts.sdl = true;
        break;
      case 'v':
        opts.verbose = true;
        setenv("LIBCAMERA_LOG_LEVELS", "DEBUG", 1);
        break;
      default:
        PRINT(
            "Usage: twincam [OPTIONS]\n\n"
            "Options:\n"
            "  -c, --camera        Camera to select\n"
            "  -d, --daemon        Daemon mode (write a pid file "
            "/var/run/twincam.pid)\n"
            "  -h, --help          Print this help\n"
            "  -l, --list-cameras  List cameras\n"
            "  -p, --pixel-format  Select pixel format\n"
            "  -S, --sdl           Display viewfinder through SDL\n"
            "  -n, --new-root-dir  chroot to /sysroot (sends SIGUSR1 to "
            "pidfile pid)\n"
            "  -u, --uptime        Trace the uptime\n"
            "  -s, --syslog        Also trace output in syslog\n"
            "  -v, --verbose       Enable verbose logging\n");

        return 1;
    }
  }

  return 0;
}

options opts;
int main(int argc, char** argv) {
  PRINT_UPTIME();  // Although this is never printed, it's important because it
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
