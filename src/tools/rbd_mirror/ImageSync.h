// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#ifndef RBD_MIRROR_IMAGE_SYNC_H
#define RBD_MIRROR_IMAGE_SYNC_H

#include "include/int_types.h"
#include "librbd/Journal.h"
#include "common/Mutex.h"
#include <map>
#include <vector>

class Context;
class Mutex;
class SafeTimer;
namespace journal { class Journaler; }
namespace librbd { class ImageCtx; }
namespace librbd { namespace journal { struct MirrorPeerClientMeta; } }

namespace rbd {
namespace mirror {

namespace image_sync { template <typename> class ImageCopyRequest; }

template <typename ImageCtxT = librbd::ImageCtx>
class ImageSync {
public:
  typedef librbd::journal::TypeTraits<ImageCtxT> TypeTraits;
  typedef typename TypeTraits::Journaler Journaler;
  typedef librbd::journal::MirrorPeerClientMeta MirrorPeerClientMeta;

  ImageSync(ImageCtxT *local_image_ctx, ImageCtxT *remote_image_ctx,
            SafeTimer *timer, Mutex *timer_lock, const std::string &mirror_uuid,
            Journaler *journaler, MirrorPeerClientMeta *client_meta,
            Context *on_finish);

  void start();
  void cancel();

private:
  /**
   * @verbatim
   *
   * <start>
   *    |
   *    v
   * PRUNE_CATCH_UP_SYNC_POINT
   *    |
   *    v
   * CREATE_SYNC_POINT (skip if already exists and
   *    |               not disconnected)
   *    v
   * COPY_SNAPSHOTS
   *    |
   *    v
   * COPY_IMAGE . . . . . .
   *    |                 .
   *    v                 .
   * PRUNE_SYNC_POINTS    . (image sync canceled)
   *    |                 .
   *    v                 .
   * <finish> < . . . . . .
   *
   * @endverbatim
   */

  typedef std::vector<librados::snap_t> SnapIds;
  typedef std::map<librados::snap_t, SnapIds> SnapMap;

  ImageCtxT *m_local_image_ctx;
  ImageCtxT *m_remote_image_ctx;
  SafeTimer *m_timer;
  Mutex *m_timer_lock;
  std::string m_mirror_uuid;
  Journaler *m_journaler;
  MirrorPeerClientMeta *m_client_meta;
  Context *m_on_finish;

  SnapMap m_snap_map;

  Mutex m_lock;
  bool m_canceled = false;

  image_sync::ImageCopyRequest<ImageCtxT> *m_image_copy_request;

  void send_prune_catch_up_sync_point();
  void handle_prune_catch_up_sync_point(int r);

  void send_create_sync_point();
  void handle_create_sync_point(int r);

  void send_copy_snapshots();
  void handle_copy_snapshots(int r);

  void send_copy_image();
  void handle_copy_image(int r);

  void send_prune_sync_points();
  void handle_prune_sync_points(int r);

  void finish(int r);
};

} // namespace mirror
} // namespace rbd

extern template class rbd::mirror::ImageSync<librbd::ImageCtx>;

#endif // RBD_MIRROR_IMAGE_SYNC_H
