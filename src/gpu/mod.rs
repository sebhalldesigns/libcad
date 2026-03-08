use std::{cmp::Ordering, time::Instant};

use glam::{Mat4, Vec3};
use raw_window_handle::{RawDisplayHandle, RawWindowHandle};
use vello::kurbo::{Affine, BezPath, Circle, Rect, Stroke};
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
    start_time: Instant,
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

const CUBE_FACES: [[usize; 4]; 6] = [
    [4, 5, 6, 7], // front (+Z)
    [1, 0, 3, 2], // back (-Z)
    [5, 1, 2, 6], // right (+X)
    [0, 4, 7, 3], // left (-X)
    [7, 6, 2, 3], // top (+Y)
    [0, 1, 5, 4], // bottom (-Y)
];

fn project_point(world: Vec3, view_proj: Mat4, width: u32, height: u32) -> Option<ProjectedPoint> {
    let clip = view_proj * world.extend(1.0);
    if clip.w <= 0.0001 {
        return None;
    }

    let ndc = clip.truncate() / clip.w;
    let x = ((ndc.x as f64) * 0.5 + 0.5) * width as f64;
    let y = (1.0 - ((ndc.y as f64) * 0.5 + 0.5)) * height as f64;

    Some(ProjectedPoint { x, y, ndc_z: ndc.z })
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

fn rebuild_scene(scene: &mut Scene, width: u32, height: u32, elapsed_seconds: f64) {
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
    let t = elapsed_seconds as f32;

    let model = Mat4::from_rotation_y(t * 0.75) * Mat4::from_rotation_x(t * 0.43);
    let camera_pos = Vec3::new(3.3, 2.5, 4.8);
    let view = Mat4::look_at_rh(camera_pos, Vec3::ZERO, Vec3::Y);
    let proj = Mat4::perspective_rh(40.0f32.to_radians(), aspect, 0.1, 100.0);
    let view_proj = proj * view;

    let world_vertices = CUBE_VERTICES.map(|v| model.transform_point3(v * 1.25));
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

    scene.stroke(
        &Stroke::new(3.0),
        Affine::IDENTITY,
        Color::from_rgba8(238, 243, 255, 210),
        None,
        &Rect::new(24.0, 24.0, w - 24.0, h - 24.0),
    );
    let pulse = Circle::new(
        (w * 0.11, h * 0.12),
        8.0 + elapsed_seconds.sin().abs() * 5.0,
    );
    scene.fill(
        Fill::NonZero,
        Affine::IDENTITY,
        Color::from_rgb8(102, 186, 255),
        None,
        &pulse,
    );
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
        start_time: Instant::now(),
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
        viewport.start_time.elapsed().as_secs_f64(),
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
