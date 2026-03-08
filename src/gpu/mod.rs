use std::cmp::Ordering;

use glam::{Mat4, Vec3};
use raw_window_handle::{RawDisplayHandle, RawWindowHandle};
use vello::kurbo::{Affine, BezPath, Rect, Stroke};
use vello::peniko::{Color, Fill};
use vello::{AaConfig, RenderParams, Renderer, RendererOptions, Scene};
use wgpu::util::TextureBlitter;

pub struct Gpu {
    pub adapter: wgpu::Adapter,
    instance: wgpu::Instance,
    device: wgpu::Device,
    queue: wgpu::Queue,
    renderer: Renderer,
}

pub struct Viewport {
    surface: wgpu::Surface<'static>,
    config: wgpu::SurfaceConfiguration,
    target_texture: wgpu::Texture,
    target_view: wgpu::TextureView,
    blitter: TextureBlitter,
    scene: Scene,
    camera: Camera,
}

#[derive(Clone, Copy)]
struct Camera {
    target: Vec3,
    yaw: f32,
    pitch: f32,
    distance: f32,
    fov_y_radians: f32,
}

pub fn init() -> Option<Gpu> {
    #[cfg(not(target_arch = "wasm32"))]
    {
        let _ = env_logger::try_init();
    }

    #[cfg(target_arch = "wasm32")]
    {
        console_log::init_with_level(log::Level::Info).unwrap_throw();
    }

    let instance = wgpu::Instance::new(&wgpu::InstanceDescriptor {
        backends: wgpu::Backends::all(),
        ..Default::default()
    });

    let adapter = pollster::block_on(instance.request_adapter(&wgpu::RequestAdapterOptions {
        power_preference: wgpu::PowerPreference::HighPerformance,
        compatible_surface: None,
        force_fallback_adapter: false,
    }))
    .expect("Failed to find an appropriate GPU adapter");

    println!("Found adapter: {}", adapter.get_info().name);

    let (device, queue) =
        pollster::block_on(adapter.request_device(&wgpu::DeviceDescriptor::default())).ok()?;
    let renderer = Renderer::new(&device, RendererOptions::default()).ok()?;

    Some(Gpu {
        instance,
        adapter,
        device,
        queue,
        renderer,
    })
}

fn create_target_texture(
    device: &wgpu::Device,
    width: u32,
    height: u32,
) -> (wgpu::Texture, wgpu::TextureView) {
    let texture = device.create_texture(&wgpu::TextureDescriptor {
        label: Some("vello-target-texture"),
        size: wgpu::Extent3d {
            width,
            height,
            depth_or_array_layers: 1,
        },
        mip_level_count: 1,
        sample_count: 1,
        dimension: wgpu::TextureDimension::D2,
        format: wgpu::TextureFormat::Rgba8Unorm,
        usage: wgpu::TextureUsages::STORAGE_BINDING | wgpu::TextureUsages::TEXTURE_BINDING,
        view_formats: &[],
    });
    let view = texture.create_view(&wgpu::TextureViewDescriptor::default());
    (texture, view)
}

#[derive(Clone, Copy)]
struct ProjectedPoint {
    x: f64,
    y: f64,
    ndc_z: f32,
}

#[derive(Clone, Copy)]
struct FaceInfo {
    projected: [ProjectedPoint; 4],
    world: [Vec3; 4],
    normal: Vec3,
    avg_depth: f32,
}

const CUBE_VERTICES: [Vec3; 8] = [
    Vec3::new(-1.0, -1.0, -1.0),
    Vec3::new(1.0, -1.0, -1.0),
    Vec3::new(1.0, 1.0, -1.0),
    Vec3::new(-1.0, 1.0, -1.0),
    Vec3::new(-1.0, -1.0, 1.0),
    Vec3::new(1.0, -1.0, 1.0),
    Vec3::new(1.0, 1.0, 1.0),
    Vec3::new(-1.0, 1.0, 1.0),
];

const CUBE_HALF_EXTENT: f32 = 1.25;
const CUBE_FACES: [[usize; 4]; 6] = [
    [4, 5, 6, 7], // front (+Z)
    [1, 0, 3, 2], // back (-Z)
    [5, 1, 2, 6], // right (+X)
    [0, 4, 7, 3], // left (-X)
    [7, 6, 2, 3], // top (+Y)
    [0, 1, 5, 4], // bottom (-Y)
];

fn default_camera() -> Camera {
    let eye = Vec3::new(3.3, 2.5, 4.8);
    let target = Vec3::ZERO;
    let offset = eye - target;
    let distance = offset.length().max(0.01);
    let yaw = offset.x.atan2(offset.z);
    let pitch = (offset.y / distance).asin();

    Camera {
        target,
        yaw,
        pitch,
        distance,
        fov_y_radians: 40.0f32.to_radians(),
    }
}

fn camera_eye(camera: &Camera) -> Vec3 {
    let cp = camera.pitch.cos();
    let direction = Vec3::new(
        camera.yaw.sin() * cp,
        camera.pitch.sin(),
        camera.yaw.cos() * cp,
    );
    camera.target + direction * camera.distance
}

fn camera_basis(camera: &Camera) -> (Vec3, Vec3, Vec3, Vec3) {
    let eye = camera_eye(camera);
    let forward = (camera.target - eye).normalize_or_zero();
    let mut right = forward.cross(Vec3::Y).normalize_or_zero();
    if right.length_squared() < 1e-8 {
        right = Vec3::X;
    }
    let up = right.cross(forward).normalize_or_zero();
    (eye, forward, right, up)
}

fn project_point(world: Vec3, view_proj: Mat4, width: u32, height: u32) -> Option<ProjectedPoint> {
    let clip = view_proj * world.extend(1.0);
    if clip.w <= 0.0001 || !clip.w.is_finite() {
        return None;
    }

    let ndc = clip.truncate() / clip.w;
    if !ndc.x.is_finite() || !ndc.y.is_finite() || !ndc.z.is_finite() {
        return None;
    }
    let x = ((ndc.x as f64) * 0.5 + 0.5) * width as f64;
    let y = (1.0 - ((ndc.y as f64) * 0.5 + 0.5)) * height as f64;

    Some(ProjectedPoint { x, y, ndc_z: ndc.z })
}

fn clip_halfspace(g0: f32, g1: f32, t_min: &mut f32, t_max: &mut f32) -> bool {
    if !g0.is_finite() || !g1.is_finite() {
        return false;
    }
    if g0 >= 0.0 && g1 >= 0.0 {
        return true;
    }
    if g0 < 0.0 && g1 < 0.0 {
        return false;
    }

    let denom = g0 - g1;
    if denom.abs() < 1e-8 {
        return false;
    }

    let t = (g0 / denom).clamp(0.0, 1.0);
    if g0 < 0.0 {
        *t_min = t_min.max(t);
    } else {
        *t_max = t_max.min(t);
    }

    *t_min <= *t_max
}

fn clip_segment_to_view_volume(a: Vec3, b: Vec3, view_proj: Mat4) -> Option<(Vec3, Vec3)> {
    let clip_a = view_proj * a.extend(1.0);
    let clip_b = view_proj * b.extend(1.0);

    let mut t_min = 0.0f32;
    let mut t_max = 1.0f32;

    let epsilon_w = 0.0001f32;

    // w > 0 (in front of camera)
    if !clip_halfspace(
        clip_a.w - epsilon_w,
        clip_b.w - epsilon_w,
        &mut t_min,
        &mut t_max,
    ) {
        return None;
    }

    // left/right clip planes: -w <= x <= w
    if !clip_halfspace(
        clip_a.x + clip_a.w,
        clip_b.x + clip_b.w,
        &mut t_min,
        &mut t_max,
    ) {
        return None;
    }
    if !clip_halfspace(
        clip_a.w - clip_a.x,
        clip_b.w - clip_b.x,
        &mut t_min,
        &mut t_max,
    ) {
        return None;
    }

    // bottom/top clip planes: -w <= y <= w
    if !clip_halfspace(
        clip_a.y + clip_a.w,
        clip_b.y + clip_b.w,
        &mut t_min,
        &mut t_max,
    ) {
        return None;
    }
    if !clip_halfspace(
        clip_a.w - clip_a.y,
        clip_b.w - clip_b.y,
        &mut t_min,
        &mut t_max,
    ) {
        return None;
    }

    // near plane (z >= 0 for wgpu-style [0, w] clip depth)
    if !clip_halfspace(clip_a.z, clip_b.z, &mut t_min, &mut t_max) {
        return None;
    }

    // far plane (z <= w)
    if !clip_halfspace(
        clip_a.w - clip_a.z,
        clip_b.w - clip_b.z,
        &mut t_min,
        &mut t_max,
    ) {
        return None;
    }

    let a_clipped = a.lerp(b, t_min);
    let b_clipped = a.lerp(b, t_max);
    if (b_clipped - a_clipped).length_squared() < 1e-10 {
        None
    } else {
        Some((a_clipped, b_clipped))
    }
}

fn build_face_path(points: &[ProjectedPoint; 4]) -> BezPath {
    let mut path = BezPath::new();
    path.move_to((points[0].x, points[0].y));
    path.line_to((points[1].x, points[1].y));
    path.line_to((points[2].x, points[2].y));
    path.line_to((points[3].x, points[3].y));
    path.close_path();
    path
}

fn face_uv_to_world(corners: &[Vec3; 4], u: f32, v: f32) -> Vec3 {
    let p00 = corners[0];
    let p10 = corners[1];
    let p01 = corners[3];
    p00 + (p10 - p00) * u + (p01 - p00) * v
}

fn stroke_face_line(
    scene: &mut Scene,
    corners: &[Vec3; 4],
    view_proj: Mat4,
    width: u32,
    height: u32,
    uv_start: (f32, f32),
    uv_end: (f32, f32),
    color: Color,
    thickness_px: f64,
) {
    let a = face_uv_to_world(corners, uv_start.0, uv_start.1);
    let b = face_uv_to_world(corners, uv_end.0, uv_end.1);
    let (Some(pa), Some(pb)) = (
        project_point(a, view_proj, width, height),
        project_point(b, view_proj, width, height),
    ) else {
        return;
    };

    let mut path = BezPath::new();
    path.move_to((pa.x, pa.y));
    path.line_to((pb.x, pb.y));
    scene.stroke(
        &Stroke::new(thickness_px),
        Affine::IDENTITY,
        color,
        None,
        &path,
    );
}

fn stroke_face_circle(
    scene: &mut Scene,
    corners: &[Vec3; 4],
    view_proj: Mat4,
    width: u32,
    height: u32,
    center_uv: (f32, f32),
    radius_uv: f32,
    segments: usize,
    color: Color,
    thickness_px: f64,
) {
    if segments < 3 {
        return;
    }

    let mut path = BezPath::new();
    for i in 0..=segments {
        let t = i as f32 / segments as f32;
        let angle = t * std::f32::consts::TAU;
        let u = center_uv.0 + radius_uv * angle.cos();
        let v = center_uv.1 + radius_uv * angle.sin();
        let world = face_uv_to_world(corners, u, v);
        let Some(projected) = project_point(world, view_proj, width, height) else {
            return;
        };

        if i == 0 {
            path.move_to((projected.x, projected.y));
        } else {
            path.line_to((projected.x, projected.y));
        }
    }
    path.close_path();

    scene.stroke(
        &Stroke::new(thickness_px),
        Affine::IDENTITY,
        color,
        None,
        &path,
    );
}

fn stroke_world_segment(
    scene: &mut Scene,
    view_proj: Mat4,
    width: u32,
    height: u32,
    a: Vec3,
    b: Vec3,
    color: Color,
    thickness_px: f64,
) {
    let Some((a, b)) = clip_segment_to_view_volume(a, b, view_proj) else {
        return;
    };

    let (Some(pa), Some(pb)) = (
        project_point(a, view_proj, width, height),
        project_point(b, view_proj, width, height),
    ) else {
        return;
    };

    let mut path = BezPath::new();
    path.move_to((pa.x, pa.y));
    path.line_to((pb.x, pb.y));
    scene.stroke(
        &Stroke::new(thickness_px),
        Affine::IDENTITY,
        color,
        None,
        &path,
    );
}

fn draw_ground_grid(
    scene: &mut Scene,
    model: Mat4,
    view_proj: Mat4,
    width: u32,
    height: u32,
    camera_pos: Vec3,
) {
    let grid_y = 0.0f32;
    let extent_cells = 90i32;
    let major_step = 5i32;
    let grid_step = CUBE_HALF_EXTENT;
    let max_coord = extent_cells as f32 * grid_step;

    for i in -extent_cells..=extent_cells {
        let v = i as f32 * grid_step;
        let major_line = i % major_step == 0;
        let thickness_px = if major_line { 1.35 } else { 0.85 };
        let base_alpha = if major_line { 145.0 } else { 65.0 };

        let radial_falloff = 1.0 - ((i.abs() as f32) / extent_cells as f32).powf(0.62);

        let x_line_mid = model.transform_point3(Vec3::new(v, grid_y, 0.0));
        let z_line_mid = model.transform_point3(Vec3::new(0.0, grid_y, v));
        let x_fade = (1.0 - x_line_mid.distance(camera_pos) / (max_coord * 1.45)).clamp(0.0, 1.0);
        let z_fade = (1.0 - z_line_mid.distance(camera_pos) / (max_coord * 1.45)).clamp(0.0, 1.0);

        let alpha_x = (base_alpha * radial_falloff * x_fade).clamp(0.0, 255.0) as u8;
        let alpha_z = (base_alpha * radial_falloff * z_fade).clamp(0.0, 255.0) as u8;

        if alpha_x > 3 {
            let center = model.transform_point3(Vec3::new(v, grid_y, 0.0));
            let a_pos = model.transform_point3(Vec3::new(v, grid_y, max_coord));
            let a_neg = model.transform_point3(Vec3::new(v, grid_y, -max_coord));
            let color = Color::from_rgba8(94, 111, 148, alpha_x);
            stroke_world_segment(
                scene,
                view_proj,
                width,
                height,
                center,
                a_pos,
                color,
                thickness_px,
            );
            stroke_world_segment(
                scene,
                view_proj,
                width,
                height,
                center,
                a_neg,
                color,
                thickness_px,
            );
        }

        if alpha_z > 3 {
            let center = model.transform_point3(Vec3::new(0.0, grid_y, v));
            let a_pos = model.transform_point3(Vec3::new(max_coord, grid_y, v));
            let a_neg = model.transform_point3(Vec3::new(-max_coord, grid_y, v));
            let color = Color::from_rgba8(94, 111, 148, alpha_z);
            stroke_world_segment(
                scene,
                view_proj,
                width,
                height,
                center,
                a_pos,
                color,
                thickness_px,
            );
            stroke_world_segment(
                scene,
                view_proj,
                width,
                height,
                center,
                a_neg,
                color,
                thickness_px,
            );
        }
    }
}

fn ray_hits_local_cube_before_point(
    camera_world: Vec3,
    point_world: Vec3,
    inv_model: Mat4,
    cube_half: f32,
) -> bool {
    let origin = inv_model.transform_point3(camera_world);
    let target = inv_model.transform_point3(point_world);
    let dir = target - origin;

    let mut t_min = 0.0f32;
    let mut t_max = 1.0f32;

    for axis in 0..3 {
        let o = origin[axis];
        let d = dir[axis];
        let min_b = -cube_half;
        let max_b = cube_half;

        if d.abs() < 1e-6 {
            if o < min_b || o > max_b {
                return false;
            }
            continue;
        }

        let mut t1 = (min_b - o) / d;
        let mut t2 = (max_b - o) / d;
        if t1 > t2 {
            std::mem::swap(&mut t1, &mut t2);
        }

        t_min = t_min.max(t1);
        t_max = t_max.min(t2);
        if t_max < t_min {
            return false;
        }
    }

    if t_max <= 0.0 {
        return false;
    }

    let entry = t_min.max(0.0);
    entry < 1.0 - 1e-4
}

fn draw_axis_layer(
    scene: &mut Scene,
    model: Mat4,
    inv_model: Mat4,
    view_proj: Mat4,
    width: u32,
    height: u32,
    camera_pos: Vec3,
    draw_occluded_layer: bool,
) {
    let cube_half = CUBE_HALF_EXTENT;
    let extent = 120.0f32;
    let segments = 260usize;
    let axis_defs = [
        (Vec3::X, (236u8, 89u8, 74u8)),
        (Vec3::Y, (109u8, 219u8, 130u8)),
        (Vec3::Z, (91u8, 149u8, 255u8)),
    ];

    for (axis_local, rgb) in axis_defs {
        let dir = model.transform_vector3(axis_local).normalize_or_zero();
        if dir.length_squared() < 1e-8 {
            continue;
        }

        for sign in [-1.0f32, 1.0f32] {
            let start = dir * (cube_half * sign);
            let end = dir * (extent * sign);
            let color_scale = if sign > 0.0 { 1.0 } else { 0.58 };
            let thickness_px = if sign > 0.0 { 2.6 } else { 1.7 };

            let draw_piece = |scene: &mut Scene, a: Vec3, b: Vec3| {
                let mid = (a + b) * 0.5;
                let d = (mid.length() - cube_half) / (extent - cube_half);
                let along_fade = (1.0 - d.clamp(0.0, 1.0)).powf(0.5);
                let distance_fade = (1.0 - mid.distance(camera_pos) / 190.0).clamp(0.0, 1.0);
                let alpha =
                    (255.0 * along_fade * distance_fade * color_scale).clamp(0.0, 255.0) as u8;
                if alpha < 5 {
                    return;
                }

                stroke_world_segment(
                    scene,
                    view_proj,
                    width,
                    height,
                    a,
                    b,
                    Color::from_rgba8(rgb.0, rgb.1, rgb.2, alpha),
                    thickness_px,
                );
            };

            let step = 1.0 / segments as f32;
            let mut prev = start;
            let mut prev_occ =
                ray_hits_local_cube_before_point(camera_pos, prev, inv_model, cube_half);

            for i in 1..=segments {
                let t1 = i as f32 * step;
                let curr = start.lerp(end, t1);
                let curr_occ =
                    ray_hits_local_cube_before_point(camera_pos, curr, inv_model, cube_half);

                if prev_occ == curr_occ {
                    if prev_occ == draw_occluded_layer {
                        draw_piece(scene, prev, curr);
                    }
                } else {
                    let mut lo = 0.0f32;
                    let mut hi = 1.0f32;
                    let left_occ = prev_occ;

                    for _ in 0..12 {
                        let mid_t = (lo + hi) * 0.5;
                        let p = prev.lerp(curr, mid_t);
                        let occ =
                            ray_hits_local_cube_before_point(camera_pos, p, inv_model, cube_half);
                        if occ == left_occ {
                            lo = mid_t;
                        } else {
                            hi = mid_t;
                        }
                    }

                    let split = prev.lerp(curr, (lo + hi) * 0.5);
                    if prev_occ == draw_occluded_layer {
                        draw_piece(scene, prev, split);
                    }
                    if curr_occ == draw_occluded_layer {
                        draw_piece(scene, split, curr);
                    }
                }

                prev = curr;
                prev_occ = curr_occ;
            }
        }
    }
}

fn rebuild_scene(scene: &mut Scene, width: u32, height: u32, camera: Camera) {
    let w = width as f64;
    let h = height as f64;

    scene.reset();

    scene.fill(
        Fill::NonZero,
        Affine::IDENTITY,
        Color::from_rgb8(18, 20, 28),
        None,
        &Rect::new(0.0, 0.0, w, h),
    );

    let aspect = (width as f32 / height.max(1) as f32).max(0.1);
    let model = Mat4::IDENTITY;
    let inv_model = model.inverse();
    let camera_pos = camera_eye(&camera);
    let view = Mat4::look_at_rh(camera_pos, camera.target, Vec3::Y);
    let proj = Mat4::perspective_rh(camera.fov_y_radians, aspect, 0.1, 1000.0);
    let view_proj = proj * view;

    draw_ground_grid(scene, model, view_proj, width, height, camera_pos);
    draw_axis_layer(
        scene, model, inv_model, view_proj, width, height, camera_pos, true,
    );

    let world_vertices = CUBE_VERTICES.map(|v| model.transform_point3(v * CUBE_HALF_EXTENT));
    let projected_vertices = world_vertices.map(|p| project_point(p, view_proj, width, height));

    let mut visible_faces: Vec<FaceInfo> = Vec::new();
    for face in CUBE_FACES {
        let world = [
            world_vertices[face[0]],
            world_vertices[face[1]],
            world_vertices[face[2]],
            world_vertices[face[3]],
        ];

        let projected = match (
            projected_vertices[face[0]],
            projected_vertices[face[1]],
            projected_vertices[face[2]],
            projected_vertices[face[3]],
        ) {
            (Some(a), Some(b), Some(c), Some(d)) => [a, b, c, d],
            _ => continue,
        };

        let normal = (world[1] - world[0])
            .cross(world[3] - world[0])
            .normalize_or_zero();
        if normal.length_squared() < 1e-8 {
            continue;
        }

        let center = (world[0] + world[1] + world[2] + world[3]) * 0.25;
        let to_camera = (camera_pos - center).normalize_or_zero();
        if normal.dot(to_camera) <= 0.0 {
            continue;
        }

        let avg_depth =
            (projected[0].ndc_z + projected[1].ndc_z + projected[2].ndc_z + projected[3].ndc_z)
                * 0.25;

        visible_faces.push(FaceInfo {
            projected,
            world,
            normal,
            avg_depth,
        });
    }

    visible_faces.sort_by(|a, b| {
        b.avg_depth
            .partial_cmp(&a.avg_depth)
            .unwrap_or(Ordering::Equal)
    });

    let light_dir = Vec3::new(-0.35, 0.85, 0.38).normalize();
    for face in visible_faces {
        let lambert = face.normal.dot(light_dir).max(0.0);
        let tone = 38.0 + lambert * 82.0;
        let face_fill = Color::from_rgb8(
            (tone + 20.0).clamp(0.0, 255.0) as u8,
            (tone + 35.0).clamp(0.0, 255.0) as u8,
            (tone + 55.0).clamp(0.0, 255.0) as u8,
        );

        let face_path = build_face_path(&face.projected);
        scene.fill(Fill::NonZero, Affine::IDENTITY, face_fill, None, &face_path);
        scene.stroke(
            &Stroke::new(2.0),
            Affine::IDENTITY,
            Color::from_rgba8(230, 238, 255, 230),
            None,
            &face_path,
        );

        for step in 1..10 {
            let t = step as f32 / 10.0;
            let major = step == 5;
            let stroke_width = if major { 2.3 } else { 1.0 };
            let alpha = if major { 235 } else { 130 };
            let grid_color = Color::from_rgba8(120, 195, 255, alpha);

            stroke_face_line(
                scene,
                &face.world,
                view_proj,
                width,
                height,
                (t, 0.0),
                (t, 1.0),
                grid_color,
                stroke_width,
            );
            stroke_face_line(
                scene,
                &face.world,
                view_proj,
                width,
                height,
                (0.0, t),
                (1.0, t),
                grid_color,
                stroke_width,
            );
        }

        stroke_face_circle(
            scene,
            &face.world,
            view_proj,
            width,
            height,
            (0.5, 0.5),
            0.28,
            64,
            Color::from_rgba8(248, 170, 92, 235),
            2.4,
        );
        stroke_face_circle(
            scene,
            &face.world,
            view_proj,
            width,
            height,
            (0.5, 0.5),
            0.12,
            48,
            Color::from_rgba8(248, 170, 92, 180),
            1.4,
        );

        stroke_face_line(
            scene,
            &face.world,
            view_proj,
            width,
            height,
            (0.15, 0.15),
            (0.85, 0.85),
            Color::from_rgba8(255, 206, 120, 220),
            1.5,
        );
        stroke_face_line(
            scene,
            &face.world,
            view_proj,
            width,
            height,
            (0.85, 0.15),
            (0.15, 0.85),
            Color::from_rgba8(255, 206, 120, 220),
            1.5,
        );
    }

    draw_axis_layer(
        scene, model, inv_model, view_proj, width, height, camera_pos, false,
    );

    scene.stroke(
        &Stroke::new(3.0),
        Affine::IDENTITY,
        Color::from_rgba8(238, 243, 255, 210),
        None,
        &Rect::new(24.0, 24.0, w - 24.0, h - 24.0),
    );
}

pub fn zoom_viewport(viewport: &mut Viewport, wheel_delta: f32) {
    if !wheel_delta.is_finite() {
        return;
    }

    let zoom_scale = 2.0f32.powf(-wheel_delta * 0.12);
    viewport.camera.distance = (viewport.camera.distance * zoom_scale).clamp(0.8, 500.0);
}

pub fn pan_viewport(viewport: &mut Viewport, delta_x: f32, delta_y: f32) {
    if !delta_x.is_finite() || !delta_y.is_finite() {
        return;
    }

    let (_, _, right, up) = camera_basis(&viewport.camera);
    let height = viewport.config.height.max(1) as f32;
    let world_per_pixel =
        2.0 * viewport.camera.distance * (viewport.camera.fov_y_radians * 0.5).tan() / height;

    let translation = right * (-delta_x * world_per_pixel) + up * (delta_y * world_per_pixel);
    viewport.camera.target += translation;
}

pub fn orbit_viewport(viewport: &mut Viewport, delta_x: f32, delta_y: f32, constrained: bool) {
    if !delta_x.is_finite() || !delta_y.is_finite() {
        return;
    }

    let sensitivity = 0.006;
    let mut yaw_delta = -delta_x * sensitivity;
    let mut pitch_delta = delta_y * sensitivity;

    if constrained {
        if delta_x.abs() >= delta_y.abs() {
            pitch_delta = 0.0;
        } else {
            yaw_delta = 0.0;
        }
    }

    viewport.camera.yaw += yaw_delta;
    viewport.camera.pitch = (viewport.camera.pitch + pitch_delta).clamp(-1.52, 1.52);
}

pub fn init_viewport(
    gpu: &Gpu,
    window: RawWindowHandle,
    display: RawDisplayHandle,
    width: u32,
    height: u32,
) -> Option<Viewport> {
    let width = width.max(1);
    let height = height.max(1);

    let surface = unsafe {
        gpu.instance
            .create_surface_unsafe(wgpu::SurfaceTargetUnsafe::RawHandle {
                raw_display_handle: display,
                raw_window_handle: window,
            })
            .ok()?
    };

    let capabilities = surface.get_capabilities(&gpu.adapter);
    let format = capabilities
        .formats
        .iter()
        .copied()
        .find(|format| {
            matches!(
                format,
                wgpu::TextureFormat::Bgra8Unorm | wgpu::TextureFormat::Rgba8Unorm
            )
        })
        .unwrap_or(capabilities.formats[0]);

    let config = wgpu::SurfaceConfiguration {
        usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
        format,
        width,
        height,
        present_mode: wgpu::PresentMode::AutoVsync,
        alpha_mode: capabilities.alpha_modes[0],
        view_formats: vec![],
        desired_maximum_frame_latency: 2,
    };
    surface.configure(&gpu.device, &config);

    let (target_texture, target_view) = create_target_texture(&gpu.device, width, height);
    let blitter = TextureBlitter::new(&gpu.device, format);

    Some(Viewport {
        surface,
        config,
        target_texture,
        target_view,
        blitter,
        scene: Scene::new(),
        camera: default_camera(),
    })
}

pub fn resize_viewport(gpu: &Gpu, viewport: &mut Viewport, width: u32, height: u32) {
    if width == 0 || height == 0 {
        return;
    }

    if viewport.config.width == width && viewport.config.height == height {
        return;
    }

    viewport.config.width = width;
    viewport.config.height = height;
    viewport.surface.configure(&gpu.device, &viewport.config);
    (viewport.target_texture, viewport.target_view) =
        create_target_texture(&gpu.device, width, height);
}

pub fn render_viewport(gpu: &mut Gpu, viewport: &mut Viewport) {
    if viewport.config.width == 0 || viewport.config.height == 0 {
        return;
    }

    rebuild_scene(
        &mut viewport.scene,
        viewport.config.width,
        viewport.config.height,
        viewport.camera,
    );

    let output = match viewport.surface.get_current_texture() {
        Ok(texture) => texture,
        Err(wgpu::SurfaceError::Lost | wgpu::SurfaceError::Outdated) => {
            viewport.surface.configure(&gpu.device, &viewport.config);
            match viewport.surface.get_current_texture() {
                Ok(texture) => texture,
                Err(_) => return,
            }
        }
        Err(wgpu::SurfaceError::OutOfMemory) => {
            eprintln!("Surface out of memory");
            return;
        }
        Err(wgpu::SurfaceError::Timeout) => return,
        Err(wgpu::SurfaceError::Other) => return,
    };

    if gpu
        .renderer
        .render_to_texture(
            &gpu.device,
            &gpu.queue,
            &viewport.scene,
            &viewport.target_view,
            &RenderParams {
                base_color: Color::from_rgb8(0, 0, 0),
                width: viewport.config.width,
                height: viewport.config.height,
                antialiasing_method: AaConfig::Area,
            },
        )
        .is_err()
    {
        return;
    }

    let output_view = output
        .texture
        .create_view(&wgpu::TextureViewDescriptor::default());
    let mut encoder = gpu
        .device
        .create_command_encoder(&wgpu::CommandEncoderDescriptor {
            label: Some("vello-blit"),
        });
    viewport.blitter.copy(
        &gpu.device,
        &mut encoder,
        &viewport.target_view,
        &output_view,
    );

    gpu.queue.submit(Some(encoder.finish()));
    output.present();
}
