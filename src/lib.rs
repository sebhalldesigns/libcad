use raw_window_handle::{RawDisplayHandle, RawWindowHandle};

mod gpu;

struct Context {
    gpu: gpu::Gpu,
}

pub type Handle = u64;

#[unsafe(no_mangle)]
pub fn create_context() -> Handle {
    println!("libcad v0.0.1");

    let gpu = gpu::init();

    if gpu.is_none() {
        println!("Failed to initialize GPU context");
        return 0;
    }

    let context = Context { gpu: gpu.unwrap() };

    return Box::into_raw(Box::new(context)) as Handle;
}

#[unsafe(no_mangle)]
pub fn init_viewport(
    context: Handle,
    window: RawWindowHandle,
    display: RawDisplayHandle,
    width: u32,
    height: u32,
) -> Handle {
    let context = unsafe { &mut *(context as *mut Context) };

    println!(
        "Creating viewport... {}",
        context.gpu.adapter.get_info().name
    );

    let viewport = match gpu::init_viewport(&context.gpu, window, display, width, height) {
        Some(viewport) => viewport,
        None => {
            println!("Failed to initialize viewport");
            return 0;
        }
    };

    return Box::into_raw(Box::new(viewport)) as Handle;
}

#[unsafe(no_mangle)]
pub fn resize_viewport(context: Handle, viewport: Handle, width: u32, height: u32) {
    let context = unsafe { &mut *(context as *mut Context) };
    let viewport = unsafe { &mut *(viewport as *mut gpu::Viewport) };

    gpu::resize_viewport(&context.gpu, viewport, width, height);
}

#[unsafe(no_mangle)]
pub fn render_viewport(context: Handle, viewport: Handle) {
    let context = unsafe { &mut *(context as *mut Context) };
    let viewport = unsafe { &mut *(viewport as *mut gpu::Viewport) };

    gpu::render_viewport(&mut context.gpu, viewport);
}

#[unsafe(no_mangle)]
pub fn zoom_viewport(viewport: Handle, wheel_delta: f32) {
    let viewport = unsafe { &mut *(viewport as *mut gpu::Viewport) };
    gpu::zoom_viewport(viewport, wheel_delta);
}

#[unsafe(no_mangle)]
pub fn pan_viewport(viewport: Handle, delta_x: f32, delta_y: f32) {
    let viewport = unsafe { &mut *(viewport as *mut gpu::Viewport) };
    gpu::pan_viewport(viewport, delta_x, delta_y);
}

#[unsafe(no_mangle)]
pub fn orbit_viewport(viewport: Handle, delta_x: f32, delta_y: f32, constrained: bool) {
    let viewport = unsafe { &mut *(viewport as *mut gpu::Viewport) };
    gpu::orbit_viewport(viewport, delta_x, delta_y, constrained);
}
