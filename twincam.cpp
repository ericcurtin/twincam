#include <kms++/kms++.h>
#include <kms++util/kms++util.h>

#include <fcntl.h>
#include <glob.h>
#include <unistd.h>

#include <linux/videodev2.h>
#include <sys/ioctl.h>

#include <libcamera/libcamera.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

using namespace std;
using namespace libcamera::properties;

using libcamera::Camera;
using libcamera::CameraManager;
using libcamera::ControlList;

using kms::Card;
using kms::Connector;
using kms::Plane;
using kms::Videomode;

enum class api { DRM, V4L2 };

static const char* usage_str =
    "Usage: twincam [OPTIONS]\n\n"
    "Options:\n"
    "  -s, --single                Single camera mode. Open only /dev/video0\n"
    "  -a, --api=<drm|v4l>         Use DRM or V4L API. Default: DRM\n"
    "  -l, --list-displays         List displays\n"
    "  -h, --help                  Print this help\n";

static std::string PixelFormatToCC(const kms::PixelFormat& f) {
  char buf[5] = {(char)(((uint32_t)f >> 0) & 0xff),
                 (char)(((uint32_t)f >> 8) & 0xff),
                 (char)(((uint32_t)f >> 16) & 0xff),
                 (char)(((uint32_t)f >> 24) & 0xff), 0};
  for (int i = 0; i < 5; ++i) {
    if (isspace(buf[i])) {
      buf[i] = 0;
      break;
    }
  }

  return std::string(buf);
}

#if 0
static std::vector<std::string> glob(const std::string& pat) {
  glob_t glob_result;
  glob(pat.c_str(), 0, NULL, &glob_result);
  vector<string> ret;
  for (unsigned i = 0; i < glob_result.gl_pathc; ++i) {
    ret.push_back(string(glob_result.gl_pathv[i]));
  }

  globfree(&glob_result);
  return ret;
}

static bool is_capture_dev(int fd) {
  struct v4l2_capability cap = {};
  int r = ioctl(fd, VIDIOC_QUERYCAP, &cap);
  if (r) {
    eprintf("%d = ioctl(%d, %ld, %p), errno: %d\n", r, fd, VIDIOC_QUERYCAP,
            &cap, errno);
    return false;
  }

  return cap.capabilities & V4L2_CAP_VIDEO_CAPTURE;
}
#endif

template <typename T>
struct TD;
int main(int argc, char** argv) {
  api dov = api::DRM;
  bool list_displays = false;
  bool list_cameras = false;

  OptionSet optionset = {
      Option("a|api",
             [&](string s) {
               if (s == "v4l")
                 dov = api::V4L2;
               else if (s == "drm")
                 dov = api::DRM;
               else
                 eprintf("invalid api: %s", s.c_str());
             }),
      Option("l|list-displays", [&]() { list_displays = true; }),
      Option("c|list-cameras", [&]() { list_cameras = true; }),
      Option("h|help",
             [&]() {
               puts(usage_str);
               return -1;
             }),
  };

  optionset.parse(argc, argv);

  if (optionset.params().size() > 0) {
    puts(usage_str);
    return -1;
  }

  if (list_displays) {
    Card card;
    printf(
        "Card:\n"
        "is_master: %d\n"
        "has_atomic: %d\n"
        "has_universal_planes: %d\n"
        "has_dumb_buffers: %d\n"
        "has_kms: %d\n"
        "version_name: %s\n\n"
        "\tConnectors:\n",
        card.is_master(), card.has_atomic(), card.has_universal_planes(),
        card.has_dumb_buffers(), card.has_kms(), card.version_name().c_str());
    for (Connector* c : card.get_connectors()) {
      printf(
          "\tfullname: %s\n"
          "\tconnected: %d\n"
          "\tconnector_type: %d\n"
          "\tconnector_type_id: %d\n"
          "\tmmWidth: %d\n"
          "\tmmHeight: %d\n"
          "\tsubpixel: %d\n"
          "\tsubpixel_str: %s\n\n"
          "%s",
          c->fullname().c_str(), c->connected(), c->connector_type(),
          c->connector_type_id(), c->mmWidth(), c->mmHeight(), c->subpixel(),
          c->subpixel_str().c_str(),
          c->get_modes().empty() ? "" : "\t\tVideomodes:\n");
      for (Videomode& v : c->get_modes()) {
        printf(
            "\t\tto_string_long: %s\n"
            "\t\thsync_start: %d\n"
            "\t\thsync_end: %d\n"
            "\t\thtotal: %d\n"
            "\t\thskew: %d\n"
            "\t\tvsync_start: %d\n"
            "\t\tvsync_end: %d\n"
            "\t\tvtotal: %d\n"
            "\t\tvscan: %d\n"
            "\t\tvalid: %d\n\n",
            v.to_string_long().c_str(), v.hsync_start, v.hsync_end, v.htotal,
            v.hskew, v.vsync_start, v.vsync_end, v.vtotal, v.vscan, v.valid());
      }
    }

    printf("\t\tPlanes:\n");
    for (Plane* p : card.get_planes()) {
      printf(
          "\t\tcrtc_id: %d\n"
          "\t\tfb_id: %d\n"
          "\t\tcrtc_x: %d\n"
          "\t\tcrtc_y: %d\n"
          "\t\tx: %d\n"
          "\t\ty: %d\n"
          "\t\tgamma_size: %d\n"
          "\t\tPixelFormatToCC:",
          p->crtc_id(), p->fb_id(), p->crtc_x(), p->crtc_y(), p->x(), p->y(),
          p->gamma_size());
      for (size_t i = 0; i < p->get_formats().size(); ++i) {
        printf("%s%s", i ? ", " : " ",
               PixelFormatToCC(p->get_formats()[i]).c_str());
      }

      printf("\n\n");
    }
  }

  if (list_cameras) {
#if 0
    vector<int> camera_fds;
    for (string vidpath : glob("/dev/video*")) {
      int fd = open(vidpath.c_str(), O_RDWR | O_NONBLOCK);

      if (fd < 0)
        continue;

      if (!is_capture_dev(fd)) {
        close(fd);
        continue;
      }

      camera_fds.push_back(fd);
      printf("Using %s\n", vidpath.c_str());
    }
#endif

    setenv("LIBCAMERA_LOG_LEVELS", "2", 1);
    CameraManager cm;
    int ret = cm.start();
    if (ret) {
      eprintf("%d = start()\n", ret);
    }

    if (!cm.cameras().empty()) {
      printf("Cameras:\n\n");
    }

    const std::vector<std::shared_ptr<Camera>>& cameras = cm.cameras();
    for (size_t cam_num = 0; cam_num < cameras.size(); ++cam_num) {
      const std::shared_ptr<Camera>& cam = cameras[cam_num];
      const ControlList& props = cam->properties();
      printf(
          "cam_num: %ld\n"
          "id: %s\n",
          cam_num, cam.get()->id().c_str());

      if (props.contains(Location)) {
        printf("Location: ");
        switch (props.get(Location)) {
          case CameraLocationFront:
            printf("front");
            break;
          case CameraLocationBack:
            printf("back");
            break;
          case CameraLocationExternal:
            printf("external");
            break;
        }

        printf("\n");
      }

      if (props.contains(Model)) {
        printf("Model: %s\n", props.get(Model).c_str());
      }

      if (props.contains(PixelArraySize)) {
        printf("PixelArraySize: %s\n",
               props.get(PixelArraySize).toString().c_str());
      }

      if (props.contains(PixelArrayActiveAreas)) {
        printf("PixelArrayActiveAreas: %s\n",
               props.get(PixelArrayActiveAreas)[0].toString().c_str());
      }

      printf("\n");

      cam.get()->acquire();
#if 0
      for (ControlList::const_iterator it = cam.get()->properties().begin();
           it != cam.get()->properties().end(); it++) {
        if (it->first == LOCATION) {
          printf("\tLocation: ");
          if (it->second == CameraLocationFront) {
            printf("front");
          } else if (it->second == CameraLocationBack) {
            printf("back");
          } else if (it->second ==
                     CameraLocationExternal) {
            printf("external");
          } else {
            printf("unknown");
          }

          printf("\n");
        }

if (it->first == MODEL) {
          printf("\tMODEL: %s\n", it->second.toString().c_str());
        } else if (it->first == PIXEL_ARRAY_SIZE) {
          printf("\tPIXEL_ARRAY_SIZE: %s\n", it->second.toString().c_str());
        } else if (it->first ==
                   PIXEL_ARRAY_ACTIVE_AREAS) {
          printf("\tPIXEL_ARRAY_ACTIVE_AREAS: %s\n",
                 it->second.toString().c_str());
        } else {
          printf("\tunknown property\n");
        }
      }

      for (const auto& ctrl : cam->controls()) {
        const ControlId* id = ctrl.first;
        const ControlInfo& info = ctrl.second;

        printf("Control: %s\n%s\n", id->name().c_str(),
               info.toString().c_str());
      }

      printf("\n");
#endif
    }
  }
}
