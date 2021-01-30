// ----------------------------------------------------------------------------
// -                        Open3D: www.open3d.org                            -
// ----------------------------------------------------------------------------
// The MIT License (MIT)
//
// Copyright (c) 2018 www.open3d.org
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
// ----------------------------------------------------------------------------

#include "open3d/visualization/gui/SceneWidget.h"

#include <imgui.h>

#include <Eigen/Geometry>
#include <set>
#include <unordered_set>

#include "open3d/geometry/BoundingVolume.h"
#include "open3d/visualization/gui/Application.h"
#include "open3d/visualization/gui/Color.h"
#include "open3d/visualization/gui/Events.h"
#include "open3d/visualization/gui/Label.h"
#include "open3d/visualization/gui/Label3D.h"
#include "open3d/visualization/gui/PickPointsInteractor.h"
#include "open3d/visualization/rendering/Camera.h"
#include "open3d/visualization/rendering/CameraInteractorLogic.h"
#include "open3d/visualization/rendering/IBLRotationInteractorLogic.h"
#include "open3d/visualization/rendering/LightDirectionInteractorLogic.h"
#include "open3d/visualization/rendering/ModelInteractorLogic.h"
#include "open3d/visualization/rendering/Open3DScene.h"
#include "open3d/visualization/rendering/Scene.h"
#include "open3d/visualization/rendering/View.h"

// Once render target is available, please remove the #ifdefs
#define NO_RENDER_TARGET 1

namespace open3d {
namespace visualization {
namespace gui {

static const double NEAR_PLANE = 0.1;
static const double MIN_FAR_PLANE = 1.0;

static const double DELAY_FOR_BEST_RENDERING_SECS = 0.2;  // seconds
// ----------------------------------------------------------------------------
class RotateSunInteractor : public SceneWidget::MouseInteractor {
public:
    RotateSunInteractor(rendering::Open3DScene* scene,
                        rendering::Camera* camera)
        : light_dir_(std::make_unique<rendering::LightDirectionInteractorLogic>(
                  scene->GetScene(), camera)) {}

    rendering::MatrixInteractorLogic& GetMatrixInteractor() override {
        return *light_dir_.get();
    }

    void SetOnSunLightChanged(
            std::function<void(const Eigen::Vector3f&)> on_changed) {
        on_light_dir_changed_ = on_changed;
    }

    void Mouse(const MouseEvent& e) override {
        switch (e.type) {
            case MouseEvent::BUTTON_DOWN:
                mouse_down_x_ = e.x;
                mouse_down_y_ = e.y;
                light_dir_->StartMouseDrag();
                break;
            case MouseEvent::DRAG: {
                int dx = e.x - mouse_down_x_;
                int dy = e.y - mouse_down_y_;
                light_dir_->Rotate(dx, dy);
                if (on_light_dir_changed_) {
                    on_light_dir_changed_(light_dir_->GetCurrentDirection());
                }
                break;
            }
            case MouseEvent::WHEEL: {
                break;
            }
            case MouseEvent::BUTTON_UP:
                light_dir_->EndMouseDrag();
                break;
            default:
                break;
        }
    }

    void Key(const KeyEvent& e) override {}

private:
    std::unique_ptr<rendering::LightDirectionInteractorLogic> light_dir_;
    int mouse_down_x_ = 0;
    int mouse_down_y_ = 0;
    std::function<void(const Eigen::Vector3f&)> on_light_dir_changed_;
};

class RotateIBLInteractor : public SceneWidget::MouseInteractor {
public:
    RotateIBLInteractor(rendering::Scene* scene, rendering::Camera* camera)
        : ibl_(std::make_unique<rendering::IBLRotationInteractorLogic>(
                  scene, camera)) {}

    rendering::MatrixInteractorLogic& GetMatrixInteractor() override {
        return *ibl_.get();
    }

    void ShowSkybox(bool is_on) { ibl_->ShowSkybox(is_on); }

    void SetOnChanged(std::function<void(const rendering::Camera::Transform&)>
                              on_changed) {
        on_rotation_changed_ = on_changed;
    }

    void Mouse(const MouseEvent& e) override {
        switch (e.type) {
            case MouseEvent::BUTTON_DOWN:
                mouse_down_x_ = e.x;
                mouse_down_y_ = e.y;
                ibl_->StartMouseDrag();
                break;
            case MouseEvent::DRAG: {
                int dx = e.x - mouse_down_x_;
                int dy = e.y - mouse_down_y_;
                if (e.modifiers & int(KeyModifier::META)) {
                    ibl_->RotateZ(dx, dy);
                } else {
                    ibl_->Rotate(dx, dy);
                }
                if (on_rotation_changed_) {
                    on_rotation_changed_(ibl_->GetCurrentRotation());
                }
                break;
            }
            case MouseEvent::WHEEL: {
                break;
            }
            case MouseEvent::BUTTON_UP:
                ibl_->EndMouseDrag();
                break;
            default:
                break;
        }
    }

    void Key(const KeyEvent& e) override {}

private:
    std::unique_ptr<rendering::IBLRotationInteractorLogic> ibl_;
    int mouse_down_x_ = 0;
    int mouse_down_y_ = 0;
    std::function<void(const rendering::Camera::Transform&)>
            on_rotation_changed_;
};

class FlyInteractor : public SceneWidget::MouseInteractor {
public:
    explicit FlyInteractor(rendering::Camera* camera)
        : camera_controls_(std::make_unique<rendering::CameraInteractorLogic>(
                  camera, MIN_FAR_PLANE)) {}

    rendering::MatrixInteractorLogic& GetMatrixInteractor() override {
        return *camera_controls_.get();
    }

    void Mouse(const MouseEvent& e) override {
        switch (e.type) {
            case MouseEvent::BUTTON_DOWN:
                last_mouse_x_ = e.x;
                last_mouse_y_ = e.y;
                camera_controls_->StartMouseDrag();
                break;
            case MouseEvent::DRAG: {
                // Use relative movement because user may be moving
                // with keys at the same time.
                int dx = e.x - last_mouse_x_;
                int dy = e.y - last_mouse_y_;
                if (e.modifiers & int(KeyModifier::META)) {
                    // RotateZ() was not intended to be used for relative
                    // movement, so reset the mouse-down matrix first.
                    camera_controls_->ResetMouseDrag();
                    camera_controls_->RotateZ(dx, dy);
                } else {
                    camera_controls_->RotateFly(-dx, -dy);
                }
                last_mouse_x_ = e.x;
                last_mouse_y_ = e.y;
                break;
            }
            case MouseEvent::WHEEL: {
                break;
            }
            case MouseEvent::BUTTON_UP:
                camera_controls_->EndMouseDrag();
                break;
            default:
                break;
        }
    }

    void Key(const KeyEvent& e) override {
        switch (e.type) {
            case KeyEvent::Type::DOWN:
                keys_down_.insert(e.key);
                break;
            case KeyEvent::Type::UP:
                keys_down_.erase(e.key);
                break;
        }
    }

    bool Tick(const TickEvent& e) override {
        bool redraw = false;
        if (!keys_down_.empty()) {
            auto& bounds = camera_controls_->GetBoundingBox();
            const float dist = float(0.0025 * bounds.GetExtent().norm());
            const float angle_rad = 0.0075f;

            auto HasKey = [this](uint32_t key) -> bool {
                return (keys_down_.find(key) != keys_down_.end());
            };

            auto move = [this, &redraw](const Eigen::Vector3f& v) {
                camera_controls_->MoveLocal(v);
                redraw = true;
            };
            auto rotate = [this, &redraw](float angle_rad,
                                          const Eigen::Vector3f& axis) {
                camera_controls_->RotateLocal(angle_rad, axis);
                redraw = true;
            };
            auto rotateZ = [this, &redraw](int dy) {
                camera_controls_->StartMouseDrag();
                camera_controls_->RotateZ(0, dy);
                redraw = true;
            };

            if (HasKey('a')) {
                move({-dist, 0, 0});
            }
            if (HasKey('d')) {
                move({dist, 0, 0});
            }
            if (HasKey('w')) {
                move({0, 0, -dist});
            }
            if (HasKey('s')) {
                move({0, 0, dist});
            }
            if (HasKey('q')) {
                move({0, dist, 0});
            }
            if (HasKey('z')) {
                move({0, -dist, 0});
            }
            if (HasKey('e')) {
                rotateZ(-2);
            }
            if (HasKey('r')) {
                rotateZ(2);
            }
            if (HasKey(KEY_UP)) {
                rotate(angle_rad, {1, 0, 0});
            }
            if (HasKey(KEY_DOWN)) {
                rotate(-angle_rad, {1, 0, 0});
            }
            if (HasKey(KEY_LEFT)) {
                rotate(angle_rad, {0, 1, 0});
            }
            if (HasKey(KEY_RIGHT)) {
                rotate(-angle_rad, {0, 1, 0});
            }
        }
        return redraw;
    }

private:
    std::unique_ptr<rendering::CameraInteractorLogic> camera_controls_;
    int last_mouse_x_ = 0;
    int last_mouse_y_ = 0;
    std::set<uint32_t> keys_down_;
};

class RotationInteractor : public SceneWidget::MouseInteractor {
protected:
    void SetInteractor(rendering::RotationInteractorLogic* r) {
        interactor_ = r;
    }

public:
    rendering::MatrixInteractorLogic& GetMatrixInteractor() override {
        return *interactor_;
    }

    void SetCenterOfRotation(const Eigen::Vector3f& center) {
        interactor_->SetCenterOfRotation(center);
    }

    void Mouse(const MouseEvent& e) override {
        switch (e.type) {
            case MouseEvent::BUTTON_DOWN:
                mouse_down_x_ = e.x;
                mouse_down_y_ = e.y;
                if (e.button.button == MouseButton::LEFT) {
                    if (e.modifiers & int(KeyModifier::SHIFT)) {
#ifdef __APPLE__
                        if (e.modifiers & int(KeyModifier::ALT)) {
#else
                        if (e.modifiers & int(KeyModifier::CTRL)) {
#endif  // __APPLE__
                            state_ = State::ROTATE_Z;
                        } else {
                            state_ = State::DOLLY;
                        }
                    } else if (e.modifiers & int(KeyModifier::CTRL)) {
                        state_ = State::PAN;
                    } else if (e.modifiers & int(KeyModifier::META)) {
                        state_ = State::ROTATE_Z;
                    } else {
                        state_ = State::ROTATE_XY;
                    }
                } else if (e.button.button == MouseButton::RIGHT) {
                    state_ = State::PAN;
                }
                interactor_->StartMouseDrag();
                break;
            case MouseEvent::DRAG: {
                int dx = e.x - mouse_down_x_;
                int dy = e.y - mouse_down_y_;
                switch (state_) {
                    case State::NONE:
                        break;
                    case State::PAN:
                        interactor_->Pan(dx, dy);
                        break;
                    case State::DOLLY:
                        interactor_->Dolly(dy,
                                           rendering::MatrixInteractorLogic::
                                                   DragType::MOUSE);
                        break;
                    case State::ROTATE_XY:
                        interactor_->Rotate(dx, dy);
                        break;
                    case State::ROTATE_Z:
                        interactor_->RotateZ(dx, dy);
                        break;
                }
                interactor_->UpdateMouseDragUI();
                break;
            }
            case MouseEvent::WHEEL: {
                interactor_->Dolly(2.0f * e.wheel.dy,
                                   e.wheel.isTrackpad
                                           ? rendering::MatrixInteractorLogic::
                                                     DragType::TWO_FINGER
                                           : rendering::MatrixInteractorLogic::
                                                     DragType::WHEEL);
                break;
            }
            case MouseEvent::BUTTON_UP:
                interactor_->EndMouseDrag();
                state_ = State::NONE;
                break;
            default:
                break;
        }
    }

    void Key(const KeyEvent& e) override {}

protected:
    rendering::RotationInteractorLogic* interactor_ = nullptr;
    int mouse_down_x_ = 0;
    int mouse_down_y_ = 0;

    enum class State { NONE, PAN, DOLLY, ROTATE_XY, ROTATE_Z };
    State state_ = State::NONE;
};

class RotateModelInteractor : public RotationInteractor {
    using Super = RotationInteractor;

public:
    explicit RotateModelInteractor(rendering::Open3DScene* scene,
                                   rendering::Camera* camera)
        : RotationInteractor(),
          rotation_(new rendering::ModelInteractorLogic(
                  scene, camera, MIN_FAR_PLANE)) {
        SetInteractor(rotation_.get());
    }

    void Mouse(const MouseEvent& e) override { Super::Mouse(e); }

private:
    std::unique_ptr<rendering::ModelInteractorLogic> rotation_;
};

class RotateCameraInteractor : public RotationInteractor {
    using Super = RotationInteractor;

public:
    explicit RotateCameraInteractor(rendering::Camera* camera)
        : camera_controls_(std::make_unique<rendering::CameraInteractorLogic>(
                  camera, MIN_FAR_PLANE)) {
        SetInteractor(camera_controls_.get());
    }

    void Mouse(const MouseEvent& e) override {
        switch (e.type) {
            case MouseEvent::BUTTON_DOWN:
            case MouseEvent::DRAG:
            case MouseEvent::BUTTON_UP:
            default:
                Super::Mouse(e);
                break;
            case MouseEvent::WHEEL: {
                if (e.modifiers == int(KeyModifier::SHIFT)) {
                    camera_controls_->Zoom(
                            e.wheel.dy,
                            e.wheel.isTrackpad
                                    ? rendering::MatrixInteractorLogic::
                                              DragType::TWO_FINGER
                                    : rendering::MatrixInteractorLogic::
                                              DragType::WHEEL);
                } else {
                    Super::Mouse(e);
                }
                break;
            }
        }
    }

private:
    std::unique_ptr<rendering::CameraInteractorLogic> camera_controls_;
};

class PickInteractor : public RotateCameraInteractor {
    using Super = RotateCameraInteractor;

public:
    PickInteractor(rendering::Open3DScene* scene, rendering::Camera* camera)
        : Super(camera), pick_(new PickPointsInteractor(scene, camera)) {}

    void SetViewSize(const Size& size) {
        GetMatrixInteractor().SetViewSize(size.width, size.height);
        pick_->GetMatrixInteractor().SetViewSize(size.width, size.height);
    }

    void SetPickableGeometry(
            const std::vector<SceneWidget::PickableGeometry>& geometry) {
        pick_->SetPickableGeometry(geometry);
    }

    void SetPickablePointSize(int px) { pick_->SetPointSize(px); }

    void SetOnPointsPicked(
            std::function<void(
                    const std::map<
                            std::string,
                            std::vector<std::pair<size_t, Eigen::Vector3d>>>&,
                    int)> on_picked) {
        pick_->SetOnPointsPicked(on_picked);
    }

    void SetNeedsRedraw() { pick_->SetNeedsRedraw(); }

    void Mouse(const MouseEvent& e) override {
        if (e.modifiers & int(KeyModifier::CTRL)) {
            pick_->Mouse(e);
        } else {
            Super::Mouse(e);
            pick_->SetNeedsRedraw();
        }
    }

private:
    std::unique_ptr<PickPointsInteractor> pick_;
};

// ----------------------------------------------------------------------------
class Interactors {
public:
    Interactors(rendering::Open3DScene* scene, rendering::Camera* camera)
        : rotate_(std::make_unique<RotateCameraInteractor>(camera)),
          fly_(std::make_unique<FlyInteractor>(camera)),
          sun_(std::make_unique<RotateSunInteractor>(scene, camera)),
          ibl_(std::make_unique<RotateIBLInteractor>(scene->GetScene(),
                                                     camera)),
          model_(std::make_unique<RotateModelInteractor>(scene, camera)),
          pick_(std::make_unique<PickInteractor>(scene, camera)) {
        current_ = rotate_.get();
    }

    void SetViewSize(const Size& size) {
        rotate_->GetMatrixInteractor().SetViewSize(size.width, size.height);
        fly_->GetMatrixInteractor().SetViewSize(size.width, size.height);
        sun_->GetMatrixInteractor().SetViewSize(size.width, size.height);
        ibl_->GetMatrixInteractor().SetViewSize(size.width, size.height);
        model_->GetMatrixInteractor().SetViewSize(size.width, size.height);
        pick_->SetViewSize(size);
    }

    void SetBoundingBox(const geometry::AxisAlignedBoundingBox& bounds) {
        rotate_->GetMatrixInteractor().SetBoundingBox(bounds);
        fly_->GetMatrixInteractor().SetBoundingBox(bounds);
        sun_->GetMatrixInteractor().SetBoundingBox(bounds);
        ibl_->GetMatrixInteractor().SetBoundingBox(bounds);
        model_->GetMatrixInteractor().SetBoundingBox(bounds);
        pick_->GetMatrixInteractor().SetBoundingBox(bounds);
    }

    void SetCenterOfRotation(const Eigen::Vector3f& center) {
        rotate_->SetCenterOfRotation(center);
    }

    void SetOnSunLightChanged(
            std::function<void(const Eigen::Vector3f&)> onChanged) {
        sun_->SetOnSunLightChanged(onChanged);
    }

    void ShowSkybox(bool isOn) { ibl_->ShowSkybox(isOn); }

    void SetSunInteractorEnabled(bool enable) {
        sun_interactor_enabled_ = enable;
    }

    void SetPickableGeometry(
            const std::vector<SceneWidget::PickableGeometry>& geometry) {
        pick_->SetPickableGeometry(geometry);
    }

    void SetPickablePointSize(int px) { pick_->SetPickablePointSize(px); }

    void SetOnPointsPicked(
            std::function<void(
                    const std::map<
                            std::string,
                            std::vector<std::pair<size_t, Eigen::Vector3d>>>&,
                    int)> on_picked) {
        pick_->SetOnPointsPicked(on_picked);
    }

    void SetPickNeedsRedraw() { pick_->SetNeedsRedraw(); }

    SceneWidget::Controls GetControls() const {
        if (current_ == fly_.get()) {
            return SceneWidget::Controls::FLY;
        } else if (current_ == sun_.get()) {
            return SceneWidget::Controls::ROTATE_SUN;
        } else if (current_ == ibl_.get()) {
            return SceneWidget::Controls::ROTATE_IBL;
        } else if (current_ == model_.get()) {
            return SceneWidget::Controls::ROTATE_MODEL;
        } else if (current_ == pick_.get()) {
            return SceneWidget::Controls::PICK_POINTS;
        } else {
            return SceneWidget::Controls::ROTATE_CAMERA;
        }
    }

    void SetControls(SceneWidget::Controls mode) {
        switch (mode) {
            case SceneWidget::Controls::ROTATE_CAMERA:
                current_ = rotate_.get();
                break;
            case SceneWidget::Controls::FLY:
                current_ = fly_.get();
                break;
            case SceneWidget::Controls::ROTATE_SUN:
                current_ = sun_.get();
                break;
            case SceneWidget::Controls::ROTATE_IBL:
                current_ = ibl_.get();
                break;
            case SceneWidget::Controls::ROTATE_MODEL:
                current_ = model_.get();
                break;
            case SceneWidget::Controls::PICK_POINTS:
                current_ = pick_.get();
                break;
        }
    }

    void Mouse(const MouseEvent& e) {
        if (current_ == rotate_.get() && sun_interactor_enabled_) {
            if (e.type == MouseEvent::Type::BUTTON_DOWN &&
                (e.button.button == MouseButton::MIDDLE ||
                 e.modifiers == int(KeyModifier::ALT))) {
                override_ = sun_.get();
            }
        }

        if (override_) {
            override_->Mouse(e);
        } else if (current_) {
            current_->Mouse(e);
        }

        if (override_ && e.type == MouseEvent::Type::BUTTON_UP) {
            override_ = nullptr;
        }
    }

    void Key(const KeyEvent& e) {
        if (current_) {
            current_->Key(e);
        }
    }

    Widget::DrawResult Tick(const TickEvent& e) {
        if (current_) {
            if (current_->Tick(e)) {
                return Widget::DrawResult::REDRAW;
            }
        }
        return Widget::DrawResult::NONE;
    }

private:
    bool sun_interactor_enabled_ = true;

    std::unique_ptr<RotateCameraInteractor> rotate_;
    std::unique_ptr<FlyInteractor> fly_;
    std::unique_ptr<RotateSunInteractor> sun_;
    std::unique_ptr<RotateIBLInteractor> ibl_;
    std::unique_ptr<RotateModelInteractor> model_;
    std::unique_ptr<PickInteractor> pick_;

    SceneWidget::MouseInteractor* current_ = nullptr;
    SceneWidget::MouseInteractor* override_ = nullptr;
};

// ----------------------------------------------------------------------------
struct SceneWidget::Impl {
    std::shared_ptr<rendering::Open3DScene> scene_;
    geometry::AxisAlignedBoundingBox bounds_;
    std::shared_ptr<Interactors> controls_;
    std::function<void(const Eigen::Vector3f&)> on_light_dir_changed_;
    std::function<void(rendering::Camera*)> on_camera_changed_;
    int buttons_down_ = 0;
    double last_fast_time_ = 0.0;
    bool frame_rect_changed_ = false;
    SceneWidget::Quality current_render_quality_ = SceneWidget::Quality::BEST;
    bool scene_caching_enabled_ = false;
#ifdef NO_RENDER_TARGET
    bool is_picking_ = false;
#endif  // NO_RENDER_TARGET

    std::unordered_set<std::shared_ptr<Label3D>> labels_3d_;
};

SceneWidget::SceneWidget() : impl_(new Impl()) {}

SceneWidget::~SceneWidget() {
    SetScene(nullptr);  // will do any necessary cleanup
}

void SceneWidget::SetFrame(const Rect& f) {
    Super::SetFrame(f);

    impl_->controls_->SetViewSize(Size(f.width, f.height));

    // We need to update the viewport and camera, but we can't do it here
    // because we need to know the window height to convert the frame
    // to OpenGL coordinates. We will actually do the updating in Draw().
    impl_->frame_rect_changed_ = true;
}

void SceneWidget::SetupCamera(
        float verticalFoV,
        const geometry::AxisAlignedBoundingBox& geometry_bounds,
        const Eigen::Vector3f& center_of_rotation) {
    impl_->bounds_ = geometry_bounds;
    impl_->controls_->SetBoundingBox(geometry_bounds);

    GoToCameraPreset(CameraPreset::PLUS_Z);  // default OpenGL view

    auto f = GetFrame();
    float aspect = 1.0f;
    if (f.height > 0) {
        aspect = float(f.width) / float(f.height);
    }
    // The far plane needs to be the max absolute distance, not just the
    // max extent, so that axes are visible if requested.
    // See also RotationInteractorLogic::UpdateCameraFarPlane().
    auto far1 = impl_->bounds_.GetMinBound().norm();
    auto far2 = impl_->bounds_.GetMaxBound().norm();
    auto far3 =
            GetCamera()->GetModelMatrix().translation().cast<double>().norm();
    auto model_size = 2.0 * impl_->bounds_.GetExtent().norm();
    auto far = std::max(MIN_FAR_PLANE,
                        std::max(std::max(far1, far2), far3) + model_size);
    GetCamera()->SetProjection(verticalFoV, aspect, NEAR_PLANE, far,
                               rendering::Camera::FovType::Vertical);
}

void SceneWidget::SetOnCameraChanged(
        std::function<void(rendering::Camera*)> on_cam_changed) {
    impl_->on_camera_changed_ = on_cam_changed;
}

void SceneWidget::SetOnSunDirectionChanged(
        std::function<void(const Eigen::Vector3f&)> on_dir_changed) {
    impl_->on_light_dir_changed_ = on_dir_changed;
    impl_->controls_->SetOnSunLightChanged([this](const Eigen::Vector3f& dir) {
        impl_->scene_->GetScene()->SetSunLightDirection(dir);
        if (impl_->on_light_dir_changed_) {
            impl_->on_light_dir_changed_(dir);
        }
    });
}

void SceneWidget::ShowSkybox(bool is_on) {
    impl_->controls_->ShowSkybox(is_on);
}

void SceneWidget::SetSunInteractorEnabled(bool enable) {
    impl_->controls_->SetSunInteractorEnabled(enable);
}

void SceneWidget::SetPickableGeometry(
        const std::vector<PickableGeometry>& geometry) {
    impl_->controls_->SetPickableGeometry(geometry);
}

void SceneWidget::SetPickablePointSize(int px) {
    impl_->controls_->SetPickablePointSize(px);
}

void SceneWidget::SetOnPointsPicked(
        std::function<
                void(const std::map<
                             std::string,
                             std::vector<std::pair<size_t, Eigen::Vector3d>>>&,
                     int)> on_picked) {
    impl_->controls_->SetOnPointsPicked(on_picked);
}

void SceneWidget::SetScene(std::shared_ptr<rendering::Open3DScene> scene) {
    impl_->scene_ = scene;
    if (impl_->scene_) {
        auto view = impl_->scene_->GetView();
        impl_->controls_ = std::make_shared<Interactors>(impl_->scene_.get(),
                                                         view->GetCamera());
    }
}

std::shared_ptr<rendering::Open3DScene> SceneWidget::GetScene() const {
    return impl_->scene_;
}

rendering::View* SceneWidget::GetRenderView() const {
    if (impl_->scene_) {
        return impl_->scene_->GetView();
    } else {
        return nullptr;
    }
}

void SceneWidget::SetViewControls(Controls mode) {
    if (mode == Controls::ROTATE_CAMERA &&
        impl_->controls_->GetControls() == Controls::FLY) {
        impl_->controls_->SetControls(mode);
        // If we're going from fly to standard rotate obj, we need to
        // adjust the center of rotation or it will jump to a different
        // matrix rather abruptly. The center of rotation is used for the
        // panning distance so that the cursor stays in roughly the same
        // position as the user moves the mouse. Use the distance to the
        // center of the model, which should be reasonable.
        auto camera = GetCamera();
        Eigen::Vector3f to_center = impl_->bounds_.GetCenter().cast<float>() -
                                    camera->GetPosition();
        Eigen::Vector3f forward = camera->GetForwardVector();
        Eigen::Vector3f center =
                camera->GetPosition() + to_center.norm() * forward;
        impl_->controls_->SetCenterOfRotation(center);
    } else {
        impl_->controls_->SetControls(mode);
    }

#if NO_RENDER_TARGET
    if (mode == Controls::PICK_POINTS) {
        impl_->is_picking_ = true;
    }
    EnableSceneCaching(impl_->scene_caching_enabled_);
#endif  // NO_RENDER_TARGET
}

void SceneWidget::EnableSceneCaching(bool enable) {
    impl_->scene_caching_enabled_ = enable;
#if NO_RENDER_TARGET
    if (impl_->is_picking_) {
        enable = false;
    }
#endif
    if (!enable) {
        impl_->scene_->GetRenderer().EnableCaching(false);
        impl_->scene_->GetScene()->SetViewActive(impl_->scene_->GetViewId(),
                                                 true);
    }
}

void SceneWidget::ForceRedraw() {
    // ForceRedraw only applies when scene caching is enabled
#if NO_RENDER_TARGET
    if (!impl_->scene_caching_enabled_ || impl_->is_picking_) return;
#else
    if (!impl_->scene_caching_enabled_) return;
#endif  // NO_RENDER_TARGET

    impl_->scene_->GetRenderer().EnableCaching(true);
    impl_->scene_->GetScene()->SetRenderOnce(impl_->scene_->GetViewId());
    impl_->controls_->SetPickNeedsRedraw();
}

void SceneWidget::SetRenderQuality(Quality quality) {
    auto currentQuality = GetRenderQuality();
    if (currentQuality != quality) {
        impl_->current_render_quality_ = quality;
        if (quality == Quality::FAST) {
            impl_->scene_->SetLOD(rendering::Open3DScene::LOD::FAST);
#if NO_RENDER_TARGET
            if (impl_->scene_caching_enabled_ && !impl_->is_picking_) {
#else
            if (impl_->scene_caching_enabled_) {
#endif  // NO_RENDER_TARGET
                impl_->scene_->GetRenderer().EnableCaching(false);
                impl_->scene_->GetScene()->SetViewActive(
                        impl_->scene_->GetViewId(), true);
            }
        } else {
            impl_->scene_->SetLOD(rendering::Open3DScene::LOD::HIGH_DETAIL);
#if NO_RENDER_TARGET
            if (impl_->scene_caching_enabled_ && !impl_->is_picking_) {
#else
            if (impl_->scene_caching_enabled_) {
#endif  // NO_RENDER_TARGET
                impl_->scene_->GetRenderer().EnableCaching(true);
                impl_->scene_->GetScene()->SetRenderOnce(
                        impl_->scene_->GetViewId());
            }
        }
    }
}

SceneWidget::Quality SceneWidget::GetRenderQuality() const {
    return impl_->current_render_quality_;
}

void SceneWidget::GoToCameraPreset(CameraPreset preset) {
    // To get the eye position we move maxDim away from the center in the
    // appropriate direction. We cannot simply use maxDim as that value
    // for that dimension, because the model may not be centered around
    // (0, 0, 0), and this will result in the far plane being not being
    // far enough and clipping the model. To test, use
    // https://docs.google.com/uc?export=download&id=0B-ePgl6HF260ODdvT09Xc1JxOFE
    float max_dim = float(1.25 * impl_->bounds_.GetMaxExtent());
    Eigen::Vector3f center = impl_->bounds_.GetCenter().cast<float>();
    Eigen::Vector3f eye, up;
    switch (preset) {
        case CameraPreset::PLUS_X: {
            eye = Eigen::Vector3f(center.x() + max_dim, center.y(), center.z());
            up = Eigen::Vector3f(0, 1, 0);
            break;
        }
        case CameraPreset::PLUS_Y: {
            eye = Eigen::Vector3f(center.x(), center.y() + max_dim, center.z());
            up = Eigen::Vector3f(1, 0, 0);
            break;
        }
        case CameraPreset::PLUS_Z: {
            eye = Eigen::Vector3f(center.x(), center.y(), center.z() + max_dim);
            up = Eigen::Vector3f(0, 1, 0);
            break;
        }
    }
    GetCamera()->LookAt(center, eye, up);
    impl_->controls_->SetCenterOfRotation(center);
    ForceRedraw();
}

rendering::Camera* SceneWidget::GetCamera() const {
    return impl_->scene_->GetCamera();
}

std::shared_ptr<Label3D> SceneWidget::AddLabel(const Eigen::Vector3f& pos,
                                               const char* text) {
    auto l = std::make_shared<Label3D>(pos, text);
    impl_->labels_3d_.insert(l);
    return l;
}

void SceneWidget::RemoveLabel(std::shared_ptr<Label3D> label) {
    auto liter = impl_->labels_3d_.find(label);
    if (liter != impl_->labels_3d_.end()) {
        impl_->labels_3d_.erase(liter);
    }
}

void SceneWidget::Layout(const Theme& theme) {
    Super::Layout(theme);
    // The UI may have changed size such that the scene has been exposed. Need
    // to force a redraw in that case.

    ForceRedraw();
}

Widget::DrawResult SceneWidget::Draw(const DrawContext& context) {
    // If the widget has changed size we need to update the viewport and the
    // camera. We can't do it in SetFrame() because we need to know the height
    // of the window to convert to OpenGL coordinates for the viewport.
    if (impl_->frame_rect_changed_) {
        impl_->frame_rect_changed_ = false;

        auto f = GetFrame();
        impl_->controls_->SetViewSize(Size(f.width, f.height));
        // GUI has origin of Y axis at top, but renderer has it at bottom
        // so we need to convert coordinates.
        int y = context.screenHeight - (f.height + f.y);

        auto view = impl_->scene_->GetView();
        view->SetViewport(f.x, y, f.width, f.height);

        auto* camera = GetCamera();
        float aspect = 1.0f;
        if (f.height > 0) {
            aspect = float(f.width) / float(f.height);
        }
        GetCamera()->SetProjection(camera->GetFieldOfView(), aspect,
                                   camera->GetNear(), camera->GetFar(),
                                   camera->GetFieldOfViewType());

        impl_->controls_->SetPickNeedsRedraw();
    }

    if (!impl_->labels_3d_.empty()) {
        const auto f = GetFrame();
        // Setup ImGUI
        ImGui::SetNextWindowPos(ImVec2(float(f.x), float(f.y)));
        ImGui::SetNextWindowSize(ImVec2(float(f.width), float(f.height)));
        ImGui::Begin("3D Labels", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                             ImGuiWindowFlags_NoNav |
                             ImGuiWindowFlags_NoBackground);

        // Draw each text label
        for (const auto& l : impl_->labels_3d_) {
            auto ndc = GetCamera()->GetNDC(l->GetPosition());
            ndc += Eigen::Vector2f::Ones();
            ndc *= 0.5f;
            ndc.x() *= f.width;
            ndc.y() *= f.height;
            ImGui::SetCursorScreenPos(
                    ImVec2(ndc.x() - f.x, f.height - ndc.y() - f.y));
            auto color = l->GetTextColor();
            ImGui::TextColored({color.GetRed(), color.GetGreen(),
                                color.GetBlue(), color.GetAlpha()},
                               "%s", l->GetText());
        }

        ImGui::End();
    }

    // The actual drawing is done later, at the end of drawing in
    // Window::OnDraw(), in FilamentRenderer::Draw(). We can always
    // return NONE because any changes this frame will automatically
    // be rendered (unlike the ImGUI parts).
    return Widget::DrawResult::NONE;
}

Widget::EventResult SceneWidget::Mouse(const MouseEvent& e) {
    // Lower render quality while rotating, since we will be redrawing
    // frequently. This will give a snappier feel to mouse movements,
    // especially for point clouds, which are a little slow.
    if (e.type != MouseEvent::MOVE) {
        SetRenderQuality(Quality::FAST);
    }
    // Render quality will revert back to BEST after a short delay,
    // unless the user starts rotating again, or is scroll-wheeling.
    if (e.type == MouseEvent::DRAG || e.type == MouseEvent::WHEEL) {
        impl_->last_fast_time_ = Application::GetInstance().Now();
    }

    if (e.type == MouseEvent::BUTTON_DOWN) {
        impl_->buttons_down_ |= int(e.button.button);
    } else if (e.type == MouseEvent::BUTTON_UP) {
        impl_->buttons_down_ &= ~int(e.button.button);
    }

    auto& frame = GetFrame();
    MouseEvent local = e;
    local.x -= frame.x;
    local.y -= frame.y;
    impl_->controls_->Mouse(local);

    if (impl_->on_camera_changed_) {
        impl_->on_camera_changed_(GetCamera());
    }

    return Widget::EventResult::CONSUMED;
}

Widget::EventResult SceneWidget::Key(const KeyEvent& e) {
    impl_->controls_->Key(e);

    if (impl_->on_camera_changed_) {
        impl_->on_camera_changed_(GetCamera());
    }
    return Widget::EventResult::CONSUMED;
}

Widget::DrawResult SceneWidget::Tick(const TickEvent& e) {
    auto result = impl_->controls_->Tick(e);
    // If Tick() redraws, then a key is down. Make sure we are rendering
    // FAST and mark the time so that we don't timeout and revert back
    // to slow rendering before the key up happens.
    if (result == Widget::DrawResult::REDRAW) {
        SetRenderQuality(Quality::FAST);
        impl_->last_fast_time_ = Application::GetInstance().Now();
    }
    if (impl_->buttons_down_ == 0 && GetRenderQuality() == Quality::FAST) {
        double now = Application::GetInstance().Now();
        if (now - impl_->last_fast_time_ > DELAY_FOR_BEST_RENDERING_SECS) {
            SetRenderQuality(Quality::BEST);
            result = Widget::DrawResult::REDRAW;
        }
    }
    return result;
}

}  // namespace gui
}  // namespace visualization
}  // namespace open3d
