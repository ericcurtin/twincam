#include <kms++/kms++.h>
#include <kms++util/kms++util.h>

#include <fmt/format.h>

#include <fcntl.h>
#include <glob.h>
#include <unistd.h>

#include <linux/videodev2.h>
#include <sys/ioctl.h>

#include <libcamera/libcamera.h>

#define eprint(...) fprintf(stderr, format(__VA_ARGS__).c_str())

using namespace fmt;
using namespace std;
using namespace libcamera::properties;

using libcamera::Camera;
using libcamera::CameraConfiguration;
using libcamera::CameraManager;
using libcamera::ControlId;
using libcamera::ControlInfo;
using libcamera::ControlInfoMap;
using libcamera::ControlList;
using libcamera::Size;
using libcamera::StreamConfiguration;
using libcamera::StreamFormats;
using libcamera::StreamRole;

using kms::AtomicReq;
using kms::Card;
using kms::Connector;
using kms::DumbFramebuffer;
using kms::Framebuffer;
using kms::Plane;
using kms::Videomode;

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

template <typename T>
struct TD;
int main(int argc, char** argv) {
  bool list_displays = false;
  bool list_cameras = false;

  OptionSet optionset = {
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

  Card card;
  if (list_displays) {
    print(
        "Card:\n"
        "is_master: {:d}\n"
        "has_atomic: {:d}\n"
        "has_universal_planes: {:d}\n"
        "has_dumb_buffers: {:d}\n"
        "has_kms: {:d}\n"
        "version_name: {:s}\n\n"
        "\tConnectors:\n",
        card.is_master(), card.has_atomic(), card.has_universal_planes(),
        card.has_dumb_buffers(), card.has_kms(), card.version_name().c_str());
    for (Connector* c : card.get_connectors()) {
      print(
          "\tfullname: {:s}\n"
          "\tconnected: {:d}\n"
          "\tconnector_type: {:d}\n"
          "\tconnector_type_id: {:d}\n"
          "\tmmWidth: {:d}\n"
          "\tmmHeight: {:d}\n"
          "\tsubpixel: {:d}\n"
          "\tsubpixel_str: {:s}\n\n"
          "{:s}",
          c->fullname().c_str(), c->connected(), c->connector_type(),
          c->connector_type_id(), c->mmWidth(), c->mmHeight(), c->subpixel(),
          c->subpixel_str().c_str(),
          c->get_modes().empty() ? "" : "\t\tVideomodes:\n");
      for (Videomode& v : c->get_modes()) {
        print(
            "\t\tto_string_long: {:s}\n"
            "\t\thsync_start: {:d}\n"
            "\t\thsync_end: {:d}\n"
            "\t\thtotal: {:d}\n"
            "\t\thskew: {:d}\n"
            "\t\tvsync_start: {:d}\n"
            "\t\tvsync_end: {:d}\n"
            "\t\tvtotal: {:d}\n"
            "\t\tvscan: {:d}\n"
            "\t\tvalid: {:d}\n\n",
            v.to_string_long().c_str(), v.hsync_start, v.hsync_end, v.htotal,
            v.hskew, v.vsync_start, v.vsync_end, v.vtotal, v.vscan, v.valid());
      }
    }

    print("\t\tPlanes:\n");
    for (Plane* p : card.get_planes()) {
      print(
          "\t\tcrtc_id: {:d}\n"
          "\t\tfb_id: {:d}\n"
          "\t\tcrtc_x: {:d}\n"
          "\t\tcrtc_y: {:d}\n"
          "\t\tx: {:d}\n"
          "\t\ty: {:d}\n"
          "\t\tgamma_size: {:d}\n"
          "\t\tPixelFormatToCC:",
          p->crtc_id(), p->fb_id(), p->crtc_x(), p->crtc_y(), p->x(), p->y(),
          p->gamma_size());
      for (size_t i = 0; i < p->get_formats().size(); ++i) {
        print("{:s}{:s}", i ? ", " : " ",
              PixelFormatToCC(p->get_formats()[i]).c_str());
      }

      print("\n\n");
    }
  }

  if (list_cameras) {
    setenv("LIBCAMERA_LOG_LEVELS", "2", 1);
    CameraManager cm;
    int ret = cm.start();
    if (ret) {
      eprint("{:d} = start()\n", ret);
    }

    if (!cm.cameras().empty()) {
      print("Cameras:\n\n");
    }

    const std::vector<std::shared_ptr<Camera>>& cameras = cm.cameras();
    for (size_t cam_num = 0; cam_num < cameras.size(); ++cam_num) {
      const std::shared_ptr<Camera>& cam = cameras[cam_num];
      const ControlList& props = cam->properties();
      print(
          "cam_num: {:d}\n"
          "id: {:s}\n",
          cam_num, cam.get()->id().c_str());

      if (props.contains(Location)) {
        print("Location: ");
        switch (props.get(Location)) {
          case CameraLocationFront:
            print("front");
            break;
          case CameraLocationBack:
            print("back");
            break;
          case CameraLocationExternal:
            print("external");
            break;
        }

        print("\n");
      }

      if (props.contains(Model)) {
        print("Model: {:s}\n", props.get(Model).c_str());
      }

      if (props.contains(PixelArraySize)) {
        print("PixelArraySize: {:s}\n",
              props.get(PixelArraySize).toString().c_str());
      }

      if (props.contains(PixelArrayActiveAreas)) {
        print("PixelArrayActiveAreas: {:s}\n",
              props.get(PixelArrayActiveAreas)[0].toString().c_str());
      }

      for (const pair<const ControlId* const, ControlInfo>& ctrl :
           cam->controls()) {
        const ControlId* id = ctrl.first;
        const ControlInfo& info = ctrl.second;

        print("{:s}: {:s}\n", id->name().c_str(), info.toString().c_str());
      }

      std::unique_ptr<CameraConfiguration> config =
          cam->generateConfiguration({StreamRole::Viewfinder});
      if (!config) {
        eprint("0 = generateConfiguration()\n");
      }

      switch (config->validate()) {
        case CameraConfiguration::Valid:
          break;

        case CameraConfiguration::Adjusted:
          print("Camera configuration adjusted\n");
          break;

        case CameraConfiguration::Invalid:
          eprint("Camera configuration invalid\n");
          break;
      }

      for (const StreamConfiguration& cfg : *config.get()) {
        print("StreamConfiguration: {:s}\n\n", cfg.toString().c_str());

        const StreamFormats& formats = cfg.formats();
        print("\tPixelFormats:\n");
        for (libcamera::PixelFormat pf : formats.pixelformats()) {
          print("\ttoString: {:s} {:s}\n\tSizes: ", pf.toString().c_str(),
                formats.range(pf).toString().c_str());

          for (size_t i = 0; i < formats.sizes(pf).size(); ++i) {
            print("{:s}{:s}", i ? ", " : "",
                  formats.sizes(pf)[i].toString().c_str());
          }

          print("\n\n");
        }
      }
    }
  }

#if 0
  for (Connector* c : card.get_connectors()) {
    print(
        "\tfullname: {:s}\n"
        "\tconnected: {:d}\n"
        "\tconnector_type: {:d}\n"
        "\tconnector_type_id: {:d}\n"
        "\tmmWidth: {:d}\n"
        "\tmmHeight: {:d}\n"
        "\tsubpixel: {:d}\n"
        "\tsubpixel_str: {:s}\n\n"
        "{:s}",
        c->fullname().c_str(), c->connected(), c->connector_type(),
        c->connector_type_id(), c->mmWidth(), c->mmHeight(), c->subpixel(),
        c->subpixel_str().c_str(),
        c->get_modes().empty() ? "" : "\t\tVideomodes:\n");
    for (Videomode& v : c->get_modes()) {
#endif

#if 0
  cm.cameras();
  static const unsigned CAMERA_BUF_QUEUE_SIZE = 3;
  kms::PixelFormat pf = kms::PixelFormat::YUYV;
  for (unsigned i = 0; i < CAMERA_BUF_QUEUE_SIZE; i++) {
    Framebuffer* fb = new DumbFramebuffer(card, m_in_width, m_in_height, pf);

    v4lbuf.index = i;
    if (m_buffer_provider == BufferProvider::DRM)
      v4lbuf.m.fd = fb->prime_fd(0);
    r = ioctl(m_fd, VIDIOC_QBUF, &v4lbuf);
    ASSERT(r == 0);

    m_fb.push_back(fb);
  }

  AtomicReq req(card);

  Framebuffer* fb = m_fb[0];

  req.add(m_plane, "CRTC_ID", m_crtc->id());
  req.add(m_plane, "FB_ID", fb->id());

  req.add(m_plane, "CRTC_X", m_out_x);
  req.add(m_plane, "CRTC_Y", m_out_y);
  req.add(m_plane, "CRTC_W", m_out_width);
  req.add(m_plane, "CRTC_H", m_out_height);

  req.add(m_plane, "SRC_X", 0);
  req.add(m_plane, "SRC_Y", 0);
  req.add(m_plane, "SRC_W", m_in_width << 16);
  req.add(m_plane, "SRC_H", m_in_height << 16);

  for (int i = 0; i < 2000; ++i) {
    r = req.commit_sync();
    if (!r) {
      break;
    }

    usleep(1000);
  }
#endif
}
