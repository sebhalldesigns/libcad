use raw_window_handle::{RawDisplayHandle, RawWindowHandle};

pub struct Gpu
{
    pub adapter: wgpu::Adapter,
    instance: wgpu::Instance,
    device: wgpu::Device,
    queue: wgpu::Queue
}

pub fn init() -> Option<Gpu>
{
    #[cfg(not(target_arch = "wasm32"))]
    {
        env_logger::init();
    }

    #[cfg(target_arch = "wasm32")]
    {
        console_log::init_with_level(log::Level::Info).unwrap_throw();
    }

    let instance = wgpu::Instance::new
    (
        &wgpu::InstanceDescriptor 
        {
            backends: wgpu::Backends::all(),
            ..Default::default()
        }
    );

    let adapter = pollster::block_on
    (
        instance.request_adapter
        (
            &wgpu::RequestAdapterOptions 
            {
                power_preference: wgpu::PowerPreference::None, /* don't care about performance for now */
                compatible_surface: None, /* no surface needed for now */
                force_fallback_adapter: false
            }
        )
    ).expect("Failed to find an appropriate GPU adapter");

    println!("Found adapter: {}", adapter.get_info().name);

    let (device, queue) = pollster::block_on(
        adapter.request_device(&wgpu::DeviceDescriptor::default())
    ).unwrap();

    return Some( Gpu { instance, adapter, device, queue } );

}

pub struct Viewport
{
    surface: wgpu::Surface<'static>,
    config: wgpu::SurfaceConfiguration

} 

pub fn init_viewport(gpu: &Gpu, window: RawWindowHandle, display: RawDisplayHandle, width: u32, height: u32) -> Option<Viewport>
{
    let surface = unsafe 
    {
        gpu.instance.create_surface_unsafe
        (
            wgpu::SurfaceTargetUnsafe::RawHandle 
            {
                raw_display_handle: display,
                raw_window_handle: window,
            }
        ).ok()?
    };

    let capabilities = surface.get_capabilities(&gpu.adapter);

    let config = wgpu::SurfaceConfiguration {
        usage: wgpu::TextureUsages::RENDER_ATTACHMENT,
        format: capabilities.formats[0],
        width,
        height,
        present_mode: wgpu::PresentMode::AutoVsync,
        alpha_mode: capabilities.alpha_modes[0],
        view_formats: vec![],
        desired_maximum_frame_latency: 1,
    };

    surface.configure(&gpu.device, &config);

    return Some( Viewport { surface, config } );
}   

pub fn resize_viewport(gpu: &Gpu, viewport: &mut Viewport, width: u32, height: u32)
{
    if width == 0 || height == 0 {
        return;
    }

    if viewport.config.width == width && viewport.config.height == height {
        return;
    }

    viewport.config.width = width;
    viewport.config.height = height;
    viewport.surface.configure(&gpu.device, &viewport.config);
}

pub fn render_viewport(gpu: &Gpu, viewport: &Viewport)
{
    let output = match viewport.surface.get_current_texture()
    {
        Ok(texture) => texture,
        Err(wgpu::SurfaceError::Lost | wgpu::SurfaceError::Outdated) =>
        {
            viewport.surface.configure(&gpu.device, &viewport.config);
            match viewport.surface.get_current_texture() 
            {
                Ok(texture) => texture,
                Err(_) => return,
            }
        } 
        Err(_) => return,
    };

    let view = output.texture.create_view(&wgpu::TextureViewDescriptor::default());

    let mut encoder = gpu.device.create_command_encoder(
        &wgpu::CommandEncoderDescriptor { label: Some("render") }
    );

    let render_pass = encoder.begin_render_pass(
        &wgpu::RenderPassDescriptor 
        {
            label: Some("clear"),
            color_attachments: &[Some(wgpu::RenderPassColorAttachment {
                    view: &view,
                    resolve_target: None,
                    depth_slice: None,
                    ops: wgpu::Operations {
                        load: wgpu::LoadOp::Clear(wgpu::Color {
                            r: 0.1, g: 0.1, b: 0.1, a: 1.0,
                        }),
                        store: wgpu::StoreOp::Store,
                    },
                })],
            depth_stencil_attachment: None,
            ..Default::default()
        }
    );

    /* force drop so we can use encoder */
    drop(render_pass); 

    gpu.queue.submit(Some(encoder.finish()));
    output.present();

}
